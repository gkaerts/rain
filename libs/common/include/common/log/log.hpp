#pragma once

#include "common/common.hpp"

#ifndef RN_LOGGING_DISABLED
    #define RN_LOGGING_DISABLED 0
#endif

#if !RN_LOGGING_DISABLED
    #include "spdlog/spdlog.h"
#endif

#define RN_REMINDME(msgLiteral) { static bool haveIBeenReminded = false; if (!haveIBeenReminded) { rn::LogError(rn::LogCategory::Default, "REMINDME: " msgLiteral); haveIBeenReminded = true; } }

namespace rn
{
    enum class LogCategoryID : uint8_t {};

    LogCategoryID AllocateLogCategoryID(const char* nameLiteral);
    uint8_t NumLogCategories();

    #define RN_DEFINE_LOG_CATEGORY(name) \
        namespace LogCategory \
        {   \
            extern const rn::LogCategoryID name = rn::AllocateLogCategoryID(#name); \
        }

    #define RN_LOG_CATEGORY(name) \
        namespace LogCategory \
        { \
            extern const LogCategoryID name; \
        }

    const char* LogCategoryName(LogCategoryID cat);

    struct LoggerSettings
    {
        struct
        {
            const char* logFilename;
        } desktop;
    };

    RN_LOG_CATEGORY(Default)

    void InitializeLogger(const LoggerSettings& settings);
    void TeardownLogger();

    template <typename... Args> void LogInfo(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args);
    template <typename... Args> void LogWarning(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args);
    template <typename... Args> void LogError(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args);

    #if !RN_LOGGING_DISABLED
        namespace detail
        {
            spdlog::logger* GetSpdLogger(LogCategoryID cat);
        }

        template <typename... Args> void LogInfo(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::logger* logger = detail::GetSpdLogger(cat);
            if (logger) { logger->info(fmt, std::forward<Args>(args)...); }
        }

        template <typename... Args> void LogWarning(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::logger* logger = detail::GetSpdLogger(cat);
            if (logger) { logger->warn(fmt, std::forward<Args>(args)...); }
        }

        template <typename... Args> void LogError(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            spdlog::logger* logger = detail::GetSpdLogger(cat);
            if (logger) { logger->error(fmt, std::forward<Args>(args)...); }
        }

    #else
        template <typename... Args> void LogInfo(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args) {}
        template <typename... Args> void LogWarning(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args) {}
        template <typename... Args> void LogError(LogCategoryID cat, spdlog::format_string_t<Args...> fmt, Args&&... args) {}

    #endif
}