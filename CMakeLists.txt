cmake_minimum_required(VERSION 2.8.8)

include(ExternalProject)

project(cg2p1)

set(CMAKE_AUTOMOC on)

find_package(OpenGL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)

if(WIN32)
    ExternalProject_Add(
        dake
        GIT_REPOSITORY git://github.com/XanClic/dake.git
        GIT_TAG origin/master
        SOURCE_DIR dake
        CMAKE_COMMAND true
        BUILD_COMMAND make -f Makefile.windows
        BINARY_DIR dake
        INSTALL_COMMAND true
    )
else(WIN32)
    ExternalProject_Add(
        dake
        GIT_REPOSITORY git://github.com/XanClic/dake.git
        GIT_TAG origin/master
        SOURCE_DIR dake
        CMAKE_COMMAND true
        BUILD_COMMAND make -j 4
        BINARY_DIR dake
        INSTALL_COMMAND true
    )
endif(WIN32)

ExternalProject_Get_Property(dake binary_dir)

include_directories(${binary_dir}/include /usr/include/eigen3)
link_directories(${binary_dir})

add_executable(cg2p1 main.cpp cloud.cpp window.cpp render_output.cpp shader_sources.cpp kd_tree.cpp)
if(WIN32)
    target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES} /c/MinGW/bin/glew32.dll glew32.lib)
else(WIN32)
    target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES})
endif(WIN32)
add_dependencies(cg2p1 dake)

set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -g2 -Wall -Wextra")

qt5_use_modules(cg2p1 Core Gui OpenGL)
