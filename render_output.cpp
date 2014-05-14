#include <cassert>
#include <cstring>
#include <stdexcept>
#include <QtOpenGL>
#include <QDoubleSpinBox>
#include <QTimer>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>

#include "render_output.hpp"


enum program_flag_bits {
    BIT_LIGHTING,
    BIT_NORMALS,
    BIT_COLORED,

    PROGRAM_FLAG_COUNT
};

enum program_flags {
    LIGHTING = 1 << BIT_LIGHTING,
    NORMALS  = 1 << BIT_NORMALS,
    COLORED  = 1 << BIT_COLORED,

    PROGRAM_COUNT = 1 << PROGRAM_FLAG_COUNT
};


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
    delete[] prgs;
}


dake::gl::program *render_output::select_program(bool normals)
{
    dake::gl::program *selected = &prgs[(lighting * LIGHTING) |
                                        (normals  * NORMALS)  |
                                        (colored  * COLORED)];

    if (selected != current_prg) {
        selected->use();

        if (normals)
            selected->uniform<float>("normal_scale") = normlen;
    }

    return current_prg = selected;
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
                "in vec3 in_normal;\n"
                "in vec3 in_color;\n"
                "out vec3 vg_color;\n"
                "out vec3 vg_normal;\n"
                "uniform mat4 mv;\n"
                "void main(void)\n"
                "{\n"
                "    gl_Position = mv * vec4(in_position, 1.0);\n"
                "    vg_color = in_color;\n"
                "    vg_normal = in_normal;\n"
                "}");
    if (!tvsh.compile())
        throw std::logic_error("Could not compile trivial vertex shader");

    dake::gl::shader lvsh(dake::gl::shader::VERTEX);
    lvsh.source("#version 150 core\n"
                "in vec3 in_position;\n"
                "in vec3 in_normal;\n"
                "in vec3 in_color;\n"
                "out vec3 vg_color;\n"
                "out vec3 vg_normal;\n"
                "uniform mat4 mv;\n"
                "uniform mat3 nmat;\n"
                "uniform vec3 light_dir;\n"
                "void main(void)\n"
                "{\n"
                "    gl_Position = mv * vec4(in_position, 1.0);\n"
                "    vg_color = max(dot(nmat * in_normal, light_dir), 0.0) * in_color;\n"
                "    vg_normal = in_normal;\n"
                "}");
    if (!lvsh.compile())
        throw std::logic_error("Could not compile lighting vertex shader");

    dake::gl::shader utvsh(dake::gl::shader::VERTEX);
    utvsh.source("#version 150 core\n"
                 "in vec3 in_position;\n"
                 "in vec3 in_normal;\n"
                 "out vec3 vg_color;\n"
                 "out vec3 vg_normal;\n"
                 "uniform mat4 mv;\n"
                 "void main(void)\n"
                 "{\n"
                 "    gl_Position = mv * vec4(in_position, 1.0);\n"
                 "    vg_color = vec3(1.0, 1.0, 1.0);\n"
                 "    vg_normal = in_normal;\n"
                 "}");
    if (!utvsh.compile())
        throw std::logic_error("Could not compile uncolored trivial vertex shader");

    dake::gl::shader ulvsh(dake::gl::shader::VERTEX);
    ulvsh.source("#version 150 core\n"
                 "in vec3 in_position;\n"
                 "in vec3 in_normal;\n"
                 "out vec3 vg_color;\n"
                 "out vec3 vg_normal;\n"
                 "uniform mat4 mv;\n"
                 "uniform mat3 nmat;\n"
                 "uniform vec3 light_dir;\n"
                 "void main(void)\n"
                 "{\n"
                 "    gl_Position = mv * vec4(in_position, 1.0);\n"
                 "    vg_color = max(dot(nmat * in_normal, light_dir), 0.0) * vec3(1.0, 1.0, 1.0);\n"
                 "    vg_normal = in_normal;\n"
                 "}");
    if (!ulvsh.compile())
        throw std::logic_error("Could not compile uncolored lighting vertex shader");

    dake::gl::shader tgsh(dake::gl::shader::GEOMETRY);
    tgsh.source("#version 150 core\n"
                "layout(points) in;\n"
                "layout(points, max_vertices=1) out;\n"
                "in vec3 vg_color[];\n"
                "out vec3 gf_color;\n"
                "uniform mat4 proj;\n"
                "void main(void)\n"
                "{\n"
                "    gf_color = vg_color[0];\n"
                "    gl_Position = proj * gl_in[0].gl_Position;\n"
                "    EmitVertex();\n"
                "    EndPrimitive();\n"
                "}");
    if (!tgsh.compile())
        throw std::logic_error("Could not compile trivial geometry shader");

    dake::gl::shader ngsh(dake::gl::shader::GEOMETRY);
    ngsh.source("#version 150 core\n"
               "layout(points) in;\n"
               "layout(line_strip, max_vertices=2) out;\n"
               "in vec3 vg_normal[];\n"
               "out vec3 gf_color;\n"
               "uniform mat4 proj;\n"
               "uniform mat3 nmat;\n"
               "uniform float normal_scale;\n"
               "void main(void)\n"
               "{\n"
               "    gf_color = vg_normal[0];\n"
               "    gl_Position = proj * gl_in[0].gl_Position;\n"
               "    EmitVertex();\n"
               "\n"
               "    gf_color = vg_normal[0];\n"
               "    gl_Position = proj * vec4(gl_in[0].gl_Position.xyz + normal_scale * (nmat * vg_normal[0]), 1.0);\n"
               "    EmitVertex();\n"
               "\n"
               "    EndPrimitive();\n"
               "}");
    if (!ngsh.compile())
        throw std::logic_error("Could not compile normal geometry shader");

    dake::gl::shader fsh(dake::gl::shader::FRAGMENT);
    fsh.source("#version 150 core\n"
               "in vec3 gf_color;\n"
               "out vec4 out_color;\n"
               "void main(void)\n"
               "{\n"
               "    out_color = vec4(gf_color, 1.0);\n"
               "}\n");
    if (!fsh.compile())
        throw std::logic_error("Could not compile fragment shader");


    prgs = new dake::gl::program[PROGRAM_COUNT];

    for (int i = 0; i < PROGRAM_COUNT; i++) {
        switch (i & (LIGHTING | COLORED)) {
            case LIGHTING | COLORED: prgs[i] <<  lvsh; break;
            case LIGHTING:           prgs[i] << ulvsh; break;
            case COLORED:            prgs[i] <<  tvsh; break;
            case 0:                  prgs[i] << utvsh; break;
        };

        switch (i & NORMALS) {
            case NORMALS: prgs[i] << ngsh; break;
            case 0:       prgs[i] << tgsh; break;
        }

        prgs[i] << fsh;

        prgs[i].bind_attrib("in_position", 0);
        prgs[i].bind_attrib("in_normal", 1);
        prgs[i].bind_attrib("in_color", 2);

        prgs[i].bind_frag("out_color", 0);

        if (!prgs[i].link())
            throw std::logic_error("Could not link program");
    }

    prgs[0].use();
    current_prg = &prgs[0];

    redraw_timer->start(0);
}

void render_output::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    proj = dake::math::mat4::projection(M_PI / 4.f, static_cast<float>(w) / h, .1f, 100.f);

    dake::gl::program *cur = current_prg;
    for (int i = 0; i < PROGRAM_COUNT; i++)
        prgs[i].uniform<dake::math::mat4>("proj") = proj;
    if (cur)
        cur->use();
}

extern void draw_clouds(void);

void render_output::paintGL(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_clouds();
}
