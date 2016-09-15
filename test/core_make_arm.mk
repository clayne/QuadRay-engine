
INC_PATH =                              \
        -I../core/config/               \
        -I../core/engine/               \
        -I../core/system/               \
        -I../core/tracer/               \
        -I../data/materials/            \
        -I../data/objects/              \
        -I../data/textures/             \
        -Iscenes/

SRC_LIST =                              \
        ../core/engine/engine.cpp       \
        ../core/engine/object.cpp       \
        ../core/engine/rtgeom.cpp       \
        ../core/engine/rtimag.cpp       \
        ../core/system/system.cpp       \
        ../core/tracer/tracer.cpp       \
        ../core/tracer/tracer_128v1.cpp \
        ../core/tracer/tracer_128v2.cpp \
        ../core/tracer/tracer_128v4.cpp \
        core_test.cpp

LIB_PATH =

LIB_LIST =                              \
        -lm                             \
        -lstdc++


build: core_test_arm

strip:
	arm-linux-gnueabi-strip core_test.arm

clean:
	rm core_test.arm


core_test_arm:
	arm-linux-gnueabi-g++ -O3 -g -static \
        -DRT_LINUX -DRT_ARM -DRT_128=1 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test.arm


build_n900: core_test_arm_n900

strip_n900:
	arm-linux-gnueabi-strip core_test.arm_n900

clean_n900:
	rm core_test.arm_n900


core_test_arm_n900:
	arm-linux-gnueabi-g++ -O3 -g -static \
        -DRT_LINUX -DRT_ARM -DRT_128=1 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=1 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test.arm_n900


build_rpiX: core_test_arm_rpi2 core_test_arm_rpi3

strip_rpiX:
	arm-linux-gnueabihf-strip core_test.arm_rpi*

clean_rpiX:
	rm core_test.arm_rpi*


core_test_arm_rpi2:
	arm-linux-gnueabihf-g++ -O3 -g -static \
        -DRT_LINUX -DRT_ARM -DRT_128=2 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test.arm_rpi2

core_test_arm_rpi3:
	arm-linux-gnueabihf-g++ -O3 -g -static \
        -DRT_LINUX -DRT_ARM -DRT_128=4 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test.arm_rpi3


# On Ubuntu 16.04 Live CD add "universe multiverse" to "main restricted"
# in /etc/apt/sources.list (sudo gedit /etc/apt/sources.list) then run:
# sudo apt-get update (ignoring the old database errors in the end)
#
# Prerequisites for the build:
# (cross-)compiler for ARMv7 is installed and in the PATH variable.
# sudo apt-get install g++-arm-linux-gnueabi
#
# Prerequisites for emulation:
# recent QEMU(-2.5) is installed or built from source and in the PATH variable.
# sudo apt-get install qemu
#
# Building/running CORE test:
# make -f core_make_arm.mk
# qemu-arm -cpu cortex-a8 core_test.arm -i -a
# (should produce antialiased (-a) images (-i) in the ../dump subfolder)

# 0) Build flags above are intended for default "vanilla" ARMv7 target, while
# settings suitable for specific hardware platforms are given below (replace).
# 1) Nokia N900, Maemo 5 scratchbox: -DRT_EMBED_FILEIO=1
# 2) Raspberry Pi 2, Raspbian: arm-linux-gnueabihf-g++ -DRT_128=2
# 3) Raspberry Pi 3, Raspbian: arm-linux-gnueabihf-g++ -DRT_128=4
