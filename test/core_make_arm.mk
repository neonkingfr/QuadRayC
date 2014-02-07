
INC_PATH =                          \
        -I../core/config/           \
        -I../core/engine/           \
        -I../core/object/           \
        -I../core/system/           \
        -I../core/tracer/           \
        -I../data/materials/        \
        -I../data/objects/          \
        -I../data/textures/         \
        -Iscenes/

SRC_LIST =                          \
        ../core/engine/engine.cpp   \
        ../core/engine/thread.cpp   \
        ../core/object/object.cpp   \
        ../core/object/rtgeom.cpp   \
        ../core/object/rtimag.cpp   \
        ../core/system/system.cpp   \
        ../core/tracer/tracer.cpp   \
        core_test.cpp

core_test:
	g++ -O3 -g -fexceptions \
        -DRT_PATH="../" \
        -DRT_LINUX -DRT_ARM -DRT_DEBUG=1 \
        -DRT_EMBED_STDOUT=0 -DRT_EMBED_FILEIO=1 -DRT_EMBED_TEX=1 \
        ${INC_PATH} ${SRC_LIST} -o core_test.arm
