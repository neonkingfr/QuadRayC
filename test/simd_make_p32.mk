
INC_PATH =                              \
        -I../core/config/

SRC_LIST =                              \
        simd_test.cpp

LIB_PATH =

LIB_LIST =                              \
        -lm

simd_test:
	powerpc-linux-gnu-g++ -O3 -g -static \
        -DRT_LINUX -DRT_P32 -DRT_128=1 -DRT_DEBUG=0 \
        -DRT_POINTER=32 -DRT_ADDRESS=32 -DRT_ENDIAN=1 \
        ${INC_PATH} ${SRC_LIST} ${LIB_PATH} ${LIB_LIST} -o simd_test.p32

# Prerequisites for the build:
# (cross-)compiler for PowerPC is installed and in the PATH variable.
# Prerequisites for emulation:
# latest QEMU(-2.5) is built from source and in the PATH variable.
#
# make -f simd_make_p32.mk
# qemu-ppc -cpu G4 simd_test.p32

# For 32-bit Power(7,7+,8) VSX target use (replace): -DRT_128=2
# qemu-ppc64abi32 -cpu POWER7 simd_test.p32

# For 64-bit Power(7,7+,8) VSX target use (replace): -DRT_128=2
# powerpc64le-linux-gnu-g++ -O2 -DRT_P64 -DRT_POINTER=64 -DRT_ENDIAN=0
# rename produced binary to simd_test.p64_32 or adjust the build command above.
# qemu-ppc64le -cpu POWER7 simd_test.p64_32

# For 64-bit Power(7,7+,8) VSX big-endian target use (replace): -DRT_128=2
# powerpc64-linux-gnu-g++ -O2 -DRT_P64 -DRT_POINTER=64 -DRT_ENDIAN=1
# rename produced binary to simd_test.p64_32 or adjust the build command above.
# qemu-ppc64 -cpu POWER7 simd_test.p64_32
