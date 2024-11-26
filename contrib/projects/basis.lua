local BASIS_DIR = "%{wks.location}/../../contrib/submodules/basis_universal"
local BASIS_LIBRARY_SRCS = {
    BASIS_DIR .. "/encoder/basisu_backend.cpp",
    BASIS_DIR .. "/encoder/basisu_basis_file.cpp",
    BASIS_DIR .. "/encoder/basisu_comp.cpp",
    BASIS_DIR .. "/encoder/basisu_enc.cpp",
    BASIS_DIR .. "/encoder/basisu_etc.cpp",
    BASIS_DIR .. "/encoder/basisu_frontend.cpp",
    BASIS_DIR .. "/encoder/basisu_gpu_texture.cpp",
    BASIS_DIR .. "/encoder/basisu_pvrtc1_4.cpp",
    BASIS_DIR .. "/encoder/basisu_resampler.cpp",
    BASIS_DIR .. "/encoder/basisu_resample_filters.cpp",
    BASIS_DIR .. "/encoder/basisu_ssim.cpp",
    BASIS_DIR .. "/encoder/basisu_uastc_enc.cpp",
    BASIS_DIR .. "/encoder/basisu_bc7enc.cpp",
    BASIS_DIR .. "/encoder/jpgd.cpp",
    BASIS_DIR .. "/encoder/basisu_kernels_sse.cpp",
    BASIS_DIR .. "/encoder/basisu_opencl.cpp",
    BASIS_DIR .. "/encoder/pvpngreader.cpp",
    BASIS_DIR .. "/encoder/basisu_astc_hdr_enc.cpp",
	BASIS_DIR .. "/encoder/3rdparty/android_astc_decomp.cpp",
    BASIS_DIR .. "/encoder/3rdparty/tinyexr.cpp",
    BASIS_DIR .. "/transcoder/basisu_transcoder.cpp",
    BASIS_DIR .. "/zstd/zstd.c"
}

local BASISU_SRCS = {
    BASIS_DIR .. "/basisu_tool.cpp"
}

local BASIS_DEFINES = {
    "BASISU_SUPPORT_SSE=1",
    "BASISU_SUPPORT_KTX2_ZSTD=1"
}

RN_BASIS_INCLUDES = {
    BASIS_DIR
}

project "basisu_encoder"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    defines(BASIS_DEFINES)

    flags {
        "MultiProcessorCompile"
    }

    files(BASIS_LIBRARY_SRCS)
    includedirs(BASIS_DIR)
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"


project "basisu"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    defines(BASIS_DEFINES)

    flags {
        "MultiProcessorCompile"
    }

    files(BASISU_SRCS)
    includedirs(BASIS_DIR)
    links { "basisu_encoder" }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"