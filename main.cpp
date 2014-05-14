#include <QApplication>
#include <dake/math/matrix.hpp>

#include "cloud.hpp"
#include "window.hpp"


cloud c;
window *wnd;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    std::ifstream inp(argv[1]);
    c.load(inp);

    wnd = new window;
    wnd->show();

    return app.exec();
}


void draw_clouds(void)
{
    wnd->renderer()->active_program()->uniform<dake::math::mat4>("mvp") = wnd->renderer()->projection() * c.transformation().translated(dake::math::vec3(0., 0., -5.));

    c.vertex_array()->draw(GL_POINTS);
}
