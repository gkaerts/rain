#ifndef _MATERIAL_SHADER_INCLUDE_HLSLI_
#define _MATERIAL_SHADER_INCLUDE_HLSLI_

// Ok, I know this is incredibly dumb, but it's the only sane way I could guarantee this structure to show up in some sort of usable reflection data
#define ExportMaterialType(type) \
    ConstantBuffer<type> __dummyMaterialValue; \
    RWStructuredBuffer<type> __dummyStructuredBuffer; \
    [numthreads(1,1,1)] void __material_export_main() { __dummyStructuredBuffer[0] = __dummyMaterialValue; }



#endif // _MATERIAL_SHADER_INCLUDE_HLSLI_