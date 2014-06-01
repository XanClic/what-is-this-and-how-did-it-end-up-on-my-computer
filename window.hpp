#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <QMainWindow>
#include <QBoxLayout>
#include <QCheckBox>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QComboBox>

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
        void unload_cloud(void);
        void do_cull(void);
        void recalc_normals(void);

    private:
        QWidget *i_hate_qt;

        render_output *gl;
        QHBoxLayout *l1, *ldl;
        QVBoxLayout *l2;
        QFrame *f[6];
        QCheckBox *smooth_points, *lighting, *colored, *rng, *renormal_inv;
        QDoubleSpinBox *point_size, *normal_length, *fov, *ld_x, *ld_y, *ld_z, *cull_ratio;
        QLabel *point_size_label, *normal_length_label, *fov_label, *ld, *k_label, *cull_ratio_label;
        QPushButton *unify, *load, *store, *unload, *cull, *renormal;
        QDoubleSpinBox *unify_res;
        QComboBox *clouds;
        QSpinBox *k;
};


void init_progress(const char *format, int max);
void announce_progress(int amount);
void reset_progress(void);

#endif
