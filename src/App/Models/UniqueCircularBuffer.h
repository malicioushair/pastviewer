#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <utility>

template <typename T, typename ID, size_t CAPACITY, typename KeyOfFn>
class UniqueCircularBuffer
{
public:
    UniqueCircularBuffer(KeyOfFn f)
        : m_getKeyOf(std::move(f))
    {
    }

    size_t Size() const
    {
        return m_size;
    }

    const T & At(size_t idx) const
    {
        if (m_size == 0)
            throw std::out_of_range("Attempt to access item of an empty UniqueCircularBuffer");
        else if (idx >= m_size)
            throw std::out_of_range("Attempt to access item outside of UniqueCircularBuffer bounds");
        return m_data.at((m_tail + idx) % CAPACITY);
    }

    T & At(size_t idx)
    {
        return const_cast<T &>(std::as_const(*this).At(idx));
    }

    void Push(const T & item)
    {
        Insert(item, EvictionPolicy::Allow);
    }

    void Push(T && item)
    {
        Insert(std::move(item), EvictionPolicy::Allow);
    }

    template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_reference_t<Range>, T>
    void Push(Range && newItems)
    {
        for (auto && item : newItems)
            Push(std::forward<decltype(item)>(item));
    }

    T Pop()
    {
        if (m_size == 0)
            throw std::out_of_range("UniqueCircularBuffer is empty");
        T res = m_data[m_tail++ % CAPACITY];
        const auto id = std::invoke(m_getKeyOf, res);
        m_keys.erase(id);
        --m_size;
        return res;
    }

    bool IsFull() const
    {
        return m_size == CAPACITY;
    }

    void Clear()
    {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
        m_keys.clear();
    }

    bool IsEmpty()
    {
        return m_size < 1;
    }

    bool Contains(const T & item)
    {
        const auto id = std::invoke(m_getKeyOf, item);
        return m_keys.contains(id);
    }

    void AppendWithoutEviction(const T & item)
    {
        Insert(item, EvictionPolicy::Forbid);
    }

    void AppendWithoutEviction(T && item)
    {
        Insert(std::move(item), EvictionPolicy::Forbid);
    }

    template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_reference_t<Range>, T>
    void AppendWithoutEviction(Range && newItems)
    {
        for (auto && item : newItems)
            AppendWithoutEviction(std::forward<decltype(item)>(item));
    }

    size_t Capacity()
    {
        return CAPACITY;
    }

    auto begin()
    {
        return m_data.begin();
    }

    auto cbegin() const
    {
        return m_data.cbegin();
    }

    auto end()
    {
        return m_data.end();
    }

    auto cend() const
    {
        return m_data.cend();
    }

private:
    enum class EvictionPolicy { Allow, Forbid };

    template <typename Item>
    void Insert(Item && item, EvictionPolicy evictionPolicy)
    {
        const auto id = std::invoke(m_getKeyOf, item);
        if (m_keys.contains(id))
            return;

        if (m_size >= CAPACITY)
        {
            if (evictionPolicy == EvictionPolicy::Forbid)
                throw std::logic_error("No eviction allowed.");
            (void)Pop();
        }

        m_data[m_head % CAPACITY] = std::forward<Item>(item);
        ++m_head;
        m_keys.insert(id);
        ++m_size;
    }

    std::array<T, CAPACITY> m_data;
    std::unordered_set<ID> m_keys;
    KeyOfFn m_getKeyOf;
    size_t m_head { 0 };
    size_t m_tail { 0 };
    size_t m_size { 0 };
};
