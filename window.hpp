#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QCheckBox>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QLabel>

#include "render_output.hpp"


class window:
    public QMainWindow
{
    Q_OBJECT

    public:
        window(void);
        ~window(void);

        render_output *renderer(void)
        { return gl; }

    private:
        QWidget *i_hate_qt;

        render_output *gl;
        QHBoxLayout *l1;
        QVBoxLayout *l2;
        QCheckBox *smooth_points, *lighting, *colored;
        QDoubleSpinBox *point_size, *normal_length, *fov;
        QLabel *point_size_label, *normal_length_label, *fov_label;
};

#endif
