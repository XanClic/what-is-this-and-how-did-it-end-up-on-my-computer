#include <cstdio>
#include <list>
#include <QApplication>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>

#include "cloud.hpp"
#include "window.hpp"


std::list<cloud> clouds;
window *wnd;


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (argc <= 1) {
        fprintf(stderr, "Usage: %s file0.ply [file1.ply [file2.ply [...]]]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        std::ifstream inp(argv[i]);
        if (!inp.is_open()) {
            fprintf(stderr, "%s: Could not open %s\n", argv[0], argv[i]);
            return 1;
        }

        clouds.emplace_back();
        clouds.back().load(inp);
    }

    wnd = new window;
    wnd->show();

    return app.exec();
}
