#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QCheckBox>
#include <QWidget>
#include <QDoubleSpinBox>

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
        QCheckBox *smooth_points;
        QDoubleSpinBox *point_size;
};

#endif
