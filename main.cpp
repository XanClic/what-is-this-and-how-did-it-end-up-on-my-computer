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
    dake::math::mat4 m(c.transformation().translated(dake::math::vec3(0.f, 0.f, -5.f)));
    wnd->renderer()->active_program()->uniform<dake::math::mat4>("mvp") = wnd->renderer()->projection() * m;
    if (wnd->renderer()->lighting_enabled()) {
        dake::math::mat3 norm(m);
        norm.transposed_invert();
        wnd->renderer()->active_program()->uniform<dake::math::mat3>("nmat") = norm;
        wnd->renderer()->active_program()->uniform<dake::math::vec3>("light_dir") = dake::math::vec3(.5f, -1.f, .5f).normalized();
    }

    c.vertex_array()->draw(GL_POINTS);
}
