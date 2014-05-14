#include <QApplication>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>

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
    dake::math::mat4 mv(wnd->renderer()->modelview() * c.transformation());
    dake::math::mat3 norm(mv);
    norm.transposed_invert();

    dake::gl::program *prg = wnd->renderer()->select_program(false);
    prg->uniform<dake::math::mat4>("mv") = mv;
    if (wnd->renderer()->lighting_enabled()) {
        prg->uniform<dake::math::mat3>("nmat") = norm;
        prg->uniform<dake::math::vec3>("light_dir") = dake::math::vec3(.5f, -1.f, .5f).normalized();
    }

    c.vertex_array()->draw(GL_POINTS);


    prg = wnd->renderer()->select_program(true);
    prg->uniform<dake::math::mat4>("mv") = mv;
    prg->uniform<dake::math::mat3>("nmat") = norm;

    c.vertex_array()->draw(GL_POINTS);
}
