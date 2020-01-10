//
// Created by ganler-Mac on 2019-09-30.
//

#pragma once

#include <cstddef>
#include <iostream>
#include <future>
#include <vector>
#include <thread>
#include <unordered_map>

namespace ganler
{

#define ganler_assert(exp)\
{\
    if(!(exp))\
    {\
        std::cerr << "GANLER_ASSERT FAILED in line " << __LINE__ << ":\t" << #exp << '\n';\
        std::exit(-1);\
    }\
}

// Trivial thread-safe ref_counter;
namespace trivial
{

class ref_cnter
{
public:
    ref_cnter() : m_cnt_ptr(new std::atomic<std::size_t>{1}) {}
    ~ref_cnter() noexcept
    {
        if(nullptr != m_cnt_ptr)
            dec();
    }
    ref_cnter(const ref_cnter& cnter) noexcept
    {
        m_cnt_ptr = cnter.m_cnt_ptr;
        if(nullptr != m_cnt_ptr)
            inc();
    }
    ref_cnter(ref_cnter&& cnter) noexcept
    {
        m_cnt_ptr = cnter.m_cnt_ptr;
        cnter.m_cnt_ptr = nullptr;
    }
    ref_cnter& operator=(ref_cnter cnter) noexcept
    {
        std::swap(m_cnt_ptr, cnter.m_cnt_ptr);
        return *this;
    }
private:
    inline void dec_it(std::atomic<std::size_t>* ptr) const noexcept
    {
        if(ptr->fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete ptr;
    }
    void dec() const noexcept
    {
        dec_it(m_cnt_ptr);
    }
    void inc() const noexcept
    {
        m_cnt_ptr->fetch_add(1, std::memory_order_relaxed);
    }
    mutable std::atomic<std::size_t>* m_cnt_ptr;
};

}

// Optimized thread-safe ref_counter;
inline namespace opt
{

class ref_cnter
{
public:
    ref_cnter() :
            m_global_cnt_ptr(new std::atomic<std::size_t>{1})
    {
        m_map[m_global_cnt_ptr] = 1;
    }
    ~ref_cnter()
    {
        if(nullptr != m_global_cnt_ptr)
            dec();
    }
    ref_cnter(const ref_cnter& cnter):
            m_global_cnt_ptr(cnter.m_global_cnt_ptr)
    {// tested fine!
        if(&m_map != &cnter.m_map)
            global_inc();
        local_inc();
    }
    ref_cnter(ref_cnter&& cnter):
            m_global_cnt_ptr(cnter.m_global_cnt_ptr)
    {
        if(&m_map != &cnter.m_map)
        {
            global_inc();
            cnter.dec();
            m_map[m_global_cnt_ptr] = 1;
        }
        cnter.m_global_cnt_ptr = nullptr;
    }
//    ref_cnter& operator=(ref_cnter cnter)
// We cannot just simply do swap work here.(&cnter.m_map may != this.m_map) // !!! Attention.
//    {
//        std::swap(m_global_cnt_ptr, cnter.m_global_cnt_ptr);
//        return *this;
//    }
    ref_cnter& operator=(const ref_cnter& cnter)
    {
        const auto global_fetch = cnter.m_global_cnt_ptr;
        if(m_global_cnt_ptr == global_fetch)   // Totally the SAME.
            return *this;
        auto original = m_global_cnt_ptr;
        m_global_cnt_ptr = global_fetch;
        local_inc();
        if(&m_map != &cnter.m_map)
            global_inc();
        dec_it(original);
        return *this;
    }
    ref_cnter& operator=(ref_cnter&& cnter)
    {
        dec();
        m_global_cnt_ptr = cnter.m_global_cnt_ptr;
        if(&m_map != &cnter.m_map)
        {
            global_inc();
            cnter.dec();
            m_map[m_global_cnt_ptr] = 1;
        }
        cnter.m_global_cnt_ptr = nullptr;
        return *this;
    }
private:
    void local_inc() const
    {
        ++m_map[m_global_cnt_ptr];
    }
    void global_inc() const
    {
        m_global_cnt_ptr->fetch_add(1, std::memory_order_relaxed);
    }
    void dec_it(std::atomic<std::size_t>* ptr) const
    {
        if(--m_map[ptr] == 0)
        {
            m_map.erase(ptr);
            if(ptr->fetch_sub(1, std::memory_order_acq_rel) == 1)
                delete ptr;
        }
    }
    void dec() const
    {
        dec_it(m_global_cnt_ptr);
    }
    inline std::unordered_map<std::atomic<std::size_t>*, int>& get_map() const
    {
        thread_local std::unordered_map<std::atomic<std::size_t>*, int> local_map;
        return local_map;
    }
    mutable std::atomic<std::size_t>* m_global_cnt_ptr;
    std::unordered_map<std::atomic<std::size_t>*, int>& m_map = std::ref(get_map());
    // Inline static thread_local std::unordered_map<std::atomic<std::size_t>*, int> m_map; // map is safe as it's local!
    // The performance of std::unordered_map is related to its impl.
    // (Actually I believe this is more useful when non-local operation is much more expensive than local ones.)
};

}

// Non-thread-safe ref_counter;
namespace unsafe
{

class ref_cnter
{
public:
    ref_cnter() : m_cnt_ptr(new std::size_t(1)) {}
    ~ref_cnter()
    {
        dec();
    }
    ref_cnter(const ref_cnter& cnter) noexcept
    {
        m_cnt_ptr = cnter.m_cnt_ptr;
        inc();
    }
    ref_cnter(ref_cnter&& cnter) noexcept
    {
        m_cnt_ptr = cnter.m_cnt_ptr;
        cnter.m_cnt_ptr = nullptr;
    }
    ref_cnter& operator=(ref_cnter& cnter)
    {
        if(m_cnt_ptr == cnter.m_cnt_ptr)
            return *this;
        cnter.inc();
        dec();
        m_cnt_ptr = cnter.m_cnt_ptr;
        return *this;
    }
    ref_cnter& operator=(ref_cnter&& cnter)
    {
        dec();
        m_cnt_ptr = cnter.m_cnt_ptr;
        cnter.m_cnt_ptr = nullptr;
        return *this;
    }
private:
    void dec() const
    {
        if(0 == --(*m_cnt_ptr))
            delete m_cnt_ptr;
    }
    void inc() const noexcept
    {
        ++(*m_cnt_ptr);
    }
    mutable std::size_t* m_cnt_ptr = nullptr;
};

}

}