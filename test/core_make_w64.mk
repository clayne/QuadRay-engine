
INC_PATH =                                  \
        -I../core/config/                   \
        -I../core/engine/                   \
        -I../core/system/                   \
        -I../core/tracer/                   \
        -I../data/materials/                \
        -I../data/objects/                  \
        -I../data/textures/                 \
        -Iscenes/

SRC_LIST =                                  \
        ../core/engine/engine.cpp           \
        ../core/engine/object.cpp           \
        ../core/engine/rtgeom.cpp           \
        ../core/engine/rtimag.cpp           \
        ../core/system/system.cpp           \
        ../core/tracer/tracer.cpp           \
        ../core/tracer/tracer_128v2.cpp     \
        ../core/tracer/tracer_128v4.cpp     \
        ../core/tracer/tracer_128v8.cpp     \
        ../core/tracer/tracer_256v4_r8.cpp  \
        ../core/tracer/tracer_256v1.cpp     \
        ../core/tracer/tracer_256v2.cpp     \
        ../core/tracer/tracer_256v8.cpp     \
        ../core/tracer/tracer_512v1_r8.cpp  \
        ../core/tracer/tracer_512v2_r8.cpp  \
        ../core/tracer/tracer_512v1.cpp     \
        ../core/tracer/tracer_512v2.cpp     \
        ../core/tracer/tracer_512v4.cpp     \
        ../core/tracer/tracer_512v8.cpp     \
        ../core/tracer/tracer_1K4v1.cpp     \
        ../core/tracer/tracer_1K4v2.cpp     \
        ../core/tracer/tracer_2K8v1_r8.cpp  \
        ../core/tracer/tracer_2K8v2_r8.cpp  \
        core_test.cpp

LIB_PATH =

LIB_LIST =                                  \
        -lm                                 \
        -lstdc++


build: core_test_w64_32 core_test_w64_64 core_test_w64f32 core_test_w64f64

strip:
	strip core_test_w64*.exe

clean:
	del core_test_w64*.exe


core_test_w64_32:
	g++ -O3 -g -static -m64 \
        -DRT_WIN64 -DRT_X64 \
        -DRT_128=2+4+8 -DRT_256_R8=4 -DRT_256=1+2+8 \
        -DRT_512_R8=1+2 -DRT_512=1+2 -DRT_1K4=1+2 -DRT_2K8_R8=0 \
        -DRT_POINTER=64 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test_w64_32.exe

core_test_w64_64:
	g++ -O3 -g -static -m64 \
        -DRT_WIN64 -DRT_X64 \
        -DRT_128=2+4+8 -DRT_256_R8=4 -DRT_256=1+2+8 \
        -DRT_512_R8=1+2 -DRT_512=1+2 -DRT_1K4=1+2 -DRT_2K8_R8=0 \
        -DRT_POINTER=64 -DRT_ADDRESS=32 -DRT_ELEMENT=64 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test_w64_64.exe

core_test_w64f32:
	g++ -O3 -g -static -m64 \
        -DRT_WIN64 -DRT_X64 \
        -DRT_128=2+4+8 -DRT_256_R8=4 -DRT_256=1+2+8 \
        -DRT_512_R8=1+2 -DRT_512=1+2 -DRT_1K4=1+2 -DRT_2K8_R8=0 \
        -DRT_POINTER=64 -DRT_ADDRESS=64 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test_w64f32.exe

core_test_w64f64:
	g++ -O3 -g -static -m64 \
        -DRT_WIN64 -DRT_X64 \
        -DRT_128=2+4+8 -DRT_256_R8=4 -DRT_256=1+2+8 \
        -DRT_512_R8=1+2 -DRT_512=1+2 -DRT_1K4=1+2 -DRT_2K8_R8=0 \
        -DRT_POINTER=64 -DRT_ADDRESS=64 -DRT_ELEMENT=64 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test_w64f64.exe


# Prerequisites for the build:
# TDM64-GCC compiler for Win32/64 is installed and in the PATH variable.
# Download tdm64-gcc-5.1.0-2.exe from sourceforge and run the installer.
#
# Building/running CORE test:
# run core_make_w64.bat from Windows UI or
# run the following from Command Prompt "cmd":
# mingw32-make -f core_make_w64.mk
# core_test_w64f32.exe -i -a
# (should produce antialiased (-a) images (-i) in the ../dump subfolder)

# core_test uses runtime SIMD target selection, multiple can be specified above
# on x86 targets top cpuid-value is chosen by default, use -n/-k/-s to override
# 1K4-bit SIMD is achieved by combining pairs of 512-bit registers/instructions
# 2K8-bit SIMD is achieved by combining quads of 512-bit registers/instructions
# For 30 512-bit AVX3.1/.2 registers on Skylake-X use (replace): RT_512=4+8

# For interpretation of SIMD build flags check compatibility layer in rtzero.h

# 64/32-bit (ptr/adr) hybrid mode is compatible with native 64-bit ABI,
# use (replace): RT_ADDRESS=32, rename the binary to core_test_w64_**.exe

# 64-bit packed SIMD mode (fp64/int64) is supported on 64-bit targets,
# use (replace): RT_ELEMENT=64, rename the binary to core_test_w64*64.exe
