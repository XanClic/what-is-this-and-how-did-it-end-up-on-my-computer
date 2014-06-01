#include <dake/gl/gl.hpp>

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <QGLWidget>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>

#include "cloud.hpp"
#include "render_output.hpp"
#include "shader_sources.hpp"


using namespace dake;
using namespace dake::math;
using namespace dake::gl;


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

    PROGRAM_COUNT = 1 << PROGRAM_FLAG_COUNT,

    // Not-really-flags™ (may not be used with any other flags but only given
    // alone)

    // Riemann's Neighborhood Graph
    RNG      = 1 << PROGRAM_FLAG_COUNT,
};


static int shader_flag_mask(shader::type t)
{
    switch (t) {
        case shader::VERTEX:   return LIGHTING | COLORED;
        case shader::GEOMETRY: return NORMALS;
        case shader::FRAGMENT: return 0;
        default:               throw std::invalid_argument("Invalid argument given to shader_flag_mask()");
    }
}


render_output::render_output(QGLFormat fmt, QDoubleSpinBox *point_size_widget, QWidget *parent):
    QGLWidget(fmt, parent),
    psw(point_size_widget),
    proj(mat4::identity()),
    mv  (mat4::identity().translated(vec3(0.f, 0.f, -5.f))),
    light_dir(0.f, 0.f, 0.f)
{
    redraw_timer = new QTimer(this);
    connect(redraw_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
}

render_output::~render_output(void)
{
    delete[] prgs;
}


gl::program *render_output::select_program(bool normals)
{
    bool tl = lighting && light_dir.length();

    gl::program *selected = &prgs[(tl      * LIGHTING) |
                                  (normals * NORMALS)  |
                                  (colored * COLORED)];

    if (selected != current_prg) {
        selected->use();
    }

    return current_prg = selected;
}


void render_output::invalidate(void)
{
    reload_uniforms = true;
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


void render_output::change_fov(double fov_deg)
{
    fov = static_cast<float>(fov_deg) * static_cast<float>(M_PI) / 180.f;

    resizeGL(w, h);
}


void render_output::initializeGL(void)
{
    gl::glext_init();

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


    // (╯°□°）╯︵ ┻━┻
    shader *shaders = static_cast<shader *>(malloc(shader_source_count * sizeof(shader)));

    for (size_t i = 0; i < shader_source_count; i++) {
        if ((shader_sources[i].flags < PROGRAM_COUNT) && (shader_sources[i].flags & ~shader_flag_mask(shader_sources[i]))) {
            throw std::logic_error(std::string("Invalid flags given for ") + shader_sources[i].description);
        }

        // I sure hope I'll burn for this
        (new (&shaders[i]) shader(shader_sources[i]))->source(shader_sources[i]);
        if (!shaders[i].compile()) {
            throw std::logic_error(std::string("Failed to compile ") + shader_sources[i].description);
        }
    }


    prgs = new gl::program[PROGRAM_COUNT];

    for (int i = 0; i < PROGRAM_COUNT; i++) {
        // Kind of FIXME, but probably not worth the trouble
        for (size_t j = 0; j < shader_source_count; j++) {
            if ((shader_sources[j].flags < PROGRAM_COUNT) &&
               ((shader_sources[j].flags & shader_flag_mask(shader_sources[j])) ==
                (i                       & shader_flag_mask(shader_sources[j]))))
            {
                prgs[i] << shaders[j];
            }
        }

        prgs[i].bind_attrib("in_position", 0);
        prgs[i].bind_attrib("in_normal", 1);
        prgs[i].bind_attrib("in_color", 2);

        prgs[i].bind_frag("out_color", 0);

        if (!prgs[i].link()) {
            throw std::logic_error("Could not link program");
        }
    }

    rng_prg = new gl::program;
    for (size_t i = 0; i < shader_source_count; i++) {
        if (shader_sources[i].flags == RNG) {
            *rng_prg << shaders[i];
        }
    }
    rng_prg->bind_attrib("in_position", 0);
    rng_prg->bind_frag("out_color", 0);

    if (!rng_prg->link()) {
        throw std::logic_error("Could not compile RNG program");
    }

    // (๑′ᴗ'๑)ｴﾍﾍ
    for (size_t i = 0; i < shader_source_count; i++) {
        shaders[i].~shader();
    }
    free(shaders);

    redraw_timer->start(0);
}


void render_output::resizeGL(int wdt, int hgt)
{
    w = wdt;
    h = hgt;

    glViewport(0, 0, w, h);

    proj = mat4::projection(fov, static_cast<float>(w) / h, .1f, 100.f);

    gl::program *cur = current_prg;

    for (int i = 0; i < PROGRAM_COUNT; i++) {
        prgs[i].uniform<mat4>("proj") = proj;
    }
    rng_prg->uniform<mat4>("proj") = proj;

    if (cur) {
        cur->use();
    }
}


extern cloud_manager cm;


void render_output::paintGL(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    mat4 mv;
    mat3 norm;

    bool multicloud = cm.clouds().size() > 1;

    for (cloud &c: cm.clouds()) {
        if (reload_uniforms || multicloud) {
            mv = this->mv * c.transformation();
            norm = mv;
            norm.transposed_invert();
        }

        gl::program *prg = select_program(false);
        if (reload_uniforms || multicloud) {
            prg->uniform<mat4>("mv") = mv;
            if (lighting && light_dir.length()) {
                prg->uniform<mat3>("nmat") = norm;
                if (reload_uniforms) {
                    prg->uniform<vec3>("light_dir") = mat3(mv) * light_dir.normalized();
                }
            }
        }

        c.vertex_array()->draw(GL_POINTS);


        if (normlen) {
            prg = select_program(true);
            if (reload_uniforms || multicloud) {
                prg->uniform<mat4>("mv") = mv;
                prg->uniform<mat3>("nmat") = norm;
                if (reload_uniforms) {
                    prg->uniform<float>("normal_scale") = normlen;
                }
            }

            c.vertex_array()->draw(GL_POINTS);
        }

        if (show_rng) {
            rng_prg->use();
            if (reload_uniforms || multicloud) {
                rng_prg->uniform<mat4>("mv") = mv;
            }

            c.rng_vertex_array(rng_k)->draw(GL_LINES);
        }
    }

    reload_uniforms = false;

    gl::program::unuse();
    current_prg = nullptr;
}


void render_output::mousePressEvent(QMouseEvent *evt)
{
    if ((evt->button() == Qt::LeftButton) && !move_camera) {
        grabMouse(Qt::ClosedHandCursor);

        rotate_camera = true;

        rot_l_x = evt->x();
        rot_l_y = evt->y();
    } else if ((evt->button() == Qt::RightButton) && !rotate_camera) {
        grabMouse(Qt::ClosedHandCursor);

        move_camera = true;

        rot_l_x = evt->x();
        rot_l_y = evt->y();
    }
}


void render_output::mouseReleaseEvent(QMouseEvent *evt)
{
    if ((evt->button() == Qt::LeftButton) && rotate_camera) {
        releaseMouse();
        rotate_camera = false;
    } else if ((evt->button() == Qt::RightButton) && move_camera) {
        releaseMouse();
        move_camera = false;
    }
}


void render_output::mouseMoveEvent(QMouseEvent *evt)
{
    // No, actually I don't even want to know.
#ifdef __MINGW32__
    int dx = evt->x() - rot_l_x;
    int dy = evt->y() - rot_l_y;
#else
    int dx = rot_l_x - evt->x();
    int dy = rot_l_y - evt->y();
#endif

    if (rotate_camera) {
        mv = mat4::identity().rotated(dx / 4.f * static_cast<float>(M_PI) / 180.f, vec3(0.f, 1.f, 0.f)) * mv;
        mv = mat4::identity().rotated(dy / 4.f * static_cast<float>(M_PI) / 180.f, vec3(1.f, 0.f, 0.f)) * mv;
    } else {
        mv = mat4::identity().translated(vec3(dx / 100.f, -dy / 100.f, 0.f)) * mv;
    }

    reload_uniforms = true;

    rot_l_x = evt->x();
    rot_l_y = evt->y();
}


void render_output::wheelEvent(QWheelEvent *evt)
{
    mv = mat4::identity().translated(vec3(0.f, 0.f, evt->delta() / 360.f)) * mv;

    reload_uniforms = true;
}
