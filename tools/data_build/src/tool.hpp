#pragma once

#include "common/common.hpp"
#include "common/memory/span.hpp"
#include <string_view>
#include <iostream>

using namespace std::literals;

namespace rn
{
    struct Tool
    {
        std::string_view name;
        std::string_view additionalUsageText;
    };

    template <typename ArgType>
    using FnOptionFound = void(*)(ArgType& args, std::string_view param);

    template <typename ArgType>
    struct ToolOption
    {
        std::string_view option;
        std::string_view parameter;
        std::string_view description;
        FnOptionFound<ArgType> onOptionFound = nullptr;
    };

    template <typename ArgType>
    void PrintUsage(const Tool& tool, Span<const ToolOption<ArgType>> options)
    {
        std::cout << "Usage: " << tool.name << " [OPTION]... FILE" << std::endl;

        for (const ToolOption<ArgType>& option : options)
        {
            std::cout << "-" << option.option << " " << option.parameter << "\t\t" << option.description << std::endl;
        }
        std::cout << std::endl;
        std::cout << tool.additionalUsageText << std::endl;
    }

    template <typename ArgType>
    bool ParseArgs(const Tool& tool, ArgType& outArgs, std::string_view& outInput, Span<const ToolOption<ArgType>> options, int argc, char* argv[])
    {
        if (argc <= 1)
        {
            std::cerr << "ERROR: No arguments provided" << std::endl;
            PrintUsage(tool, options);
            return false;
        }

        for (int i = 1; i < argc; ++i)
        {
            std::string_view arg = argv[i];
            if (arg[0] == '-')
            {
                bool optionFound = false;
                for (const ToolOption<ArgType>& option : options)
                {
                    if (option.option == arg.substr(1))
                    {
                        std::string_view param = ""sv;
                        if (!option.parameter.empty())
                        {
                            if (i == argc - 1)
                            {
                                std::cerr << "ERROR: No parameter provided for option: " << arg << std::endl;
                                return false;
                            }
                            
                            param = argv[++i];
                        }

                        option.onOptionFound(outArgs, param);
                        optionFound = true;
                        break;
                    }
                }

                if (!optionFound)
                {
                    std::cerr << "ERROR: Unrecognized option: " << arg << std::endl;
                    return false;
                }
            }
            else if (i < argc - 1)
            {
                std::cerr << "ERROR: Unrecognized argument: \"" << arg << "\"" << std::endl;
                return false;
            }
            else
            {
                outInput = arg;
            }
        }

        return true;
    }
}