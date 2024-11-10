#pragma once

#include "common/config.hpp"
#include <cstdint>
#include <cstdio>

#if RN_COMPILER_CLANG
    #include <signal.h>
#endif

// Utility macros
#define RN_UNUSED(x) (void)(x)

#if RN_COMPILER_MSVC
    #define RN_DEBUG_BREAK() __debugbreak()

#elif RN_COMPILER_CLANG
    #define RN_DEBUG_BREAK() raise(SIGTRAP)

#endif

#define RN_ASSERT(x) if (!(x)) { std::printf("ASSERT FAILED: %s", #x); RN_DEBUG_BREAK(); }
#define RN_NOT_IMPLEMENTED() { std::printf("%s is not implemented!", __func__); RN_DEBUG_BREAK(); }

#define RN_MATCH_ENUM_AND_ARRAY(arrayName, enumName) static_assert(sizeof(arrayName) / sizeof(arrayName[0]) == static_cast<size_t>(enumName::Count));
#define RN_ARRAY_SIZE(arrayName) (sizeof(arrayName) / sizeof(arrayName[0]))

// Enum API
#define RN_DEFINE_ENUM_CLASS_BITWISE_OR(enumName) inline constexpr enumName operator|(enumName a, enumName b) { \
    return static_cast<enumName>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b)); \
}

#define RN_DEFINE_ENUM_CLASS_BITWISE_OR_ASSIGN(enumName) inline constexpr enumName& operator|=(enumName& a, enumName b) { \
    a = a | b; return a; \
}

#define RN_DEFINE_ENUM_CLASS_BITWISE_AND(enumName) inline constexpr enumName operator&(enumName a, enumName b) { \
    return static_cast<enumName>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b)); \
}

#define RN_DEFINE_ENUM_CLASS_BITWISE_AND_ASSIGN(enumName) inline constexpr enumName& operator&=(enumName& a, enumName b) { \
    a = a & b; return a; \
}

#define RN_DEFINE_ENUM_CLASS_BITWISE_NOT(enumName) inline constexpr enumName operator~(enumName a) { \
    return static_cast<enumName>(~static_cast<uint64_t>(a)); \
}

#define RN_DEFINE_ENUM_CLASS_BOOL(enumName) inline constexpr bool operator!(enumName a) { \
    return a == enumName::None; \
}

#define RN_DEFINE_ENUM_CLASS_TEST_FLAG(enumName) inline constexpr bool TestFlag(enumName a, enumName b) { \
    return (a & b) == b; \
}

#define RN_DEFINE_ENUM_CLASS_BITWISE_API(enumName) \
    RN_DEFINE_ENUM_CLASS_BITWISE_OR(enumName) \
    RN_DEFINE_ENUM_CLASS_BITWISE_OR_ASSIGN(enumName) \
    RN_DEFINE_ENUM_CLASS_BITWISE_AND(enumName) \
    RN_DEFINE_ENUM_CLASS_BITWISE_AND_ASSIGN(enumName) \
    RN_DEFINE_ENUM_CLASS_BITWISE_NOT(enumName) \
    RN_DEFINE_ENUM_CLASS_BOOL(enumName) \
    RN_DEFINE_ENUM_CLASS_TEST_FLAG(enumName)

namespace rn
{
    constexpr const size_t KILO = 1024;
    constexpr const size_t MEGA = KILO * KILO;
    constexpr const size_t GIGA = MEGA * KILO;

    constexpr const size_t CACHE_LINE_TARGET_SIZE = 64;

    inline bool IsPowerOfTwo(size_t size)
    {
        return (size & (size - 1)) == 0;
    }

    inline void* AlignPtr(void* ptr, size_t alignment)
    {
        std::uintptr_t uPtr = reinterpret_cast<std::uintptr_t>(ptr);
        std::uintptr_t alignDiff = uPtr % alignment;
        
        return (alignDiff > 0) ? 
            reinterpret_cast<void*>(uPtr + alignment - alignDiff) :
            ptr;
    }

    inline size_t AlignSize(size_t size, size_t alignment)
    {
        size_t alignDiff = size % alignment;
        return (alignDiff > 0) ? size + alignment - alignDiff : size;
    }
}