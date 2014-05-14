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
        { lighting = enable; }

        bool colors_enabled(void) const
        { return colored; }
        void enable_colors(bool enable)
        { colored = enable; }

        float normal_length(void) const
        { return normlen; }
        void set_normal_length(float len)
        { normlen = len; }

        const dake::math::mat4 &projection(void) const
        { return proj; }
        dake::math::mat4 &projection(void)
        { return proj; }

        const dake::math::mat4 &modelview(void) const
        { return mv; }
        dake::math::mat4 &modelview(void)
        { return mv; }

    public slots:
        void change_point_size(double sz);
        void change_point_smoothness(int smooth);
        void change_lighting(int lighting) { enable_lighting(lighting); }
        void change_colors(int colored) { enable_colors(colored); }
        void change_normal_length(double length) { set_normal_length(length); }
        void change_fov(double fov_deg);

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
        dake::gl::program *prgs = nullptr, *current_prg = nullptr;
        QTimer *redraw_timer;
        dake::math::mat4 proj, mv;
        bool lighting = false, colored = true, rotate_camera = false, move_camera = false;
        float normlen = 0.f;
        float rot_l_x, rot_l_y;
        float fov = static_cast<float>(M_PI) / 4.f;
        int w, h;

        void recalc_projection(float aspect, float fov);
};

#endif
