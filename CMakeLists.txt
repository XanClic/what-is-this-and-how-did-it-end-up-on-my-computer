cmake_minimum_required(VERSION 2.8.8)

include(ExternalProject)

project(cg2p1)

set(CMAKE_AUTOMOC on)

find_package(OpenGL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)

ExternalProject_Add(
    dake
    GIT_REPOSITORY git://github.com/XanClic/dake.git
    GIT_TAG master
    SOURCE_DIR dake
    CMAKE_COMMAND true
    BUILD_COMMAND make -j 4
    BINARY_DIR dake
    INSTALL_COMMAND true
)

ExternalProject_Get_Property(dake binary_dir)

include_directories(${binary_dir}/include)
link_directories(${binary_dir})

add_executable(cg2p1 main.cpp cloud.cpp window.cpp render_output.cpp)
target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES})
add_dependencies(cg2p1 dake)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -DGL_GLEXT_PROTOTYPES")

qt5_use_modules(cg2p1 Core Gui OpenGL)
