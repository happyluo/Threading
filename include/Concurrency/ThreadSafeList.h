// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef CONCURRENCY_THREAD_SAFE_LIST_H
#define CONCURRENCY_THREAD_SAFE_LIST_H

//#include <memory>
#include <Concurrency/Mutex.h>
#include <Util/Atomic.h>
#include <Util/UniquePtr.h>
 
THREADING_BEGIN

template<typename T>
class ThreadSafeList
{

public:
    ThreadSafeList()
    {}

    ~ThreadSafeList()
    {
#ifdef LANG_CPP11
        remove_if ([](T const&){return true;});
#else
        remove_all();
#endif
    }

#ifdef LANG_CPP11
    ThreadSafeList(ThreadSafeList const& other) = delete;
    ThreadSafeList& operator=(ThreadSafeList const& other) = delete;
#endif
    
    void push_front(T const& value)
    {
        UniquePtr<Node> new_node(new Node(value));
        Threading::Mutex::LockGuard sync(m_head.m_mutex);
#ifdef LANG_CPP11
        new_node->m_next = std::move(m_head.m_next);
        m_head.m_next = std::move(new_node);
#else
        new_node->m_next = m_head.m_next;
        m_head.m_next = new_node;
#endif
        ++m_size;
    }

    void push_front(SharedPtr<T> const& value)
    {
        UniquePtr<Node> new_node(new Node(value));
        Threading::Mutex::LockGuard sync(m_head.m_mutex);
#ifdef LANG_CPP11
        new_node->m_next = std::move(m_head.m_next);
        m_head.m_next = std::move(new_node);
#else
        new_node->m_next = m_head.m_next;
        m_head.m_next = new_node;
#endif
        ++m_size;
    }

    template<typename Function>
    void for_each(Function fun)
    {
        Node* current = &m_head;
        Threading::Mutex* sync = &m_head.m_mutex;
        sync->Lock();

        while (Node* const next = current->m_next.Get())
        {
            Threading::Mutex* next_sync = &next->m_mutex;
            next_sync->Lock();
            sync->Unlock();
            fun(next->m_data);
            current = next;
            sync = next_sync;
        }

        sync->Unlock();
    }

    template<typename Predicate>
    SharedPtr<T> find_first_if (Predicate p)
    {
        Node* current = &m_head;
        Threading::Mutex* sync = &m_head.m_mutex;
        sync->Lock();

        while (Node* const next = current->m_next.Get())
        {
            Threading::Mutex* next_sync = &next->m_mutex;
            next_sync->Lock();
            sync->Unlock();
            if (p(*next->m_data))
            {
                return next->m_data;
            }
            current = next;
            sync = next_sync;
        }

        sync->Unlock();
        return SharedPtr<T>();
    }

    template<typename Predicate>
    void remove_if (Predicate p)
    {
        Node* current = &m_head;
        Threading::Mutex* sync = &m_head.m_mutex;
        sync->Lock();
        while (Node* const next = current->m_next.Get())
        {
            Threading::Mutex* next_sync = &next->m_mutex;
            next_sync->Lock();

            if (p(*next->m_data))
            {
#ifdef LANG_CPP11
                UniquePtr<Node> old_next = std::move(current->m_next);
                current->m_next = std::move(next->m_next);
#else
                UniquePtr<Node> old_next = current->m_next;
                current->m_next = next->m_next;
#endif
                --m_size;
                next_sync->Unlock();
            }
            else
            {
                sync->Unlock();
                current = next;
                sync = next_sync;
            }
        }

        sync->Unlock();
    }

    void remove(const T* data)
    {
        Node* current = &m_head;
        Threading::Mutex* sync = &m_head.m_mutex;
        sync->Lock();
        while (Node* const next = current->m_next.Get())
        {
            Threading::Mutex* next_sync = &next->m_mutex;
            next_sync->Lock();

            if (data == next->m_data.Get())
            {
#ifdef LANG_CPP11
                UniquePtr<Node> old_next = std::move(current->m_next);
                current->m_next = std::move(next->m_next);
#else
                UniquePtr<Node> old_next = current->m_next;
                current->m_next = next->m_next;
#endif
                --m_size;
                next_sync->Unlock();
            }
            else
            {
                sync->Unlock();
                current = next;
                sync = next_sync;
            }
        }

        sync->Unlock();
    }

    void remove_all()
    {
        Node* current = &m_head;
        Threading::Mutex* sync = &m_head.m_mutex;
        sync->Lock();
        while (Node* const next = current->m_next.Get())
        {
            Threading::Mutex* next_sync = &next->m_mutex;
            next_sync->Lock();
#ifdef LANG_CPP11
            UniquePtr<Node> old_next = std::move(current->m_next);
            current->m_next = std::move(next->m_next);
#else
            UniquePtr<Node> old_next = current->m_next;
            current->m_next = next->m_next;
#endif
            --m_size;
            next_sync->Unlock();
        }

        sync->Unlock();
    }

    int size() const
    {
        return m_size;
    }

private:
    ThreadSafeList(ThreadSafeList const& other);
    ThreadSafeList& operator =(ThreadSafeList const& other);

    struct Node
    {
        Threading::Mutex            m_mutex;
        SharedPtr<T>    m_data;
        UniquePtr<Node> m_next;

        Node() : m_next()
        {}

        Node(T const& value) : m_data(new T(value))
        {}

        Node(SharedPtr<T> const& value) : m_data(value)
        {}
    };

    Node        m_head;
    AtomicInt    m_size;
};

THREADING_END

#endif