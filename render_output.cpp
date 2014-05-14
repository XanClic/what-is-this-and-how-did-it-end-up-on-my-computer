#include <cassert>
#include <stdexcept>
#include <QtOpenGL>
#include <QDoubleSpinBox>
#include <QTimer>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>

#include "render_output.hpp"


render_output::render_output(QGLFormat fmt, QDoubleSpinBox *point_size_widget, QWidget *parent):
    QGLWidget(fmt, parent),
    psw(point_size_widget),
    proj(dake::math::mat4::identity())
{
    redraw_timer = new QTimer(this);
    connect(redraw_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
}

render_output::~render_output(void)
{
}


void render_output::change_point_size(double sz)
{
    glPointSize(sz);
}


void render_output::change_point_smoothness(int smooth)
{
    if (smooth) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_NOTEQUAL, 0.f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_POINT_SMOOTH);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_POINT_SMOOTH);
    }

}


void render_output::change_lighting(int lighting)
{
    enable_lighting(lighting);
}


void render_output::initializeGL(void)
{
    int maj, min;
    glGetIntegerv(GL_MAJOR_VERSION, &maj);
    glGetIntegerv(GL_MINOR_VERSION, &min);

    printf("Received OpenGL %i.%i\n", maj, min);

    glViewport(0, 0, width(), height());
    glClearColor(0.f, 0.f, 0.f, 1.f);

    glEnable(GL_DEPTH_TEST);

    int psr[2];
    glGetIntegerv(GL_POINT_SIZE_RANGE, psr);

    psw->setMinimum(psr[0]);
    psw->setMaximum(psr[1]);

    dake::gl::shader tvsh(dake::gl::shader::VERTEX);
    tvsh.source("#version 150 core\n"
               "in vec3 in_position;\n"
               "in vec3 in_color;\n"
               "out vec3 vf_color;\n"
               "uniform mat4 mvp;\n"
               "void main(void)\n"
               "{\n"
               "    gl_Position = mvp * vec4(in_position, 1.0);\n"
               "    vf_color = in_color;\n"
               "}");
    if (!tvsh.compile())
        throw std::logic_error("Could not compile trivial vertex shader");

    dake::gl::shader lvsh(dake::gl::shader::VERTEX);
    lvsh.source("#version 150 core\n"
               "in vec3 in_position;\n"
               "in vec3 in_normal;\n"
               "in vec3 in_color;\n"
               "out vec3 vf_color;\n"
               "uniform mat4 mvp;\n"
               "uniform mat3 nmat;\n"
               "uniform vec3 light_dir;\n"
               "void main(void)\n"
               "{\n"
               "    gl_Position = mvp * vec4(in_position, 1.0);\n"
               "    vf_color = max(dot(nmat * in_normal, light_dir), 0.0) * in_color;\n"
               "}");
    if (!lvsh.compile())
        throw std::logic_error("Could not compile lighting vertex shader");

    dake::gl::shader fsh(dake::gl::shader::FRAGMENT);
    fsh.source("#version 150 core\n"
               "in vec3 vf_color;\n"
               "out vec4 out_color;\n"
               "void main(void)\n"
               "{\n"
               "    out_color = vec4(vf_color, 1.0);\n"
               "}\n");
    if (!fsh.compile())
        throw std::logic_error("Could not compile fragment shader");


    tprg = new dake::gl::program;
    *tprg << tvsh;
    *tprg << fsh;

    tprg->bind_attrib("in_position", 0);
    tprg->bind_attrib("in_normal", 1);
    tprg->bind_attrib("in_color", 2);

    tprg->bind_frag("out_color", 0);

    if (!tprg->link())
        throw std::logic_error("Could not link trivial program");


    lprg = new dake::gl::program;
    *lprg << lvsh;
    *lprg << fsh;

    lprg->bind_attrib("in_position", 0);
    lprg->bind_attrib("in_normal", 1);
    lprg->bind_attrib("in_color", 2);

    lprg->bind_frag("out_color", 0);

    if (!lprg->link())
        throw std::logic_error("Could not link lighting program");


    tprg->use();

    redraw_timer->start(0);
}

void render_output::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    proj = dake::math::mat4::projection(M_PI / 4.f, static_cast<float>(w) / h, .1f, 100.f);
}

extern void draw_clouds(void);

void render_output::paintGL(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_clouds();
}
