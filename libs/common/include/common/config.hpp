#pragma once

// Compiler defines
#if defined(__clang__)
    #define RN_COMPILER_CLANG 1
    #define RN_COMPILER_NAME "Clang"

#elif defined (_MSC_VER)
    #define RN_COMPILER_MSVC 1
    #define RN_COMPILER_NAME "MSVC"

#else
    #error Unsupported compiler detected!
#endif

#if !defined (RN_COMPILER_CLANG)
    #define RN_COMPILER_CLANG 0
#endif

#if !defined (RN_COMPILER_MSVC)
    #define RN_COMPILER_MSVC 0
#endif

// Platform defines
#if defined(_WIN32) || defined(_WIN64)
    #define RN_PLATFORM_WINDOWS 1
    #define RN_PLATFORM_DESKTOP 1
    #define RN_PLATFORM_NAME "Windows"

#else
    #error Unsupported platform
#endif

#if !defined(RN_PLATFORM_WINDOWS)
    #define RN_PLATFORM_WINDOWS 0
#endif

#if !defined(RN_PLATFORM_DESKTOP)
    #define RN_PLATFORM_DESKTOP 0
#endif