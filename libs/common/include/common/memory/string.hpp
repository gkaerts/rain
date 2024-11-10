#pragma once

#include "common/memory/memory.hpp"

#include <string>
#include <filesystem>

namespace rn
{
    RN_MEMORY_CATEGORY(String)

    template <typename T>
    class StringAllocatorSTL : public TrackedAllocatorSTL<T>
    {
        public:

        using size_type = size_t;
        using pointer = T*;
        using const_pointer = const T*;

        template <typename U>
        struct rebind
        {
            using other = StringAllocatorSTL<U>;
        };

        StringAllocatorSTL() throw() : TrackedAllocatorSTL<T>(MemoryCategory::String)
        {}
        
        StringAllocatorSTL(const StringAllocatorSTL& rhs) throw() : TrackedAllocatorSTL<T>(rhs)
        {}

        template <class U>
        StringAllocatorSTL(const StringAllocatorSTL<U>& a) throw() : TrackedAllocatorSTL<T>(a) 
        {}

        ~StringAllocatorSTL() throw() 
        {}
    };

    using String  = std::basic_string<char, std::char_traits<char>, StringAllocatorSTL<char>>;
    using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, StringAllocatorSTL<wchar_t>>;
    using StringHash = size_t;

    inline String PathToString(const std::filesystem::path& p)
    {
        return p.generic_string<char, std::char_traits<char>, StringAllocatorSTL<char>>();
    }

    StringHash HashString(const char* ptr, size_t length);
    StringHash HashString(const wchar_t* ptr, size_t length);
    StringHash HashString(const char* ptr);
    StringHash HashString(const wchar_t* ptr);
    StringHash HashString(const String& str);
    StringHash HashString(const WString& str);

    template <size_t N> StringHash HashString(const char(&ptr)[N]) { return HashString(ptr, N); }
    template <size_t N> StringHash HashString(const wchar_t(&ptr)[N]) { return HashString(ptr, N); }
}