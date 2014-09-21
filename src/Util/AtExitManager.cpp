// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Util/AtExitManager.h>
#include <Logging/Logger.h>

namespace Util
{

// Keep a stack of registered AtExitManagers.  We always operate on the most
// recent, and we should never have more than one outside of testing, when we
// use the shadow version of the constructor.  We don't protect this for
// thread-safe access, since it will only be modified in testing.
static AtExitManager* gTopManager = NULL;

AtExitManager::AtExitManager() : m_nextManager(NULL) 
{
	UTIL_DCHECK(!gTopManager);
	gTopManager = this;
}

AtExitManager::AtExitManager(bool shadow) : m_nextManager(gTopManager)
{
	UTIL_DCHECK(shadow || !gTopManager);
	gTopManager = this;
}

AtExitManager::~AtExitManager() 
{
	if (!gTopManager)
	{
		NOTREACHED() << "Tried to ~AtExitManager without an AtExitManager";
		return;
	}
	UTIL_DCHECK(gTopManager == this);

	ProcessCallbacksNow();
	gTopManager = m_nextManager;
}

// static
void AtExitManager::RegisterCallback(AtExitCallbackType func, void* param)
{
	if (!gTopManager)
	{
		NOTREACHED() << "Tried to RegisterCallback without an AtExitManager";
		return;
	}

	UTIL_DCHECK(func);

	Util::Mutex::LockGuard sync(gTopManager->m_lock);
	gTopManager->m_stack.push(CallbackAndParam(func, param));
}

// static
void AtExitManager::ProcessCallbacksNow() 
{
	if (!gTopManager)
	{
		NOTREACHED() << "Tried to ProcessCallbacksNow without an AtExitManager";
		return;
	}

	Util::Mutex::LockGuard sync(gTopManager->m_lock);

	while (!gTopManager->m_stack.empty())
	{
		CallbackAndParam callback_and_param = gTopManager->m_stack.top();
		gTopManager->m_stack.pop();

		callback_and_param.m_func(callback_and_param.m_param);
	}
}

}  // namespace Util
