#ifndef _STATIC_SAMPLERS_H_
#define _STATIC_SAMPLERS_H_

#if defined(__cplusplus)
    #define RN_DESCRIBE_STATIC_SAMPLER(name, filter, addressU, addressV, addressW, reg) DescribeStaticSampler(reg, D3D12_FILTER_##filter, D3D12_TEXTURE_ADDRESS_MODE_##addressU, D3D12_TEXTURE_ADDRESS_MODE_##addressV, D3D12_TEXTURE_ADDRESS_MODE_##addressW),

#else
    #define RN_VK_SAMPLER_DESCRIPTOR_SET_INDEX 1
    #define RN_DESCRIBE_STATIC_SAMPLER(reg, filter, addressU, addressV, addressW) [[vk::binding(reg, RN_VK_SAMPLER_DESCRIPTOR_SET_INDEX)]] SamplerState name : register(s##reg);

#endif // !defined(__cplusplus)

RN_DESCRIBE_STATIC_SAMPLER(PointClamp,          MIN_MAG_MIP_POINT,          CLAMP,  CLAMP,  CLAMP,  0)
RN_DESCRIBE_STATIC_SAMPLER(PointWrap,           MIN_MAG_MIP_POINT,          WRAP,   WRAP,   WRAP,   1)
RN_DESCRIBE_STATIC_SAMPLER(PointBorder,         MIN_MAG_MIP_POINT,          BORDER, BORDER, BORDER, 2)
RN_DESCRIBE_STATIC_SAMPLER(BilinearClamp,       MIN_MAG_LINEAR_MIP_POINT,   CLAMP,  CLAMP,  CLAMP,  3)
RN_DESCRIBE_STATIC_SAMPLER(BilinearWrap,        MIN_MAG_LINEAR_MIP_POINT,   WRAP,   WRAP,   WRAP,   4)
RN_DESCRIBE_STATIC_SAMPLER(BilinearBorder,      MIN_MAG_LINEAR_MIP_POINT,   BORDER, BORDER, BORDER, 5)
RN_DESCRIBE_STATIC_SAMPLER(TrilinearClamp,      MIN_MAG_MIP_LINEAR,         CLAMP,  CLAMP,  CLAMP,  6)
RN_DESCRIBE_STATIC_SAMPLER(TrilinearWrap,       MIN_MAG_MIP_LINEAR,         WRAP,   WRAP,   WRAP,   7)
RN_DESCRIBE_STATIC_SAMPLER(TrilinearBorder,     MIN_MAG_MIP_LINEAR,         BORDER, BORDER, BORDER, 8)
RN_DESCRIBE_STATIC_SAMPLER(AnisotropicClamp,    ANISOTROPIC,                CLAMP,  CLAMP,  CLAMP,  9)
RN_DESCRIBE_STATIC_SAMPLER(AnisotropicWrap,     ANISOTROPIC,                WRAP,   WRAP,   WRAP,   10)
RN_DESCRIBE_STATIC_SAMPLER(AnisotropicBorder,   ANISOTROPIC,                BORDER, BORDER, BORDER, 11)

#undef RN_DESCRIBE_STATIC_SAMPLER
#endif // _STATIC_SAMPLERS_H_