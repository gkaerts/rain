#include "common/log/log.hpp"
#include "common/memory/memory.hpp"

#if !RN_LOGGING_DISABLED
    #if RN_PLATFORM_WINDOWS
        #include "spdlog/sinks/stdout_color_sinks.h"
        #include "spdlog/sinks/basic_file_sink.h"
    #else

        #error No logging sinks provided for platform!
    #endif
#endif

namespace rn
{
#if RN_LOGGING_DISABLED

    void InitializeLogger(const LoggerSettings& settings) {}
    void TeardownLogger() {}

#else

    RN_DEFINE_LOG_CATEGORY(Default)

    namespace
    {
        constexpr const uint8_t MAX_LOG_CATEGORY_COUNT = UINT8_MAX;
        const char* LOG_CATEGORY_NAMES[MAX_LOG_CATEGORY_COUNT] = {};

        uint8_t& LogCategoryCounter()
        {
            static uint8_t categoryCounter = 0;
            return categoryCounter;
        }
    }

    LogCategoryID AllocateLogCategoryID(const char* nameLiteral)
    {
        uint8_t& counter = LogCategoryCounter();

        RN_ASSERT(counter < MAX_LOG_CATEGORY_COUNT);

        const uint8_t categoryID = counter++;
        LOG_CATEGORY_NAMES[categoryID] = nameLiteral;

        return LogCategoryID(categoryID);
    }

    uint8_t NumLogCategories()
    {
        return LogCategoryCounter();
    }

    namespace
    {
        bool IsValidLogCategory(LogCategoryID cat)
        {
            return uint8_t(cat) < NumLogCategories();
        }

        class Logger
        {
            public:

                Logger(const LoggerSettings& settings)
                {
                    #if RN_PLATFORM_WINDOWS
                        consoleSink = MakeSharedTracked<spdlog::sinks::stdout_color_sink_mt>(MemoryCategory::Default);
                        std::initializer_list<spdlog::sink_ptr> sinks{ consoleSink };

                        if (settings.desktop.logFilename)
                        {
                            fileSink = MakeSharedTracked<spdlog::sinks::basic_file_sink_mt>(MemoryCategory::Default, settings.desktop.logFilename, false);
                            sinks = { consoleSink, fileSink };
                        }
                    #else
                        #error No logging sinks implemented for platform
                    #endif

                    const uint8_t numCategories = NumLogCategories();
                    for (uint8_t cat = 0; cat < numCategories; ++cat)
                    {
                        categoryLoggers[cat] = MakeSharedTracked<spdlog::logger>(MemoryCategory::Default, LOG_CATEGORY_NAMES[cat], sinks);
                    }
                }

                Logger(const Logger&) = delete;
                Logger& operator=(const Logger&) = delete;

                spdlog::logger* GetLoggerForCategory(LogCategoryID cat)
                {
                    RN_ASSERT(IsValidLogCategory(cat));
                    return categoryLoggers[uint8_t(cat)].get();
                }

            private:

            #if RN_PLATFORM_WINDOWS
			    TrackedSharedPtr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
			    TrackedSharedPtr<spdlog::sinks::basic_file_sink_mt> fileSink;
            #else
            
                #error No logging sinks provided for platform!
			#endif

                TrackedSharedPtr<spdlog::logger> categoryLoggers[MAX_LOG_CATEGORY_COUNT];
        };

        Logger* LOGGER = nullptr;
    }

    namespace detail
    {
        spdlog::logger* GetSpdLogger(LogCategoryID cat)
        {
            spdlog::logger* categoryLogger = nullptr;
            if (LOGGER)
            {
                categoryLogger = LOGGER->GetLoggerForCategory(cat);
            }

            return categoryLogger;
        }
    }

    void InitializeLogger(const LoggerSettings& settings)
    {
        if (!LOGGER)
        {
            LOGGER = TrackedNew<Logger>(MemoryCategory::Default, settings);
        }
    }

    void TeardownLogger()
    {
        if (LOGGER)
        {
            TrackedDelete(LOGGER);
            LOGGER = nullptr;
        }
    }

#endif
}