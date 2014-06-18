What is this?
-------------

I don't even know. It just was there one day on my hard disk. I don't know how
it got there!

Seriously speaking, it's my solution for exercise 1 of the SS14 CG2 course of TU
Dresden. It is something that deals with point clouds, being representation,
nearest-neighbor calculation, normal calculation and registration.


Build instructions
------------------

Yay, build instructions! I never wrote such stuff before, but let's give it a
try.


Linux
=====

Well, on Linux you just build it. Preferably in a different directory. I'd
recommend therefore (supposing you're in the source directory):

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

And that's it, there is your cg2p1.


### Requirements

* Libraries: Qt5 (I'm sorry), Eigen3, OpenGL (3.3+)
* Build programs: git (for loading dake, some kind of matrix/OpenGL library),
  a C++ build environment (preferably GNU), CMake, GNU make, the coreutils,
  etc. pp.

I hope I didn't forget anything.


Windows
=======

Hahaha, this is fun. I did my best to make this work, so good look to you, kind
stranger!

Generally, you can try what works. If you get to compile this, you probably did
it right. On the other hand, here's what I do:

    $ mkdir build
    $ cd build
    $ CMAKE_PREFIX_PATH="/c/QtSDK-x86_64/x86_64-w64-mingw32/"
      GLEW_DLL_PATH="/c/MinGW/mingw64/mingw64/bin/glew32.dll"
      cmake -G "Unix Makefiles" ..
    $ make

I think you can guess what the environment variables are:

* `CMAKE_PREFIX_PATH` is the path to your mingw64 installation of Qt5 (the devel
  part, i.e. libraries and include)
* `GLEW_DLL_PATH` is the link to a mingw64 GLEW DLL. No, the LIB will not work.
  Or maybe it will for you. But for me, it doesn't. That's why you have to give
  the path to the DLL (my linker won't use the DLL unless I give the whole path,
  it always tries to use the LIB which results in undefined references).


### Requirements

Try to build a GNU environment as much as possible. I have the following:

* mingw64: I haven't tried clang and Visual Studio will most probably not work.
  mingw32 will definitely not work (only very limited `<thread>` support and
  probably more issues).
* Qt5: You need Qt5 for mingw64. I have Qt 5.3, so it's only tested with that
  version. There is some mingw64 version online, try to
  google/bing/duckduckgo/whatever it.
* Eigen3: Download and install with CMake. It's installed to your Program Files
  folder, which is probably good. A FindEigen3.cmake file is part of this
  project and will try to locate Eigen3. If you just let CMake install it to the
  default location, there's a good chance it will be found.
* OpenGL: Should be part of Windows/mingw64; you'll need a 3.3+ environment.
* GLEW: Copy the headers to your mingw64 installation and copy the DLL anywhere,
  just point the environment variable there.
* Other mingw/msys/unixy tools: git, probably bash and of course other things
  such as the coreutils (ls etc.).

Put everything in your PATH, preferable before the Windows folders (so that the
GNU tools override the Windows tools (e.g. find and make)).



Acknowledgements
----------------

Thanks and cheers to the chair of computer graphics! I very much enjoyed the
hours I spent on Windows. I never thought I could ever again have the chance to.
And it feels useless, because I'll never use the executable built for Windows
myself. But thanks anyway! It teaches me a lot about diligence and having to
fulfill tasks even though you don't see the point but your superior forces you
to do them anyway.
