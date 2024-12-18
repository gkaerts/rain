#ifndef _STRING_HLSLI_
#define _STRING_HLSLI_

#define RN_SHADER_STRING_SHORT(c0, c1, c2, c3) (((c3 & 0xFF) << 24) | ((c2 & 0xFF) << 16) | ((c1 & 0xFF) << 8) | (c0 & 0xFF))
#define RN_SHADER_STRING(c0, c1, c2, c3, c4, c5, c6, c7) uint2(RN_SHADER_STRING_SHORT(c0, c1, c2, c3), RN_SHADER_STRING_SHORT(c4, c5, c6, c7))
#define RN_SHADER_STRING_LONG(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15) uint4(RN_SHADER_STRING_SHORT(c0, c1, c2, c3), RN_SHADER_STRING_SHORT(c4, c5, c6, c7), RN_SHADER_STRING_SHORT(c8, c9, c10, c11), RN_SHADER_STRING_SHORT(c12, c13, c14, c15))

#endif // _STRING_HLSLI_