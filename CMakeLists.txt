cmake_minimum_required(VERSION 3.24)
project(Doom_Rendering)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -fpermissive")

include_directories(src)
include_directories(src/bob)
include_directories(src/GameStates)
include_directories(src/GLUTesselator)

#ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/tessellate.c )
ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/dict.c src/GLUTesselator/mesh.c src/GLUTesselator/render.c src/GLUTesselator/tess.c src/GLUTesselator/geom.c src/GLUTesselator/memalloc.c src/GLUTesselator/normal.c src/GLUTesselator/priorityq.c src/GLUTesselator/sweep.c src/GLUTesselator/tessmono.c src/GLUTesselator/tessellate.c )
#ADD_LIBRARY( GLUTesselator STATIC src/GLUTesselator/tessellate.c )

include_directories($ENV{INCLUDE})
link_libraries(SDL2 SDL2main SDL2_image SDL2_ttf GLUTesselator)

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
    include_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2-2.30.1/x86_64-w64-mingw32/include)
    include_directories($ENV{LIB}/SDL2_image-2.8.2/x86_64-w64-mingw32/include)
    include_directories($ENV{LIB}/SDL2_ttf-2.22.0/x86_64-w64-mingw32/include)

    link_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2-2.30.1/x86_64-w64-mingw32/lib)
    link_directories($ENV{LIB}/SDL2-devel-2.30.1-mingw/SDL2_image-2.8.2x86_64-w64-mingw32/lib)
    link_directories($ENV{LIB}/SDL2_ttf-2.22.0/x86_64-w64-mingw32/lib)
endif()

add_executable(Doom_Rendering
        src/bob/SSE_Vec4.h
        src/bob/Timer.h
        src/bob/Vec2.h
        src/bob/Vec3.h
        src/GameStates/BenchmarkState.cpp
        src/GameStates/BenchmarkState.h
        src/GameStates/GameStateBase.cpp
        src/GameStates/GameStateBase.h
        src/GameStates/MainGame.cpp
        src/GameStates/MainGame.h
        src/blitting.cpp
        src/blitting.h
        src/C_Input.cpp
        src/C_Input.h
        src/Color.cpp
        src/Color.h
        src/CoordinateTransformer.cpp
        src/CoordinateTransformer.h
        src/DoomMap.cpp
        src/DoomMap.h
        src/DoomStructs.h
        src/DoomWorldLoader.cpp
        src/DoomWorldLoader.h
        src/FloatPack16.h
        src/FloatPack8.h
        src/helpers.h
        src/main.cpp
        src/Matrix4.cpp
        src/Matrix4.h
        src/Model.cpp
        src/Model.h
        src/PerformanceMonitor.cpp
        src/PerformanceMonitor.h
        src/PixelBuffer.h
        src/Polygon.cpp
        src/Polygon.h
        src/PolygonBitmap.cpp
        src/PolygonBitmap.h
        src/PolygonTriangulator.cpp
        src/PolygonTriangulator.h
        src/real.h
        src/Sky.cpp
        src/Sky.h
        src/smart.h
        src/Statsman.cpp
        src/Statsman.h
        src/Texture.cpp
        src/Texture.h
        src/TextureManager.cpp
        src/TextureManager.h
        src/TextureMapper.cpp
        src/TextureMapper.h
        src/TexVertex.cpp
        src/TexVertex.h
        src/Threadpool.cpp
        src/Threadpool.h
        src/Triangle.cpp
        src/Triangle.h
        src/Vec.h
        src/VectorPack.h
        src/WadLoader.cpp
        src/WadLoader.h
        src/ZBuffer.cpp
        src/ZBuffer.h
        src/FloatColorBuffer.h
        src/FloatColorBuffer.cpp)
