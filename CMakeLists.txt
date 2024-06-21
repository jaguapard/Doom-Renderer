cmake_minimum_required(VERSION 3.24)
project(Doom_Rendering)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

include_directories(src)
include_directories(src/bob)
include_directories(src/GameStates)

include_directories($ENV{INCLUDE})
link_libraries(SDL2 SDL2main SDL2_image)

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
        src/ZBuffer.h)
