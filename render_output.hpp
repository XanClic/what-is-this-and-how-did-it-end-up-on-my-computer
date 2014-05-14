#ifndef RENDER_OUTPUT_HPP
#define RENDER_OUTPUT_HPP

#include <QGLWidget>
#include <QDoubleSpinBox>
#include <QTimer>
#include <dake/math/matrix.hpp>
#include <dake/gl/shader.hpp>


class render_output:
    public QGLWidget
{
    Q_OBJECT

    public:
        render_output(QGLFormat fmt, QDoubleSpinBox *point_size_widget, QWidget *parent = nullptr);
        ~render_output(void);

        dake::gl::program *active_program(void)
        { return prg; }

        const dake::math::mat4 &projection(void) const
        { return proj; }
        dake::math::mat4 &projection(void)
        { return proj; }

    public slots:
        void change_point_size(double sz);
        void change_point_smoothness(int smooth);

    protected:
        void initializeGL(void);
        void resizeGL(int w, int h);
        void paintGL(void);

    private:
        QDoubleSpinBox *psw;
        dake::gl::program *prg;
        QTimer *redraw_timer;
        dake::math::mat4 proj;
};

#endif
