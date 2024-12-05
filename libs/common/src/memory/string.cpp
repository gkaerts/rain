#include "common/memory/string.hpp"
#include "murmurhash3/MurmurHash3.h"

#include <cstring>

namespace rn
{
    RN_DEFINE_MEMORY_CATEGORY(String)
    namespace
    {
        constexpr const uint32_t StringHash_SEED = 0xAE83B98E;
    };

    StringHash HashString(const char* ptr, size_t length)
    {
        StringHash hash = 0;
        MurmurHash3_x86_32(ptr, int(length * sizeof(char)), StringHash_SEED, &hash);
        return hash;
    }

    StringHash HashString(const wchar_t* ptr, size_t length)
    {
        StringHash hash = 0;
        MurmurHash3_x86_32(ptr, int(length * sizeof(wchar_t)), StringHash_SEED, &hash);
        return hash;
    }

    StringHash HashString(const char* ptr)
    {
        return HashString(ptr, strlen(ptr));
    }

    StringHash HashString(const wchar_t* ptr)
    {
        return HashString(ptr, wcslen(ptr));
    }

    StringHash HashString(const String& str)
    {
        return HashString(str.data(), str.length());
    }

    StringHash HashString(const WString& str)
    {
        return HashString(str.data(), str.length());
    }

    StringHash HashString(const std::string_view& str)
    {
        return HashString(str.data(), str.length());
    }
}