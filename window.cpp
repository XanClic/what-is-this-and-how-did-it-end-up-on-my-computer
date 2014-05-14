#include <cmath>
#include <QGLWidget>
#include <QBoxLayout>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QLabel>

#include "render_output.hpp"
#include "window.hpp"

static int fls(int mask)
{
    int i = 0;

    while (mask) {
        i++;
        mask >>= 1;
    }

    return i;
}


// Thanks to Qt quality (they are idiots)
static const int ogl_version[] = {
    0x11, 0x12, 0x13, 0x14, 0x15, 0x20, 0x21, 0x21,
    0x21, 0x21, 0x21, 0x21, 0x30, 0x31, 0x32, 0x33,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x44, 0x44,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
};


window::window(void):
    QMainWindow(nullptr)
{
    i_hate_qt = new QWidget(this);
    setCentralWidget(i_hate_qt);

    smooth_points = new QCheckBox("Smooth points");
    point_size_label = new QLabel("Point size:");
    point_size = new QDoubleSpinBox;
    lighting = new QCheckBox("Lighting");
    colored = new QCheckBox("Colored");
    colored->setCheckState(Qt::Checked);
    normal_length_label = new QLabel("Normal length:");
    normal_length = new QDoubleSpinBox;
    normal_length->setSingleStep(.1);
    normal_length->setRange(-HUGE_VAL, HUGE_VAL);
    fov_label = new QLabel("FOV:");
    fov = new QDoubleSpinBox;
    fov->setValue(45.);
    fov->setRange(.01, 179.99);

    l2 = new QVBoxLayout;
    l2->addWidget(smooth_points);
    l2->addWidget(point_size_label);
    l2->addWidget(point_size);
    l2->addWidget(lighting);
    l2->addWidget(colored);
    l2->addWidget(normal_length_label);
    l2->addWidget(normal_length);
    l2->addWidget(fov_label);
    l2->addWidget(fov);
    l2->addStretch();

    int highest = fls(QGLFormat::openGLVersionFlags());
    if (!highest)
        throw std::runtime_error("No OpenGL support");

    int version = ogl_version[highest - 1];

    QGLFormat fmt;
    fmt.setProfile(QGLFormat::CoreProfile);
    fmt.setVersion(version >> 4, version & 0xf);

    printf("Selecting OpenGL %i.%i Core\n", version >> 4, version & 0xf);

    gl = new render_output(fmt, point_size);
    connect(smooth_points, SIGNAL(stateChanged(int)), gl, SLOT(change_point_smoothness(int)));
    connect(point_size, SIGNAL(valueChanged(double)), gl, SLOT(change_point_size(double)));
    connect(lighting, SIGNAL(stateChanged(int)), gl, SLOT(change_lighting(int)));
    connect(colored, SIGNAL(stateChanged(int)), gl, SLOT(change_colors(int)));
    connect(normal_length, SIGNAL(valueChanged(double)), gl, SLOT(change_normal_length(double)));
    connect(fov, SIGNAL(valueChanged(double)), gl, SLOT(change_fov(double)));

    l1 = new QHBoxLayout;
    l1->addWidget(gl, 1);
    l1->addLayout(l2);

    i_hate_qt->setLayout(l1);
}

window::~window(void)
{
    delete l1;
    delete l2;
    delete gl;
    delete fov;
    delete fov_label;
    delete normal_length;
    delete normal_length_label;
    delete colored;
    delete lighting;
    delete point_size;
    delete point_size_label;
    delete smooth_points;
    delete i_hate_qt;
}
