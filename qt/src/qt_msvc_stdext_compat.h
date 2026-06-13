// MSVC compatibility shim for Qt 6.4's QVarLengthArray.
//
// Visual Studio 2022 17.10 (_MSC_VER 1940) removed the long-deprecated
// stdext::checked_array_iterator / unchecked_array_iterator from the STL.
// Qt 6.4's qvarlengtharray.h still references them, so any translation
// unit that instantiates QVarLengthArray fails to compile against the
// modern toolset shipped on current windows-latest runners.
//
// This header restores minimal, correct implementations and is
// force-included (cl /FI) ahead of every Qt header on MSVC builds. It is
// a no-op everywhere except VS toolsets that removed the symbols, so
// older MSVC (which still provides them natively) is unaffected.
#pragma once

#if defined(_MSC_VER) && _MSC_VER >= 1940

#include <cstddef>
#include <iterator>

namespace stdext {

template <typename Iterator>
class checked_array_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

    constexpr checked_array_iterator() = default;
    constexpr checked_array_iterator(Iterator array, std::size_t size, std::size_t index = 0)
        : m_array(array), m_size(size), m_index(index) {}

    constexpr reference operator*() const { return m_array[m_index]; }
    constexpr pointer operator->() const { return m_array + m_index; }
    constexpr reference operator[](difference_type n) const { return m_array[m_index + n]; }

    constexpr checked_array_iterator &operator++() { ++m_index; return *this; }
    constexpr checked_array_iterator operator++(int) { auto copy = *this; ++m_index; return copy; }
    constexpr checked_array_iterator &operator--() { --m_index; return *this; }
    constexpr checked_array_iterator operator--(int) { auto copy = *this; --m_index; return copy; }

    constexpr checked_array_iterator &operator+=(difference_type n) { m_index += n; return *this; }
    constexpr checked_array_iterator &operator-=(difference_type n) { m_index -= n; return *this; }
    constexpr checked_array_iterator operator+(difference_type n) const { auto copy = *this; return copy += n; }
    constexpr checked_array_iterator operator-(difference_type n) const { auto copy = *this; return copy -= n; }
    constexpr difference_type operator-(const checked_array_iterator &o) const
    {
        return difference_type(m_index) - difference_type(o.m_index);
    }

    constexpr bool operator==(const checked_array_iterator &o) const { return m_index == o.m_index; }
    constexpr bool operator!=(const checked_array_iterator &o) const { return m_index != o.m_index; }
    constexpr bool operator<(const checked_array_iterator &o) const { return m_index < o.m_index; }
    constexpr bool operator>(const checked_array_iterator &o) const { return m_index > o.m_index; }
    constexpr bool operator<=(const checked_array_iterator &o) const { return m_index <= o.m_index; }
    constexpr bool operator>=(const checked_array_iterator &o) const { return m_index >= o.m_index; }

    constexpr Iterator base() const { return m_array + m_index; }

private:
    Iterator m_array = Iterator();
    std::size_t m_size = 0;
    std::size_t m_index = 0;
};

template <typename Iterator>
class unchecked_array_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;

    constexpr unchecked_array_iterator() = default;
    constexpr explicit unchecked_array_iterator(Iterator ptr) : m_ptr(ptr) {}

    constexpr reference operator*() const { return *m_ptr; }
    constexpr pointer operator->() const { return m_ptr; }
    constexpr reference operator[](difference_type n) const { return m_ptr[n]; }

    constexpr unchecked_array_iterator &operator++() { ++m_ptr; return *this; }
    constexpr unchecked_array_iterator operator++(int) { auto copy = *this; ++m_ptr; return copy; }
    constexpr unchecked_array_iterator &operator--() { --m_ptr; return *this; }
    constexpr unchecked_array_iterator operator--(int) { auto copy = *this; --m_ptr; return copy; }

    constexpr unchecked_array_iterator &operator+=(difference_type n) { m_ptr += n; return *this; }
    constexpr unchecked_array_iterator &operator-=(difference_type n) { m_ptr -= n; return *this; }
    constexpr unchecked_array_iterator operator+(difference_type n) const { auto copy = *this; return copy += n; }
    constexpr unchecked_array_iterator operator-(difference_type n) const { auto copy = *this; return copy -= n; }
    constexpr difference_type operator-(const unchecked_array_iterator &o) const { return m_ptr - o.m_ptr; }

    constexpr bool operator==(const unchecked_array_iterator &o) const { return m_ptr == o.m_ptr; }
    constexpr bool operator!=(const unchecked_array_iterator &o) const { return m_ptr != o.m_ptr; }
    constexpr bool operator<(const unchecked_array_iterator &o) const { return m_ptr < o.m_ptr; }
    constexpr bool operator>(const unchecked_array_iterator &o) const { return m_ptr > o.m_ptr; }
    constexpr bool operator<=(const unchecked_array_iterator &o) const { return m_ptr <= o.m_ptr; }
    constexpr bool operator>=(const unchecked_array_iterator &o) const { return m_ptr >= o.m_ptr; }

    constexpr Iterator base() const { return m_ptr; }

private:
    Iterator m_ptr = Iterator();
};

} // namespace stdext

#endif // _MSC_VER >= 1940
