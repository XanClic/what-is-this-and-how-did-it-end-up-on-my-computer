#ifndef RENDER_OUTPUT_HPP
#define RENDER_OUTPUT_HPP

#include <QGLWidget>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>


class render_output:
    public QGLWidget
{
    Q_OBJECT

    public:
        render_output(QGLFormat fmt, QDoubleSpinBox *point_size_widget, QWidget *parent = nullptr);
        ~render_output(void);

        dake::gl::program *select_program(bool normals);

        bool lighting_enabled(void) const
        { return lighting; }
        void enable_lighting(bool enable)
        { lighting = enable; reload_uniforms = true; }

        bool colors_enabled(void) const
        { return colored; }
        void enable_colors(bool enable)
        { colored = enable; reload_uniforms = true; }

        float normal_length(void) const
        { return normlen; }
        void set_normal_length(float len)
        { normlen = len; reload_uniforms = true; }

        const dake::math::mat4 &projection(void) const
        { return proj; }
        dake::math::mat4 &projection(void)
        { return proj; }

        const dake::math::mat4 &modelview(void) const
        { return mv; }
        dake::math::mat4 &modelview(void)
        { reload_uniforms = true; return mv; }

        const dake::math::vec3 &light_direction(void) const
        { return light_dir; }
        dake::math::vec3 &light_direction(void)
        { reload_uniforms = true; return light_dir; }

    public slots:
        void change_point_size(double sz);
        void change_point_smoothness(int smooth);
        void change_lighting(int lighting) { enable_lighting(lighting); }
        void change_colors(int colored) { enable_colors(colored); }
        void change_normal_length(double length) { set_normal_length(length); }
        void change_fov(double fov_deg);
        void change_ld_x(double ld_x) { light_direction().x() = ld_x; }
        void change_ld_y(double ld_y) { light_direction().y() = ld_y; }
        void change_ld_z(double ld_z) { light_direction().z() = ld_z; }
        void toggle_rng(int state) { reload_uniforms = true; show_rng = state; }
        void set_rng_k(int k) { reload_uniforms = true; rng_k = k; }

    protected:
        void initializeGL(void);
        void resizeGL(int w, int h);
        void paintGL(void);
        void mousePressEvent(QMouseEvent *evt);
        void mouseReleaseEvent(QMouseEvent *evt);
        void mouseMoveEvent(QMouseEvent *evt);
        void wheelEvent(QWheelEvent *evt);

    private:
        QDoubleSpinBox *psw;
        dake::gl::program *prgs = nullptr, *current_prg = nullptr, *rng_prg = nullptr;
        QTimer *redraw_timer;
        dake::math::mat4 proj, mv;
        dake::math::vec3 light_dir;
        bool lighting = false, colored = true, rotate_camera = false, move_camera = false;
        bool reload_uniforms = true;
        float normlen = 0.f;
        float rot_l_x, rot_l_y;
        float fov = static_cast<float>(M_PI) / 4.f;
        int w, h;
        bool show_rng = false;
        int rng_k = 5;

        void recalc_rng(void);
};

#endif
