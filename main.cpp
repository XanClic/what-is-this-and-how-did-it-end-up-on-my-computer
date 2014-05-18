#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <QApplication>

#include "cloud.hpp"
#include "window.hpp"


cloud_manager cm;


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    for (int i = 1; i < argc; i++) {
        std::ifstream inp(argv[i]);
        if (!inp.is_open()) {
            fprintf(stderr, "%s: Could not open %s: %s\n", argv[0], argv[i], strerror(errno));
            // If you suddenly feel the urge for refreshment after actually
            // reading "strerror(errno)" in a C++ program using STL I/O streams,
            // you may find my comments in window.cpp amusing
            // (maybe I should've just sticked to C and Gtk2)
            return 1;
        }

        char name_copy[strlen(argv[i]) + 1];
        strcpy(name_copy, argv[i]);
        cm.load_new(inp, basename(name_copy));
    }

    // Putting this in something fancy like an std::shared_ptr doesn't suit Qt,
    // everything will go to hell - so don't do it
    // (find out more on Qt and me in window.cpp)
    window *wnd = new window;
    wnd->show();

    return app.exec();
}
