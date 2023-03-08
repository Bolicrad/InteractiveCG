cmake_minimum_required(VERSION 3.24)
project(InteractiveCG)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_OSX_ARCHITECTURES  "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

set(GLFW_LINK /opt/homebrew/lib/libglfw.3.dylib)
link_libraries(${OPENGL} ${GLEW_LINK} ${GLFW_LINK})

add_executable(OpenGL src/glad.c include/loadpng/lodepng.cpp src/main.cpp)
target_include_directories(OpenGL PUBLIC /opt/homebrew/include)
target_include_directories(OpenGL PRIVATE ${PROJECT_SOURCE_DIR}/include)

if (APPLE)
    target_link_libraries(OpenGL "-framework OpenGL")
    target_link_libraries(OpenGL "-framework GLUT")
endif ()