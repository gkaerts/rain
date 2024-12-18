#pragma once

#include "common/common.hpp"
#include <type_traits>

namespace rn
{
    // Construct compile-time string using __FUNCSIG__, do FNV hash, pour into constexpr TypeID
    namespace detail
    {
        constexpr const uint64_t FNV_BASIS = 14695981039346656037ull;
        constexpr uint64_t FNV1a(size_t length, const char* str, size_t seed = FNV_BASIS)
        {
            constexpr const uint64_t FNV_PRIME = 1099511628211ull;
            return length > 0 ? FNV1a(length - 1, str + 1, (seed ^ *str) * FNV_PRIME) : seed;
        }

        template <size_t N>
        constexpr uint64_t FNV1a(const char(&str)[N])
        {
            return FNV1a(N - 1, str);
        }

        template <typename T>
        constexpr uint64_t TypeIDDirect()
        {
            return detail::FNV1a(__FUNCSIG__);
        }
    }

    template <typename T>
    constexpr uint64_t TypeID()
    {
        return detail::TypeIDDirect<std::remove_cvref_t<T>>();
    }
}