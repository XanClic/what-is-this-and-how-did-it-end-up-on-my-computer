#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QCheckBox>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

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

    public slots:
        void do_unify(void);
        void load_cloud(void);
        void store_cloud(void);

    private:
        QWidget *i_hate_qt;

        render_output *gl;
        QHBoxLayout *l1, *ldl;
        QVBoxLayout *l2;
        QFrame *f[5];
        QCheckBox *smooth_points, *lighting, *colored, *rng;
        QDoubleSpinBox *point_size, *normal_length, *fov, *ld_x, *ld_y, *ld_z;
        QLabel *point_size_label, *normal_length_label, *fov_label, *ld, *rng_k_label;
        QPushButton *unify, *load, *store;
        QDoubleSpinBox *unify_res;
        QComboBox *clouds;
        QSpinBox *rng_k;
};

#endif
