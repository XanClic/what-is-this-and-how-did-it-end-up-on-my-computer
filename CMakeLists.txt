cmake_minimum_required(VERSION 2.8.8)

include(ExternalProject)

project(cg2p1)

set(CMAKE_AUTOMOC on)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)
find_package(Eigen3 REQUIRED)

ExternalProject_Add(
    dake
    GIT_REPOSITORY git://github.com/XanClic/dake.git
    GIT_TAG origin/master
    SOURCE_DIR dake
    CMAKE_COMMAND true
    BUILD_COMMAND make
    BINARY_DIR dake
    INSTALL_COMMAND true
)

ExternalProject_Get_Property(dake binary_dir)

include_directories(${binary_dir}/include ${EIGEN3_INCLUDE_DIR})
link_directories(${binary_dir})

add_executable(cg2p1 main.cpp cloud.cpp window.cpp render_output.cpp shader_sources.cpp kd_tree.cpp rng.cpp)
if(WIN32)
    # Of course this only works on my system - this is Windows, what did you
    # expect?
    # Windows is a steaming pile of shit, it's 8 in the morning and I really
    # don't have the energy to fix this. People on the internet think it's
    # stupid to link against DLLs - they're probably right, but MinGW is
    # capable of doing that, and I need to do it as I'll get undefined
    # references otherwise. I can't give a simple filename here, as CMake
    # will apparently "optimize" this or something and drop it. Great. Thanks.
    # I hate Windows. Windows is really a bad OS. I don't know why I'm forced to
    # create an executable for this fucking piece of shit.
    # Please note that I didn't even swear about Qt. But about Windows, I can't
    # help myself. Once again, it's 8 am. It was 6 when I thought "Oh, come on,
    # just recompile it under Windows, adding Eigen3 will be the worst thing of
    # it all". It wasn't. First I added Eigen3. Funny thing is, it compiles with
    # CMake, but isn't found by CMake and the FindEigen3.cmake of Eigen is
    # worthless, as it uses the prefix of the program and not of Eigen. Great.
    # So I fixed that. Then, gcc complained about std::mutex. Turns out, mingw32
    # does not have std::mutex. Also, gcc crashed because OOM (wtf?). So I
    # installed mingw64. Great, only now Qt didn't work anymore. So I went to
    # download Qt for mingw64. Only there is no such thing. Great. So I went to
    # download the source code and compile it. Only that did not work, because
    # it could not find its own functions or something (undefined reference to
    # qgetenv()). Okay, so I tried to make mingw64 work with Qt for mingw32
    # through -m32. Only that did not work, because mingw64 does not have any
    # libraries for i686. Also, it tried to use __int128 which is not supported
    # with -m32. Great. So I tried to take over all the libraries from my
    # mingw32 installation. Now it nearly worked, all that remained was iconv.
    # But that did not work at all. So I went on and finally I found some Qt5
    # build for mingw64, thank my Lord and Savior the flying spaghetti monster.
    # Tears of joy were shed, but it was another 500 MB before I could try it
    # out (I have DSL3000 here thank you). Okay, so that thing had everything
    # and it even worked with my existing mingw64 installation. After some minor
    # fixing, I could build my project again! And all because mingw32 does not
    # have std::mutex. And here I am now, more than two hours later and I don't
    # even know how people are capable of working on this fucking shit that is
    # apparently rumored to be usable as an OS. I for one cannot comprehend
    # that. It's shit. Simple as that. It's just shit. I don't even know what's
    # good about this thing. I hate Apple, but OS X is at least usable. Windows
    # is just shit. The developers at Microsoft always seem like pretty cool
    # guys, you'd think they'd sit down at one point in time and say "Yeah,
    # development under Windows is really a huge PITA, we should fix that, let's
    # introduce some nice common include and library directories, and maybe good
    # POSIX support, maybe make an official GCC or Clang port", but they don't.
    # VC++ is a bad compiler, nobody in their right mind would ever want to use
    # it, so one can't even say "Well, with VC++ everything works fine" (which
    # it does not even, I could burst into tears, but I don't know because of
    # laughter or despair). Windows is fucking shit. We just use it because it
    # has D3D. Maybe D3D is good. I don't know. But we're using OpenGL at TU
    # Dresden. Why the hell am I forced to support Windows? There is no fucking
    # reason whatsoever.
    # ...
    # ...
    # Okay, so here I am, it's 8:21 am and I really have no intention to fix the
    # path to glew32.dll below. Thank you.
    # If you've come here because you're trying to compile this on Windows and
    # the path given here is not found or something, you have my deepest pity.
    # Also, please use Linux or BSD or something.
    #
    # Okay, this comment is from some weeks later: I tried -lglew32. LD then
    # uses only glew32.lib, which is apparently not enough (undefined references
    # to __imp__glewGetUniformLocation and others). But I did realize something
    # different: I don't need the .lib at all! The .dll itself is enough
    # (glew32s.lib does not work either). Okay, this doesn't solve my original
    # problem (LD using the .lib instead of the .dll, unless *the full path is
    # specified*), so there's that. But still great, isn't it? I feel great.
    # Don't you? I know I do.
    #
    # Also, I just returned here from fixing ICP for Windows which needs some
    # completely useless reset of the target cloud's transformation matrix to
    # work (??!??!), so I'm a bit pissed right now.
    #
    # Oh, the "MX Release" of GLEW does not work either. Yay! Good times.
    #
    # I just decided to add some environment variable. Now you can't blame me
    # anymore! I feel a bit proud of myself.
    target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES} "$ENV{GLEW_DLL_PATH}")
else(WIN32)
    target_link_libraries(cg2p1 libdake.a ${OPENGL_LIBRARIES})
endif(WIN32)
add_dependencies(cg2p1 dake)

set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -g2 -Wall -Wextra")

qt5_use_modules(cg2p1 Core Gui OpenGL)
