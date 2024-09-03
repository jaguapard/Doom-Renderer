cmake_minimum_required(VERSION 3.24)
project(Doom_Rendering)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SRC
        "src/*.h"
        "src/*.cpp"
        )

include_directories(src)
include_directories(src/bob)
include_directories(src/GameStates)
include_directories(src/GLUTesselator)
include_directories(src/assimp)

#ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/tessellate.c )
ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/dict.c src/GLUTesselator/mesh.c src/GLUTesselator/render.c src/GLUTesselator/tess.c src/GLUTesselator/geom.c src/GLUTesselator/memalloc.c src/GLUTesselator/normal.c src/GLUTesselator/priorityq.c src/GLUTesselator/sweep.c src/GLUTesselator/tessmono.c src/GLUTesselator/tessellate.c )
#ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/tessellate.c )

link_libraries(SDL2 SDL2main SDL2_image SDL2_ttf GLUTesselator)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -fpermissive")
    include_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2-2.30.1/x86_64-w64-mingw32/include)
    include_directories($ENV{LIB}/SDL2_image-2.8.2/x86_64-w64-mingw32/include)
    include_directories($ENV{LIB}/SDL2_ttf-2.22.0/x86_64-w64-mingw32/include)

    link_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2-2.30.1/x86_64-w64-mingw32/lib)
    link_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2_image-2.8.2x86_64-w64-mingw32/lib)
    link_directories($ENV{LIB}/SDL2_ttf-2.22.0/x86_64-w64-mingw32/lib)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} /permissive- /arch:AVX512 /Zi /Gm- /O2 /sdl /Zc:inline /fp:fast")
    include_directories($ENV{INCLUDE})
    link_directories($ENV{LIB}/x64)
endif()


add_executable(Doom_Rendering ${SRC})
