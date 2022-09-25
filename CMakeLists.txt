cmake_minimum_required(VERSION 3.23)
project(CG_Basic)
find_package(OpenCV REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(CG_Basic main.cpp TransformMatrix.cpp TransformMatrix.h Renderer.cpp Renderer.h Shader.cpp Shader.h Primitive.cpp Primitive.h ThirdParty/OBJ_Loader.h Rasterizer.cpp Rasterizer.h ScreenBuffer.cpp ScreenBuffer.h)
target_link_libraries(CG_Basic ${OpenCV_LIBRARIES})
file(COPY Resources DESTINATION ./)
