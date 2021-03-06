#include <dake/gl/gl.hpp>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <QApplication>
#include <QGLWidget>
#include <QBoxLayout>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QComboBox>
#include <QProgressBar>
#include <QScrollArea>

#include "cloud.hpp"
#include "render_output.hpp"
#include "window.hpp"


extern cloud_manager cm;


// GO ALONG THERE'S NOTHING TO BE SEEN HERE
QProgressBar *global_progress;
static int minimum_progress;

void init_progress(const char *format, int max)
{
    global_progress->reset();
    global_progress->setFormat(format);
    global_progress->setRange(0, max);

    minimum_progress = max / 100;
}

void announce_progress(int amount)
{
    static int last_update;

    if (abs(amount - last_update) >= minimum_progress) {
        global_progress->setValue(last_update = amount);
        qApp->processEvents();
    }
}

void reset_progress(void)
{
    global_progress->reset();
}
// oh hi there welcome back


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

    load = new QPushButton("Load new cloud");
    clouds = new QComboBox;
    for (const cloud &c: cm.clouds()) {
        // I hate Qt so incredibly much (they are extreme idiots)
        // (this would be very short with a C style cast, but I have to suffer
        // because of Qt's extreme stupidity and so do you)
        clouds->addItem(c.name().c_str(), static_cast<qulonglong>(reinterpret_cast<uintptr_t>(const_cast<cloud *>(&c))));

        // As a funny side note, all these notices referring to how much I hate
        // Qt (and I do with a passion) are within this single file; however,
        // they have not been written immediately after each other: The first
        // note above ogl_version[] is from late 2013, the variable name
        // i_hate_qt is from 2014-05-14, and this comment is from 2014-05-17.
        //
        // In case of ogl_version, I just don't understand why they're incapable
        // of giving me an easy way of just finding out the highest supported
        // OpenGL version; however, in that case, Mesa is to blame as well, as
        // they are in turn incapable of just supporting Compatibility Contexts,
        // because they are more lazy than the average computer science student
        // at TU Dresden. With them putting some effort into their work, I
        // wouldn't have the problem in the first place (I'd just get my 3.3
        // context, would not use Compatibility stuff and everyone would be
        // happy).
        //
        // In case of i_hate_qt, I just don't understand why I'm forced to
        // create a Widget in a Widget just to be able to use the latter. It is
        // pure idiocy and cost me an hour at least to figure out that they
        // somehow thought they had to change the behavior from Qt4, where one
        // could just create a QMainWindow and set its layout. Good times.
        //
        // And in this case, I just don't know why they're so extremely
        // incapable of interoperating. Yeah, I know, the STL is shit, and C is,
        // too. But I don't think so. I'd like to use both when appropriate and
        // I'd love my framework to handle that with grace. Why can't I
        // construct a QString from std::string?* Why can't I cast QString to
        // std::string implicitly? Why don't they support pointers to arbitrary
        // types in QVariant? Or just void pointers?*& I KNOW WHAT I'M DOING
        // THAT'S WHY I'M WRITING C++ IN THE FIRST PLACE THANK YOU VERY MUCH
        // oh god qt is so bad
        //
        // i just wanted a nice library for a gui
        // why is qt such a bully
        // plz help
        // i want my gtk back
        // and i was always a nice kde user
        // always
        // i just want gtk back
        // qt is such a bully
        // they want to take everything away from me
        // everything which is nice
        // this is why we can't have nice things
        // qt is a weapon of mass destruction
        // o plz obama plz command a drone strike on this pile of terrorism
        //
        // i hate frameworks
        // i just want a gui library
        // just give me windows and buttons and stuff
        // i don't need your strings
        // i don't need your fancy variants
        // i just want buttons and stuff
        // (´；ω；｀)ｳｯ…
        //
        // もう嫌だ
        // MOOOUUU IYA DAAAAAaaaa~~~~
        //
        //
        //
        // *Actually, I found out I can: QString::fromStdString(), but the
        // internet says this is bad (because it is optional).
        //
        // **Yes, I have indeed been told that Qt does support this in two ways:
        //  (1) add your custom type to the Qt type system - I want a library, I
        //      have been told one could use Qt as a library, therefore I will
        //      use it as a library and _not_ incorporate my types into its
        //      type system
        //  (2) give a custom type identifier to the QVariant constructor for a
        //      void pointer - I understand the idea, but I refuse to do this
        //      until they finally get the idea of just giving a default for
        //      type ID
        // Also, being a magical gir—— errrr, using Qt is suffering, so now now
        // you at least get to suffer with me whenever I'm doing these horrible
        // casts for QVariant.
        // (╯°□°）╯︵ ┻━┻
    }
    // (buttons and stuff)
    store = new QPushButton("Store this cloud");
    unload = new QPushButton("Unload this cloud");
    rnd_trans = new QPushButton("Randomize transformations");
    unify = new QPushButton("Unify all clouds");
    unify_res = new QDoubleSpinBox;
    unify_res->setDecimals(4);
    unify_res->setRange(.0001, HUGE_VAL);
    unify_res->setSingleStep(0.01);
    unify_res->setValue(0.01);
    smooth_points = new QCheckBox("Smooth points");
    point_size_label = new QLabel("Point size:");
    point_size = new QDoubleSpinBox;
    colored = new QCheckBox("Colored");
    colored->setCheckState(Qt::Checked);
    lighting = new QCheckBox("Lighting");
    ld = new QLabel("Light direction:");
    ld_x = new QDoubleSpinBox;
    ld_x->setSingleStep(.1);
    ld_x->setRange(-HUGE_VAL, HUGE_VAL);
    ld_y = new QDoubleSpinBox;
    ld_y->setSingleStep(.1);
    ld_y->setRange(-HUGE_VAL, HUGE_VAL);
    ld_z = new QDoubleSpinBox;
    ld_z->setSingleStep(.1);
    ld_z->setRange(-HUGE_VAL, HUGE_VAL);
    normal_length_label = new QLabel("Normal length:");
    normal_length = new QDoubleSpinBox;
    normal_length->setSingleStep(.1);
    normal_length->setRange(-HUGE_VAL, HUGE_VAL);
    renormal = new QPushButton("Recalculate normals");
    renormal_inv = new QCheckBox("Invert normals");
    fov_label = new QLabel("FOV:");
    fov = new QDoubleSpinBox;
    fov->setRange(.01, 179.99);
    fov->setValue(45.);
    cull = new QPushButton("Cull outliers");
    cull_ratio_label = new QLabel("Percentage to cull:");
    cull_ratio = new QDoubleSpinBox;
    cull_ratio->setRange(0., 100.);
    cull_ratio->setValue(10.);
    k_label = new QLabel("k (neighbor count):");
    k = new QSpinBox;
    k->setRange(1, INT_MAX);
    k->setValue(5);
    rng = new QCheckBox("Riemann graph");
    icp = new QPushButton("ICP");
    icp_n_label = new QLabel("N (reg. point count):");
    icp_n = new QSpinBox;
    icp_n->setRange(1, INT_MAX);
    icp_n->setValue(42);
    icp_p_label = new QLabel("p (cull percentage):");
    icp_p = new QDoubleSpinBox;
    icp_p->setRange(0, .99);
    icp_p->setSingleStep(.01);
    icp_p->setValue(.42);
    icp_m_label = new QLabel("M (iterations):");
    icp_m = new QSpinBox;
    icp_m->setRange(1, INT_MAX);
    icp_m->setValue(42);

    global_progress = new QProgressBar;
    global_progress->setTextVisible(true);

    for (QFrame *&fr: f) {
        fr = new QFrame;
        fr->setFrameShape(QFrame::HLine);
    }

    ldl = new QHBoxLayout;
    ldl->addWidget(ld_x);
    ldl->addWidget(ld_y);
    ldl->addWidget(ld_z);

    l2 = new QVBoxLayout;
    l2->addWidget(load);
    l2->addWidget(clouds);
    l2->addWidget(store);
    l2->addWidget(unload);
    l2->addWidget(rnd_trans);
    l2->addWidget(unify);
    l2->addWidget(unify_res);
    l2->addWidget(f[0]);
    l2->addWidget(smooth_points);
    l2->addWidget(point_size_label);
    l2->addWidget(point_size);
    l2->addWidget(f[1]);
    l2->addWidget(colored);
    l2->addWidget(lighting);
    l2->addWidget(ld);
    l2->addLayout(ldl);
    l2->addWidget(f[2]);
    l2->addWidget(normal_length_label);
    l2->addWidget(normal_length);
    l2->addWidget(renormal);
    l2->addWidget(renormal_inv);
    l2->addWidget(f[3]);
    l2->addWidget(fov_label);
    l2->addWidget(fov);
    l2->addWidget(f[4]);
    l2->addWidget(cull);
    l2->addWidget(cull_ratio_label);
    l2->addWidget(cull_ratio);
    l2->addWidget(f[5]);
    l2->addWidget(k_label);
    l2->addWidget(k);
    l2->addWidget(rng);
    l2->addWidget(f[6]);
    l2->addWidget(icp);
    l2->addWidget(icp_n_label);
    l2->addWidget(icp_n);
    l2->addWidget(icp_p_label);
    l2->addWidget(icp_p);
    l2->addWidget(icp_m_label);
    l2->addWidget(icp_m);

    int highest = fls(QGLFormat::openGLVersionFlags());
    if (!highest)
        throw std::runtime_error("No OpenGL support");

    int version = ogl_version[highest - 1];

    QGLFormat fmt;
    fmt.setProfile(QGLFormat::CoreProfile);
    fmt.setVersion(version >> 4, version & 0xf);

    if (((version >> 4) < 3) || (((version >> 4) == 3) && ((version & 0xf) < 3))) {
        throw std::runtime_error("OpenGL 3.3+ is required");
    }

    printf("Selecting OpenGL %i.%i Core\n", version >> 4, version & 0xf);

    gl = new render_output(fmt, point_size);
    connect(smooth_points, SIGNAL(stateChanged(int)), gl, SLOT(change_point_smoothness(int)));
    connect(point_size, SIGNAL(valueChanged(double)), gl, SLOT(change_point_size(double)));
    connect(colored, SIGNAL(stateChanged(int)), gl, SLOT(change_colors(int)));
    connect(lighting, SIGNAL(stateChanged(int)), gl, SLOT(change_lighting(int)));
    connect(ld_x, SIGNAL(valueChanged(double)), gl, SLOT(change_ld_x(double)));
    connect(ld_y, SIGNAL(valueChanged(double)), gl, SLOT(change_ld_y(double)));
    connect(ld_z, SIGNAL(valueChanged(double)), gl, SLOT(change_ld_z(double)));
    connect(normal_length, SIGNAL(valueChanged(double)), gl, SLOT(change_normal_length(double)));
    connect(fov, SIGNAL(valueChanged(double)), gl, SLOT(change_fov(double)));
    connect(rng, SIGNAL(stateChanged(int)), gl, SLOT(toggle_rng(int)));
    connect(k, SIGNAL(valueChanged(int)), gl, SLOT(set_rng_k(int)));

    connect(unify, SIGNAL(pressed()), this, SLOT(do_unify()));
    connect(load, SIGNAL(pressed()), this, SLOT(load_cloud()));
    connect(store, SIGNAL(pressed()), this, SLOT(store_cloud()));
    connect(unload, SIGNAL(pressed()), this, SLOT(unload_cloud()));
    connect(rnd_trans, SIGNAL(pressed()), this, SLOT(randomize_transformations()));
    connect(cull, SIGNAL(pressed()), this, SLOT(do_cull()));
    connect(renormal, SIGNAL(pressed()), this, SLOT(recalc_normals()));
    connect(icp, SIGNAL(pressed()), this, SLOT(do_icp()));

    options_widget = new QWidget;
    options_widget->setLayout(l2);

    options = new QScrollArea;
    options->setWidget(options_widget);
    options->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    options->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    options->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);

    l3 = new QVBoxLayout;
    l3->addWidget(options, 1);
    l3->addStretch();
    l3->addWidget(global_progress);

    options_widget = new QWidget;
    options_widget->setLayout(l2);

    options = new QScrollArea;
    options->setWidget(options_widget);
    options->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    options->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    options->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);

    l3 = new QVBoxLayout;
    l3->addWidget(options, 1);
    l3->addWidget(global_progress);

    l1 = new QHBoxLayout;
    l1->addWidget(gl, 1);
    l1->addLayout(l3);

    i_hate_qt->setLayout(l1);


    cm.set_render_output(gl);
}

window::~window(void)
{
    cm.set_render_output(nullptr);


    delete l1;
    delete l2;
    delete l3;
    delete global_progress;
    delete options;
    delete options_widget;
    delete ldl;
    delete gl;
    delete icp_m;
    delete icp_m_label;
    delete icp_p;
    delete icp_p_label;
    delete icp_n;
    delete icp_n_label;
    delete icp;
    delete rng;
    delete k;
    delete k_label;
    delete cull_ratio;
    delete cull_ratio_label;
    delete cull;
    delete fov;
    delete fov_label;
    delete renormal_inv;
    delete renormal;
    delete normal_length;
    delete normal_length_label;
    delete ld_z;
    delete ld_y;
    delete ld_x;
    delete ld;
    delete lighting;
    delete colored;
    delete point_size;
    delete point_size_label;
    delete smooth_points;
    delete unify_res;
    delete unify;
    delete rnd_trans;
    delete unload;
    delete store;
    delete clouds;
    delete load;
    delete i_hate_qt;

    for (QFrame *fr: f) {
        delete fr;
    }
}


void window::do_unify(void)
{
    if (cm.clouds().empty()) {
        return;
    }

    clouds->clear();

    try {
        cm.unify(unify_res->value(), "Unification");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Could not unify: ") + QString(e.what()));
    }

    for (const cloud &c: cm.clouds()) {
        // do you feel the suffering
        clouds->addItem(c.name().c_str(), static_cast<qulonglong>(reinterpret_cast<uintptr_t>(const_cast<cloud *>(&c))));
    }

    gl->invalidate();
}


void window::load_cloud(void)
{
    QStringList paths = QFileDialog::getOpenFileNames(this,
                                                      "Load point cloud",
                                                      QString(),
                                                      "Polygon files (*.ply);;All files (*.*)");

    for (const auto &path: paths) {
        std::ifstream inp(path.toUtf8().constData());
        if (!inp.is_open()) {
            QMessageBox::critical(this, "Could not open file", QString("Could not open ") + path + QString(": ") + QString(strerror(errno)));
            // > errno
            // > strerror
            // > C++
            // i haet u too, stl
            // qt, plz
            continue;
        }

        try {
            cm.load_new(inp, QFileInfo(path).fileName().toUtf8().constData());
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Error", QString("Could not load from ") + path + QString(": ") + QString(e.what()));
        }
    }

    clouds->clear();

    for (const cloud &c: cm.clouds()) {
        // let it cleanse your sins
        clouds->addItem(c.name().c_str(), static_cast<qulonglong>(reinterpret_cast<uintptr_t>(const_cast<cloud *>(&c))));
    }

    gl->invalidate();
}


void window::store_cloud(void)
{
    if (clouds->currentIndex() < 0) {
        return;
    }

    QString path = QFileDialog::getSaveFileName(this,
                                                "Store point cloud",
                                                QString(),
                                                "Polygon files (*.ply);;All files (*.*)");

    std::ofstream out(path.toUtf8().constData());
    if (!out.is_open()) {
        QMessageBox::critical(this, "Could not write to file", QString("Could not write to file ") + path + QString(": ") + QString(strerror(errno)));
        return;
    }

    // jesus died for you
    // and i died of qt
    // (what am i even talking about)
    const cloud *c = reinterpret_cast<const cloud *>(static_cast<uintptr_t>(clouds->currentData().value<qulonglong>()));

    try {
        c->store(out);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Could not store ") + QString(c->name().c_str()) + QString(": ") + QString(e.what()));
    }

    // accept jesus
}


void window::unload_cloud(void)
{
    if (clouds->currentIndex() < 0) {
        return;
    }

    // The above code with the Jesus stuff and whatnot is two weeks old by now.
    // I have matured in the meantime and outgrown such lowly jokes.
    const cloud *c = reinterpret_cast<const cloud *>(static_cast<uintptr_t>(clouds->currentData().value<qulonglong>()));

    for (auto it = cm.clouds().begin(); it != cm.clouds().end(); ++it) {
        if (&*it == c) {
            cm.clouds().erase(it);
            clouds->removeItem(clouds->currentIndex());
            gl->invalidate();
            return;
        }
    }

    QMessageBox::critical(this, "Error", QString("Could not find ") + QString(c->name().c_str()) + QString(" in the list of clouds."));


    // but seriously, wtf qt
}


void window::do_cull(void)
{
    float ratio = static_cast<float>(cull_ratio->value()) / 100.f;
    int kv = k->value();

    for (cloud &c: cm.clouds()) {
        try {
            c.cull_outliers(ratio, kv);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Error", QString("Could not cull outliers from ") + QString(c.name().c_str()) + QString(": ") + QString(e.what()));
        }
    }

    gl->invalidate();
}


void window::recalc_normals(void)
{
    bool inv = renormal_inv->checkState() == Qt::Checked;
    int kv = k->value();

    for (cloud &c: cm.clouds()) {
        try {
            c.recalc_normals(kv, inv);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, "Error", QString("Could not recalculate the normals for ") + QString(c.name().c_str()) + QString(": ") + QString(e.what()));
        }
    }

    gl->invalidate();
}


void window::do_icp(void)
{
    try {
        cm.icp(icp_m->value(), icp_n->value(), icp_p->value());
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Could not do ICP: ") + QString(e.what()));
    }
}


void window::randomize_transformations(void)
{
    cm.randomize_transformations();
}
