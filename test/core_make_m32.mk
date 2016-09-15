
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
        core_test.cpp

LIB_PATH =

LIB_LIST =                              \
        -lm                             \
        -lstdc++


build: core_test_m32

strip:
	mips-mti-linux-gnu-strip core_test.m32

clean:
	rm core_test.m32


core_test_m32:
	mips-mti-linux-gnu-g++ -O3 -g -static -EL -mips32r5 -mmsa \
        -DRT_LINUX -DRT_M32 -DRT_128=1 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ELEMENT=32 -DRT_ENDIAN=0 \
        -DRT_DEBUG=0 -DRT_PATH="../" \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=0 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o core_test.m32


# The up-to-date MIPS toolchain (g++ & QEMU) can be found here:
# https://community.imgtec.com/developers/mips/tools/codescape-mips-sdk/

# On Ubuntu 16.04 Live CD add "universe multiverse" to "main restricted"
# in /etc/apt/sources.list (sudo gedit /etc/apt/sources.list) then run:
# sudo apt-get update (ignoring the old database errors in the end)
#
# Prerequisites for the build:
# (cross-)compiler for MIPS+MSA is installed and in the PATH variable.
# Codescape.GNU.Tools.Package.2015.06-05.for.MIPS.MTI.Linux.CentOS-5.x86_64
# is unpacked and folder mips-mti-linux-gnu/2015.06-05/bin is added to PATH:
# PATH=/home/ubuntu/Downloads/mips-mti-linux-gnu/2015.06-05/bin:$PATH
#
# Prerequisites for emulation:
# recent QEMU(-2.5) is installed or built from source and in the PATH variable.
# sudo apt-get install qemu
#
# Building/running CORE test:
# make -f core_make_m32.mk
# qemu-mipsel -cpu P5600 core_test.m32 -i -a
# (should produce antialiased (-a) images (-i) in the ../dump subfolder)

# For MIPS32 Release 6 target use the following options (replace):
# mips-img-linux-gnu-g++ -mips32r6 -DRT_M32=6
# For MIPS32 Release 6 emulation use QEMU 2.4.0.1.0 from imgtec.com:
# qemu-mipsel -cpu mips32r6-generic core_test.m32 -i -a
# (should produce antialiased (-a) images (-i) in the ../dump subfolder)

# For big-endian MIPS32 (r5 and r6) use (replace): -EB -DRT_ENDIAN=1
# qemu-mips -cpu *** core_test.m32 -i -a
# where *** is P5600 for r5 build and mips32r6-generic for r6 build.
# (should produce antialiased (-a) images (-i) in the ../dump subfolder)
