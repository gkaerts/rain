#pragma once

#include "common/common.hpp"
#include <span>
#include <initializer_list>

namespace rn
{
    template <typename T, std::size_t Extent = std::dynamic_extent>
    class Span : public std::span<T, Extent>
    {
    public:
        static constexpr std::size_t extent = std::span<T, Extent>::extent;
        using size_type = std::span<T, Extent>::size_type;
        using element_type = std::span<T, Extent>::element_type;
        using value_type = std::span<T, Extent>::value_type;
        
        constexpr Span() noexcept
         : std::span<T, Extent>()
        {}


        explicit(extent != std::dynamic_extent)
        constexpr Span(std::initializer_list<value_type> li) noexcept
         : std::span<T, Extent>(li.begin(), li.end())
        {}

        template< class It >
        explicit(extent != std::dynamic_extent)
        constexpr Span(It first, size_type count)
            : std::span<T, Extent>(first, count)
        {}

        template< class It, class End >
        explicit(extent != std::dynamic_extent)
        constexpr Span( It first, End last )
            : std::span<T, Extent>(first, last)
        {}

        template< std::size_t N >
        constexpr Span( std::type_identity_t<element_type> (&arr)[N] ) noexcept
            : std::span<T, Extent>(arr)
        {}

        template< class U, std::size_t N >
        constexpr Span( std::array<U, N>& arr ) noexcept
            : std::span<T, Extent>(arr)
        {}

        template< class U, std::size_t N >
        constexpr Span( const std::array<U, N>& arr ) noexcept
            : std::span<T, Extent>(arr)
        {}

        template< class R >
        explicit(extent != std::dynamic_extent)
        constexpr Span( R&& range )
            : std::span<T, Extent>(std::forward<R>(range))
        {}

        template< class U, std::size_t N >
        explicit(extent != std::dynamic_extent && N == std::dynamic_extent)
        constexpr Span( const std::span<U, N>& source ) noexcept
            : std::span<T, Extent>(source)
        {}

        constexpr Span( const Span& other ) noexcept = default;
        
    };
}