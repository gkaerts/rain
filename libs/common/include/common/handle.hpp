#pragma once

#include "common/common.hpp"

#define RN_DEFINE_HANDLE(name, salt) enum class name : uint64_t { \
        Invalid = 0, \
        SaltStart = 56, \
        GenerationStart = 48, \
        Salt = (uint64_t(salt) << SaltStart), \
        SaltMask = uint64_t(0xFF) << SaltStart, \
        GenerationMask = uint64_t(0xFF) << GenerationStart, \
        IndexMask = ~(SaltMask | GenerationMask) };

#define RN_DEFINE_SLIM_HANDLE(name, salt) enum class name : uint32_t { \
        Invalid = 0, \
        SaltStart = 24, \
        GenerationStart = 20, \
        Salt = (uint32_t(salt) << SaltStart), \
        SaltMask = uint32_t(0xFF) << SaltStart, \
        GenerationMask = uint32_t(0xF) << GenerationStart, \
        IndexMask = ~(SaltMask | GenerationMask) };

#define RN_HANDLE(name) enum class name : uint64_t;
#define RN_SLIM_HANDLE(name) enum class name : uint32_t;

namespace rn
{
    template <typename HandleType> bool IsValid(HandleType h) { return (h != HandleType::Invalid) && (HandleType(std::underlying_type_t<HandleType>(h) & std::underlying_type_t<HandleType>(HandleType::SaltMask)) == HandleType::Salt); }
    template <typename HandleType> std::underlying_type_t<HandleType> IndexFromHandle(HandleType h) { return (std::underlying_type_t<HandleType>(h) & std::underlying_type_t<HandleType>(HandleType::IndexMask)); }
    template <typename HandleType> uint8_t GenerationFromHandle(HandleType h) { return uint8_t((std::underlying_type_t<HandleType>(h) & std::underlying_type_t<HandleType>(HandleType::GenerationMask)) >> std::underlying_type_t<HandleType>(HandleType::GenerationStart)); }
    template <typename HandleType> HandleType AssembleHandle(std::underlying_type_t<HandleType> index, uint8_t generation) { return HandleType(std::underlying_type_t<HandleType>(HandleType::Salt) | (std::underlying_type_t<HandleType>(generation) << std::underlying_type_t<HandleType>(HandleType::GenerationStart)) | (index & std::underlying_type_t<HandleType>(HandleType::IndexMask))); }
}