// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_LOCAL_H
#define CONCURRENCY_THREAD_LOCAL_H

#include <Concurrency/Config.h>
#include <Util/TypeTraits.h>
#include <Concurrency/ThreadException.h>

CONCURRENCY_BEGIN

// Helpers for ThreadLocal.

class ThreadLocalValueHolderBase 
{
public:
	virtual ~ThreadLocalValueHolderBase()
	{
	}
};

// Called by pthread to delete thread-local data stored by
// pthread_setspecific().
extern "C" inline void DeleteThreadLocalValue(void* value_holder)
{
	delete static_cast<ThreadLocalValueHolderBase*>(value_holder);
}


// An object managed for a thread by a ThreadLocal instance is deleted
// when the thread exits.  Or, if the ThreadLocal instance dies in
// that thread, when the ThreadLocal dies.  It's the user's
// responsibility to ensure that all other threads using a ThreadLocal
// have exited when it dies, or the per-thread objects for those
// threads will not be deleted.
//
// The global ThreadLocal objects will die after main() has returned. 
// Therefore, no per-thread object will be leaked as long as all threads
// have exited when main() returns.
template <typename T>
class ThreadLocal
{
public:
	ThreadLocal() : m_key(CreateKey()), m_default() 
	{
	}

	explicit ThreadLocal(const T& value) : m_key(CreateKey()),
		m_default(value)
	{
	}

	~ThreadLocal() 
	{
#ifdef _WIN32
		void* val = TlsGetValue(m_key);
		if (0 != val)
		{
			DeleteThreadLocalValue(val);
		}

		// Releases resources associated with the key.  This will *not*
		// delete managed objects for other threads.
		//CHECK_SUCCESS(0 != TlsFree(m_key));
		if (0 == TlsFree(m_key))
		{
			assert(0);
		}
#elif HAS_PTHREAD
		void* val = pthread_getspecific(m_key);
		if (0 != val)
		{
			// Destroys the managed object for the current thread, if any.
			DeleteThreadLocalValue(val);
		}

		// Releases resources associated with the key.  This will *not*
		// delete managed objects for other threads.
		//CHECK_POSIX_SUCCESS(pthread_key_delete(m_key));
		if (0 != pthread_key_delete(m_key))
		{
			assert(0);
		}
#endif
	}

	T* Pointer()
	{
		return GetOrCreateValue(); 
	}

	const T* Pointer() const 
	{ 
		return GetOrCreateValue();
	}

	const T& Get() const 
	{ 
		return *Pointer(); 
	}

	void Set(const T& value) 
	{
		*Pointer() = value; 
	}

private:
	// Holds a value of type T.
	class ValueHolder : public ThreadLocalValueHolderBase
	{
	public:
		explicit ValueHolder(const T& value) : m_value(value)
		{
		}

		T* Pointer() 
		{ 
			return &m_value; 
		}

	private:
		T m_value;
		DISALLOW_COPY_AND_ASSIGN(ValueHolder);
	};


	//
	// Create thread-specific key
	//
#ifdef _WIN32
	static DWORD CreateKey()
#else if HAS_PTHREAD
	static pthread_key_t CreateKey()
#endif
	{
#ifdef _WIN32

		DWORD key = TlsAlloc();
		if (TLS_OUT_OF_INDEXES == key)
		{
			throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
		}

#else if HAS_PTHREAD

		pthread_key_t key;
		// When a thread exits, DeleteThreadLocalValue() will be called on
		// the object managed for that thread.
#	ifdef __SUNPRO_CC
		int rs = pthread_key_create(&key, reinterpret_cast<PthreadKeyDestructor>(&DeleteThreadLocalValue));
#	else
		//CHECK_POSIX_SUCCESS(
		//	pthread_key_create(&key, &DeleteThreadLocalValue));
		int rs = pthread_key_create(&key, &DeleteThreadLocalValue);
#	endif

		if (0 != rs)
		{
			throw Util::ThreadSyscallException(__FILE__, __LINE__, rs);
		}
#endif
		return key;
	}

	T* GetOrCreateValue() const 
	{
#ifdef _WIN32
		ThreadLocalValueHolderBase* const holder = 
			static_cast<ThreadLocalValueHolderBase*>(TlsGetValue(m_key));
#elif HAS_PTHREAD
		ThreadLocalValueHolderBase* const holder =
			static_cast<ThreadLocalValueHolderBase*>(pthread_getspecific(m_key));
#endif
		if (NULL != holder) 
		{
			return UtilInternal::CheckedDowncastToActualType<ValueHolder>(holder)->Pointer();
		}

		ValueHolder* const new_holder = new ValueHolder(m_default);
		ThreadLocalValueHolderBase* const holder_base = new_holder;

#ifdef _WIN32
		if (0 == TlsSetValue(m_key, holder_base))
		{
			throw Util::ThreadSyscallException(__FILE__, __LINE__, GetLastError());
		}
#elif HAS_PTHREAD
		//CHECK_POSIX_SUCCESS(pthread_setspecific(m_key, holder_base));
		int rs = pthread_setspecific(m_key, holder_base);
		if (0 != rs)
		{
			throw Util::ThreadSyscallException(__FILE__, __LINE__, rs);
		}
#endif
		return new_holder->Pointer();
	}

#ifdef _WIN32
	DWORD m_key;
#else if HAS_PTHREAD  
	// A key pthreads uses for looking up per-thread values.
	const pthread_key_t m_key;
#endif

	const T m_default;  // The default value for each thread.

	DISALLOW_COPY_AND_ASSIGN(ThreadLocal);
};


CONCURRENCY_END

#endif