#pragma once
#include "Types.hpp"
#include <array>
#include <vector>
#include <initializer_list>

template <typename T>
class ArrayProxy
{
public:
    inline constexpr ArrayProxy()
        : m_count{ 0 }
        , m_ptr{ nullptr }
    {}

    inline constexpr ArrayProxy(std::nullptr_t)
        : m_count{ 0 }
        , m_ptr{ nullptr }
    {}

    inline constexpr ArrayProxy(T const& value)
        : m_count{ 1 }
        , m_ptr{ &value }
    {}

    inline constexpr ArrayProxy(u32 count, T const* ptr)
        : m_count{ count }
        , m_ptr{ ptr }
    {}

    template<std::size_t C>
    inline constexpr ArrayProxy(T const (&ptr)[C])
        : m_count{ C }
        , m_ptr{ ptr }
    {}

    inline constexpr ArrayProxy(std::initializer_list<T> const& list)
        : m_count{ static_cast<u32>(list.size()) }
        , m_ptr{ list.begin() }
    {}

    template<typename B = T, typename std::enable_if<std::is_const<B>::value, int>::type = 0>
    inline constexpr ArrayProxy(std::initializer_list<typename std::remove_const<T>::type> const& list)
        : m_count{ static_cast<u32>(list.size()) }
        , m_ptr{ list.begin() }
    {}

    template<typename V,
                typename std::enable_if<std::is_convertible<decltype(std::declval<V>().data()), T*>::value&&
                                        std::is_convertible<decltype(std::declval<V>().size()), std::size_t>::value>::type* = nullptr>
    inline constexpr ArrayProxy(V const& v)
        : m_count{ static_cast<u32>(v.size()) }
        , m_ptr{ v.data() }
    {}

    inline auto begin() const -> T const*
    {
        return m_ptr;
    }

    inline auto end() const -> T const*
    {
        return m_ptr + m_count;
    }

    inline auto front() const -> T const&
    {
        static_assert(m_count && m_ptr);
        return *m_ptr;
    }

    inline auto back() const -> T const&
    {
        static_assert(m_count && m_ptr);
        return *(m_ptr + m_count - 1);
    }

    inline auto empty() const -> bool
    {
        return m_count == 0;
    }

    inline auto size() const -> u32
    {
        return m_count;
    }

    inline auto data() const -> T const*
    {
        return m_ptr;
    }

    inline auto operator[](u32 c) const -> T const&
    {
        return m_ptr[c];
    }

private:
    const u32 m_count;
    const T*  m_ptr;
};