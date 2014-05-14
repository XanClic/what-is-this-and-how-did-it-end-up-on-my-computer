cmake_minimum_required(VERSION 2.8.8)

project(cg2p1)

set(CMAKE_AUTOMOC on)

find_package(OpenGL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)

include_directories(.)
link_directories(.)

add_executable(cg2p1 main.cpp cloud.cpp window.cpp render_output.cpp)
target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES})

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -DGL_GLEXT_PROTOTYPES")

qt5_use_modules(cg2p1 Core Gui OpenGL)
