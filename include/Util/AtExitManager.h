// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_AT_EXIT_MANAGER_H
#define UTIL_AT_EXIT_MANAGER_H
#pragma once


#include <stack>

#include <Util/Config.h>
#include <Concurrency/Mutex.h>

namespace Util
{

// This class provides a facility similar to the CRT atexit(), except that
// we control when the callbacks are executed. Under Windows for a DLL they
// happen at a really bad time and under the loader lock. This facility is
// mostly used by Util::Singleton.
//
// The usage is simple. Early in the main() or WinMain() scope create an
// AtExitManager object on the stack:
// int main(...) {
//    Util::AtExitManager exit_manager;
//
// }
// When the exit_manager object goes out of scope, all the registered
// callbacks and singleton destructors will be called.

class UTIL_API AtExitManager
{
protected:
	// This constructor will allow this instance of AtExitManager to be created
	// even if one already exists.  This should only be used for testing!
	// AtExitManagers are kept on a global stack, and it will be removed during
	// destruction.  This allows you to shadow another AtExitManager.
	explicit AtExitManager(bool shadow);

public:
	typedef void (*AtExitCallbackType)(void*);

	AtExitManager();

	// The dtor calls all the registered callbacks. Do not try to register more
	// callbacks after this point.
	~AtExitManager();

	// Registers the specified function to be called at exit. The prototype of
	// the callback function is void func().
	static void RegisterCallback(AtExitCallbackType func, void* param);

	// Calls the functions registered with RegisterCallback in LIFO order. It
	// is possible to register new callbacks after calling this function.
	static void ProcessCallbacksNow();

private:
	struct CallbackAndParam
	{
		CallbackAndParam(AtExitCallbackType func, void* param)
			: m_func(func), m_param(param) { }
		AtExitCallbackType m_func;
		void* m_param;
	};

	Util::Mutex m_lock;
	std::stack<CallbackAndParam> m_stack;
	AtExitManager* m_nextManager;  // Stack of managers to allow shadowing.

	DISALLOW_COPY_AND_ASSIGN(AtExitManager);
};

#if defined(UNIT_TEST)
class ShadowingAtExitManager : public AtExitManager 
{
public:
	ShadowingAtExitManager() : AtExitManager(true) {}
};
#endif  // defined(UNIT_TEST)

}  // namespace Util

#endif	// UTIL_AT_EXIT_MANAGER_H
