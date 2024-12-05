#pragma once

#include "common/memory/span.hpp"
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <iostream>

namespace rn
{
    namespace internal
    {
        template <typename T>
        struct IsSpan : std::false_type {};

        template <typename T>
        struct IsSpan<Span<T>> : std::true_type {};

        template <typename T>
        struct IsSpan<std::span<T>> : std::true_type {};

        template <typename T>
        constexpr bool IsSpanV = IsSpan<T>::value;

        template <typename T>
        struct IsStringView : std::false_type {};

        template <>
        struct IsStringView<std::string_view> : std::true_type {};

        template <typename T>
        constexpr bool IsStringViewV = IsStringView<T>::value;
    }

    constexpr const uint64_t SERIALIZED_SPAN_SIZE = sizeof(uint64_t);


    template <typename T>
    requires std::is_arithmetic_v<T> || std::is_enum_v<T>
    uint64_t SerializedSize(const T& v)
    {
        return sizeof(T);
    }

    template <typename T>
    requires std::is_aggregate_v<T>
    uint64_t SerializedSize(const T& v)
    {
        return T::SerializedSize(v);
    }

    inline uint64_t SerializedSize(const std::string_view& v)
    {
        return SERIALIZED_SPAN_SIZE + v.size();
    }

    template <typename T>
    requires internal::IsSpanV<T>
    uint64_t SerializedSize(const T& v)
    {
        uint64_t size = SERIALIZED_SPAN_SIZE;
        for (const auto& e : v)
        {
            size += SerializedSize(e);
        }

        return size;
    }


    template <typename T> 
    uint64_t SerializeDirect(const Span<uint8_t> dest, uint64_t& offset, const T& v)
    {
        RN_ASSERT(dest.size_bytes() - offset >= sizeof(T));
        std::memcpy(dest.data() + offset, &v, sizeof(T));
        offset += sizeof(T);
        return sizeof(T);
    }

    template <typename T>
    requires std::is_arithmetic_v<T> || std::is_enum_v<T>
    uint64_t Serialize(const Span<uint8_t> dest, uint64_t& offset, const T& v)
    {
        return SerializeDirect(dest, offset, v);
    }

    template <typename T>
    requires std::is_aggregate_v<T>
    uint64_t Serialize(const Span<uint8_t> dest, uint64_t& offset, const T& v)
    {
        uint64_t val = T::Serialize(v, offset, dest);
        return val;
    }

    inline uint64_t Serialize(const Span<uint8_t> dest, uint64_t& offset, const std::string_view& v)
    {
        // Write where we will find the span/string data, followed by the amount of elements
        uint64_t size = SerializeDirect(dest, offset, uint64_t(v.size()));
        std::memcpy(dest.data() + offset, v.data(), v.size());
        size += v.size();
        offset += v.size();
        return size;
    }

    inline uint64_t Serialize(const Span<uint8_t> dest, uint64_t& offset, const Span<uint8_t>& v)
    {
        // Write where we will find the span/string data, followed by the amount of elements
        uint64_t size = SerializeDirect(dest, offset, uint64_t(v.size()));
        std::memcpy(dest.data() + offset, v.data(), v.size());
        size += v.size();
        offset += v.size();
        return size;
    }

    template <typename T>
    requires internal::IsSpanV<T>
    uint64_t Serialize(const Span<uint8_t> dest, uint64_t& offset, const T& v)
    {
        // Write where we will find the span/string data, followed by the amount of elements
        uint64_t size = SerializeDirect(dest, offset, uint64_t(v.size()));

        // Write elements/characters to the tail   
        for (const auto& e : v)
        {
            uint64_t elementSize = Serialize(dest, offset, e);
            size += elementSize;
        }
        return size;
    }

    template <typename T>
    uint64_t Serialize(const Span<uint8_t> dest, const T& v)
    {
        uint64_t offset = 0;
        return Serialize<T>(dest, offset, v);
    }

    template <typename T> 
    T DeserializeDirect(const Span<const uint8_t> src, uint64_t& offset)
    {
        RN_ASSERT(src.size_bytes() - offset >= sizeof(T));

        T out{};
        std::memcpy(&out, src.data() + offset, sizeof(T));
        offset += sizeof(T);
        return out;
    }

    template <typename T>
    requires std::is_arithmetic_v<T> || std::is_enum_v<T>
    T Deserialize(const Span<const uint8_t> src, uint64_t& offset, void*(*fnAlloc)(size_t))
    {
        return DeserializeDirect<T>(src, offset);
    }

    template <typename T>
    requires std::is_aggregate_v<T>
    T Deserialize(const Span<const uint8_t> src, uint64_t& offset, void*(*fnAlloc)(size_t))
    {
        return T::Deserialize(src, offset, fnAlloc);
    }

    template <typename T>
    requires internal::IsSpanV<T>
    T Deserialize(const Span<const uint8_t> src, uint64_t& offset, void*(*fnAlloc)(size_t))
    {
        // Load where we will find the span data, followed by the amount of elements
        uint64_t spanCount = DeserializeDirect<uint64_t>(src, offset);

        using ValueType = typename T::value_type;
        ValueType* span = static_cast<ValueType*>(fnAlloc(sizeof(ValueType) * spanCount));

        // Deserialize elements/characters
        for (uint64_t i = 0; i < spanCount; ++i)
        {
            new (&span[i]) ValueType;
            span[i] = Deserialize<ValueType>(src, offset, fnAlloc);
        }

        return { span, spanCount };
    }

    template <typename T>
    requires internal::IsStringViewV<T>
    T Deserialize(const Span<const uint8_t> src, uint64_t& offset, void*(*fnAlloc)(size_t))
    {
        // Load where we will find the string data, followed by the amount of elements
        uint64_t strCount = DeserializeDirect<uint64_t>(src, offset);

        char* str = static_cast<char*>(fnAlloc(strCount));
        std::memcpy(str, src.data() + offset, strCount);
        offset += strCount;

        return { str, strCount };
    }

    template <typename T>
    T Deserialize(const Span<const uint8_t> src, void*(*fnAlloc)(size_t))
    {
        uint64_t offset = 0;
        return Deserialize<T>(src, offset, fnAlloc);
    }
}