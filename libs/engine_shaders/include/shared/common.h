#ifndef _COMMON_H_
#define _COMMON_H_

#if defined(__cplusplus)
    #include "common/handle.hpp"
    #include "common/math/math.hpp"
#endif

typedef uint ShaderResource;

#if defined (__cplusplus)
namespace rn::rhi
{
    template <typename HandleType> ShaderResource ToShaderResource(HandleType h) { return rn::IndexFromHandle<HandleType>(h); }
}

#else // !defined(__cplusplus)
#define static_assert(x)

#endif

#endif // _COMMON_H_