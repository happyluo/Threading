// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_SINGLETON_H
#define CONCURRENCY_SINGLETON_H

#include <Util/AtExitManager.h>
#include <Util/AtomicOps.h>
#include <Concurrency/ThreadControl.h>
#include <Concurrency/ThreadRestrictions.h>

// Default traits for Singleton<Type>. Calls operator new and operator delete on
// the object. Registers automatic deletion at process exit.
// Overload if you need arguments or another memory allocation function.
template<typename Type>
struct DefaultSingletonTraits
{
	// Allocates the object.
	static Type* New()
	{
		// The parenthesis is very important here; it forces POD type
		// initialization.
		return new Type();
	}

	// Destroys the object.
	static void Delete(Type* x)
	{
		delete x;
	}

	// Set to true to automatically register deletion of the object on process
	// exit. See below for the required call that makes this happen.
	static const bool kRegisterAtExit = true;

	// Set to false to disallow access on a non-joinable thread.  This is
	// different from kRegisterAtExit because StaticMemorySingletonTraits allows
	// access on non-joinable threads, and gracefully handles this.
	static const bool kAllowedToAccessOnNonjoinableThread = false;
};


// Alternate traits for use with the Singleton<Type>.  Identical to
// DefaultSingletonTraits except that the Singleton will not be cleaned up
// at exit.
template<typename Type>
struct LeakySingletonTraits : public DefaultSingletonTraits<Type> 
{
	static const bool kRegisterAtExit = false;
	static const bool kAllowedToAccessOnNonjoinableThread = true;
};


// Alternate traits for use with the Singleton<Type>.  Allocates memory
// for the singleton instance from a static buffer.  The singleton will
// be cleaned up at exit, but can't be revived after destruction unless
// the Resurrect() method is called.
//
// This is useful for a certain category of things, notably logging and
// tracing, where the singleton instance is of a type carefully constructed to
// be safe to access post-destruction.
// In logging and tracing you'll typically get stray calls at odd times, like
// during static destruction, thread teardown and the like, and there's a
// termination race on the heap-based singleton - e.g. if one thread calls
// Get(), but then another thread initiates AtExit processing, the first thread
// may call into an object residing in unallocated memory. If the instance is
// allocated from the data segment, then this is survivable.
//
// The destructor is to deallocate system resources, in this case to unregister
// a callback the system will invoke when logging levels change. Note that
// this is also used in e.g. Chrome Frame, where you have to allow for the
// possibility of loading briefly into someone else's process space, and
// so leaking is not an option, as that would sabotage the state of your host
// process once you've unloaded.
template <typename Type>
struct StaticMemorySingletonTraits
{
	// WARNING: User has to deal with Get() in the singleton class
	// this is traits for returning NULL.
	static Type* New()
	{
		if (UtilInternal::NoBarrier_AtomicExchange(&ms_dead, 1))
		{
			return NULL;
		}
		Type* ptr = reinterpret_cast<Type*>(ms_buffer);

		// We are protected by a memory barrier.
		new(ptr) Type();
		return ptr;
	}

	static void Delete(Type* p)
	{
		UtilInternal::NoBarrier_Store(&ms_dead, 1);
		UtilInternal::MemoryBarrier();
		if (p != NULL)
		{
			p->Type::~Type();
		}
	}

	static const bool kRegisterAtExit = true;
	static const bool kAllowedToAccessOnNonjoinableThread = true;

	// Exposed for unittesting.
	static void Resurrect()
	{
		UtilInternal::NoBarrier_Store(&ms_dead, 0);
	}

private:
	static const size_t kBufferSize = 
		(sizeof(Type) + sizeof(intptr_t) - 1) / sizeof(intptr_t);
	static intptr_t ms_buffer[kBufferSize];

	// Signal the object was already deleted, so it is not revived.
	static UtilInternal::Atomic32 ms_dead;
};

template <typename Type> 
intptr_t StaticMemorySingletonTraits<Type>::ms_buffer[kBufferSize];
template <typename Type> 
UtilInternal::Atomic32 StaticMemorySingletonTraits<Type>::ms_dead = 0;


// The Singleton<Type, Traits, DifferentiatingType> class manages a single
// instance of Type which will be created on first use and will be destroyed at
// normal process exit). The Trait::Delete function will not be called on
// abnormal process exit.
//
// DifferentiatingType is used as a key to differentiate two different
// singletons having the same memory allocation functions but serving a
// different purpose. This is mainly used for Locks serving different purposes.
//
// Example usages: (none are preferred, they all result in the same code)
//   1. FooClass* ptr = Singleton<FooClass>::Get();
//      ptr->Bar();
//   2. Singleton<FooClass>()->Bar();
//   3. Singleton<FooClass>::Get()->Bar();
//
// Singleton<> has no non-static members and doesn't need to actually be
// instantiated. It does no harm to instantiate it and use it as a class member
// or at global level since it is acting as a POD type.
//
// This class is itself thread-safe. The underlying Type must of course be
// thread-safe if you want to use it concurrently. Two parameters may be tuned
// depending on the user's requirements.
//
// Glossary:
//   RAE = kRegisterAtExit
//
// On every platform, if Traits::RAE is true, the singleton will be destroyed at
// process exit. More precisely it uses Util::AtExitManager which requires an
// object of this type to be instantiated. AtExitManager mimics the semantics
// of atexit() such as LIFO order but under Windows is safer to call. For more
// information see at_exit.h.
//
// If Traits::RAE is false, the singleton will not be freed at process exit,
// thus the singleton will be leaked if it is ever accessed. Traits::RAE
// shouldn't be false unless absolutely necessary. Remember that the heap where
// the object is allocated may be destroyed by the CRT anyway.
//
// If you want to ensure that your class can only exist as a singleton, make
// its constructors private, and make DefaultSingletonTraits<> a friend:
//
//   #include <Util/Singleton.h>
//   class FooClass {
//    public:
//     void Bar() { ... }
//    private:
//     FooClass() { ... }
//     friend struct DefaultSingletonTraits<FooClass>;
//
//     DISALLOW_COPY_AND_ASSIGN(FooClass);
//   };
//
// Caveats:
// (a) Every call to Get(), operator->() and operator*() incurs some overhead
//     (16ns on my P4/2.8GHz) to check whether the object has already been
//     initialized.  You may wish to cache the result of Get(); it will not
//     change.
//
// (b) Your factory function must never throw an exception. This class is not
//     exception-safe.
//
template <typename Type,
	typename Traits = DefaultSingletonTraits<Type>,
	typename DifferentiatingType = Type>
class Singleton 
{
public:
	// This class is safe to be constructed and copy-constructed since it has no
	// member.

	// Return a pointer to the one true instance of the class.
	static Type* Get()
	{
		if (!Traits::kAllowedToAccessOnNonjoinableThread)
		{
			Util::ThreadRestrictions::AssertSingletonAllowed();
		}

		// Our AtomicWord doubles as a spinlock, where a value of
		// kBeingCreatedMarker means the spinlock is being held for creation.
		static const UtilInternal::AtomicWord kBeingCreatedMarker = 1;

		UtilInternal::AtomicWord value = UtilInternal::NoBarrier_Load(&ms_instance);
		if (value != 0 && value != kBeingCreatedMarker)
		{
			// See the corresponding HAPPENS_BEFORE below.
			//ANNOTATE_HAPPENS_AFTER(&ms_instance);
			return reinterpret_cast<Type*>(value);
		}

		// Object isn't created yet, maybe we will Get to create it, let's try...
		if (0 == UtilInternal::Acquire_CompareAndSwap(&ms_instance, 0, kBeingCreatedMarker)) 
		{
			// ms_instance was NULL and is now kBeingCreatedMarker.  Only one thread
			// will ever get here.  Threads might be spinning on us, and they will
			// stop right after we do this store.
			Type* newval = Traits::New();

			// This annotation helps race detectors recognize correct lock-less
			// synchronization between different threads calling Get().
			// See the corresponding HAPPENS_AFTER below and above.
			//ANNOTATE_HAPPENS_BEFORE(&ms_instance);
			UtilInternal::Release_Store(
				&ms_instance, reinterpret_cast<UtilInternal::AtomicWord>(newval));

			if (newval != NULL && Traits::kRegisterAtExit)
				Util::AtExitManager::RegisterCallback(OnExit, NULL);

			return newval;
		}

		// We hit a race.  Another thread beat us and either:
		// - Has the object in BeingCreated state
		// - Already has the object created...
		// We know value != NULL.  It could be kBeingCreatedMarker, or a valid ptr.
		// Unless your constructor can be very time consuming, it is very unlikely
		// to hit this race.  When it does, we just spin and yield the thread until
		// the object has been created.
		while (true)
		{
			value = UtilInternal::NoBarrier_Load(&ms_instance);
			if (value != kBeingCreatedMarker)
			{
				break;
			}
			// Yield Current Thread
			Util::ThreadControl::Yield();
		}

		// See the corresponding HAPPENS_BEFORE above.
		//ANNOTATE_HAPPENS_AFTER(&ms_instance);
		return reinterpret_cast<Type*>(value);
	}

	// Shortcuts.
	Type& operator*()
	{
		return *Get();
	}

	Type* operator->()
	{
		return Get();
	}

private:
	// Adapter function for use with AtExit().  This should be called single
	// threaded, so don't use atomic operations.
	// Calling OnExit while singleton is in use by other threads is a mistake.
	static void OnExit(void* unused)
	{
		// AtExit should only ever be register after the singleton instance was
		// created.  We should only ever get here with a valid ms_instance pointer.
		Traits::Delete(
			reinterpret_cast<Type*>(UtilInternal::NoBarrier_Load(&ms_instance)));
		ms_instance = 0;
	}
	static UtilInternal::AtomicWord ms_instance;
};

template <typename Type, typename Traits, typename DifferentiatingType>
UtilInternal::AtomicWord Singleton<Type, Traits, DifferentiatingType>::ms_instance = 0;

#endif  // CONCURRENCY_SINGLETON_H