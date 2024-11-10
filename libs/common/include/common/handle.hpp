#pragma once

#include "common/common.hpp"

#define RN_DEFINE_HANDLE(name, salt) enum class name : uint64_t { \
        Invalid = 0, \
        Salt = (uint64_t(salt) << 56), \
        SaltMask = uint64_t(0xFF) << 56, \
        GenerationMask = uint64_t(0xFFFF) << 40, \
        IndexMask = ~(SaltMask | GenerationMask) };

#define RN_HANDLE(name) enum class name : uint64_t;

namespace rn
{
    template <typename HandleType> bool IsValid(HandleType h) { return (h != HandleType::Invalid) && (HandleType(uint64_t(h) & uint64_t(HandleType::SaltMask)) == HandleType::Salt); }
    template <typename HandleType> uint64_t IndexFromHandle(HandleType h) { return (uint64_t(h) & uint64_t(HandleType::IndexMask)); }
    template <typename HandleType> uint16_t GenerationFromHandle(HandleType h) { return uint16_t((uint64_t(h) & uint64_t(HandleType::GenerationMask)) >> 40); }
    template <typename HandleType> HandleType AssembleHandle(uint64_t index, uint8_t generation) { return HandleType(uint64_t(HandleType::Salt) | (uint64_t(generation) << 40) | (index & uint64_t(HandleType::IndexMask))); }
}