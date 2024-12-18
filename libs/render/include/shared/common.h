#ifndef _COMMON_H_
#define _COMMON_H_

#if defined(__cplusplus)
    #include "common/handle.hpp"
    #include "common/math/hlsl_compat.hpp"
#endif

typedef uint ShaderResource;

static const uint INVALID_SHADER_RESOURCE = 0xFFFFFFFF;

struct IndirectDrawArgs
{
    uint vertexCount;
    uint instanceCount;
    uint startVertexLocation;
    uint startInstanceLocation;
};

struct IndirectIndexedDrawArgs
{
    uint indexCount;
    uint instanceCount;
    uint startIndexLocation;
    uint baseVertexLocation;
    uint startInstanceLocation;
};

struct IndirectDispatchArgs
{
    uint threadGroupCountX;
    uint threadGroupCountY;
    uint threadGroupCountZ;
};


#if defined (__cplusplus)

#define BEGIN_NAMESPACE namespace rn {
#define END_NAMESPACE }
#define RN_INLINE_SHARED inline
#define RN_CALL_SHARED __vectorcall
#define row_major
#define column_major

namespace rn::rhi
{
    template <typename HandleType> ShaderResource ToShaderResource(HandleType h) { RN_ASSERT(IsValid(h)); return rn::IndexFromHandle<HandleType>(h); }
}

#else // !defined(__cplusplus)
#define BEGIN_NAMESPACE
#define END_NAMESPACE
#define RN_INLINE_SHARED
#define RN_CALL_SHARED
#define static_assert(x)

#endif

#endif // _COMMON_H_