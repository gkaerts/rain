local FLATBUFFERS_DIR = "%{wks.location}/../../contrib/submodules/flatbuffers"
local FLATBUFFER_LIBRARY_SRCS = {
    FLATBUFFERS_DIR .. "/include/flatbuffers/allocator.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/array.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/base.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/buffer.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/buffer_ref.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/default_allocator.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/detached_buffer.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/code_generator.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/file_manager.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/flatbuffer_builder.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/flatbuffers.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/flexbuffers.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/flex_flat_util.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/hash.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/idl.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/minireflect.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/reflection.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/reflection_generated.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/registry.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/stl_emulation.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/string.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/struct.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/table.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/util.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/vector.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/vector_downward.h",
    FLATBUFFERS_DIR .. "/include/flatbuffers/verifier.h",
    FLATBUFFERS_DIR .. "/src/idl_parser.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_text.cpp",
    FLATBUFFERS_DIR .. "/src/reflection.cpp",
    FLATBUFFERS_DIR .. "/src/util.cpp",
}

local FLATBUFFER_COMPILER_SRCS = {
    FLATBUFFERS_DIR .. "/src/idl_gen_binary.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_text.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_cpp.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_csharp.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_dart.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_kotlin.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_kotlin_kmp.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_go.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_java.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_ts.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_php.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_python.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_lobster.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_rust.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_fbs.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_grpc.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_json_schema.cpp",
    FLATBUFFERS_DIR .. "/src/idl_gen_swift.cpp",
    FLATBUFFERS_DIR .. "/src/file_name_saving_file_manager.cpp",
    FLATBUFFERS_DIR .. "/src/file_binary_writer.cpp",
    FLATBUFFERS_DIR .. "/src/file_writer.cpp",
    FLATBUFFERS_DIR .. "/src/idl_namer.h",
    FLATBUFFERS_DIR .. "/src/namer.h",
    FLATBUFFERS_DIR .. "/src/flatc.cpp",
    FLATBUFFERS_DIR .. "/src/flatc_main.cpp",
    FLATBUFFERS_DIR .. "/src/bfbs_gen.h",
    FLATBUFFERS_DIR .. "/src/bfbs_gen_lua.h",
    FLATBUFFERS_DIR .. "/src/bfbs_gen_nim.h",
    FLATBUFFERS_DIR .. "/src/bfbs_namer.h",
    FLATBUFFERS_DIR .. "/include/codegen/idl_namer.h",
    FLATBUFFERS_DIR .. "/include/codegen/namer.h",
    FLATBUFFERS_DIR .. "/include/codegen/python.h",
    FLATBUFFERS_DIR .. "/include/codegen/python.cc",
    FLATBUFFERS_DIR .. "/include/flatbuffers/code_generators.h",
    FLATBUFFERS_DIR .. "/src/binary_annotator.h",
    FLATBUFFERS_DIR .. "/src/binary_annotator.cpp",
    FLATBUFFERS_DIR .. "/src/annotated_binary_text_gen.h",
    FLATBUFFERS_DIR .. "/src/annotated_binary_text_gen.cpp",
    FLATBUFFERS_DIR .. "/src/bfbs_gen_lua.cpp",
    FLATBUFFERS_DIR .. "/src/bfbs_gen_nim.cpp",
    FLATBUFFERS_DIR .. "/src/code_generators.cpp",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/schema_interface.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/cpp_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/cpp_generator.cc",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/go_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/go_generator.cc",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/java_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/java_generator.cc",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/python_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/python_generator.cc",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/swift_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/swift_generator.cc",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/ts_generator.h",
    FLATBUFFERS_DIR .. "/grpc/src/compiler/ts_generator.cc",
}

RN_FLATBUFFERS_INCLUDES = {
    FLATBUFFERS_DIR .. "/include"
}

project "flatbuffers"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    files(FLATBUFFER_LIBRARY_SRCS)
    includedirs(RN_FLATBUFFERS_INCLUDES)
    
    targetdir "%{wks.location}/%{cfg.buildcfg}/"


project "flatc"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    flags {
        "MultiProcessorCompile"
    }

    files(FLATBUFFER_LIBRARY_SRCS)
    files(FLATBUFFER_COMPILER_SRCS)
    includedirs(RN_FLATBUFFERS_INCLUDES)
    includedirs {
        FLATBUFFERS_DIR .. "/grpc"
    }

    targetdir "%{wks.location}/%{cfg.buildcfg}/"

