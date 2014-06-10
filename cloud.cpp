#include <dake/gl/gl.hpp>

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <list>
#include <queue>
#include <random>
#include <stdexcept>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <dake/math/matrix.hpp>
#include <dake/gl/vertex_array.hpp>
#include <dake/gl/vertex_attrib.hpp>
#include <dake/container/algorithm.hpp>
#include <dake/helper/function.hpp>
#include <Eigen/Eigenvalues>

#include "cloud.hpp"
#include "kd_tree.hpp"
#include "point.hpp"
#include "rng.hpp"
#include "window.hpp"


#ifndef M_PI
#define M_PI 3.141592
#endif


using namespace dake::math;
using namespace dake::gl;
using namespace dake::container;
using namespace dake::helper;


namespace std
{

template<> struct hash<std::pair<int, int>> {
    typedef std::pair<int, int> argument_type;
    typedef size_t value_type;

    value_type operator()(const argument_type &v) const
    {
        // I tried
        return static_cast<value_type>(int_hash(v.first))
             + static_cast<value_type>(int_hash(v.second)) * 524287;
    }


    private:
        std::hash<int> int_hash;
};

}


cloud::cloud(const std::string &name):
    trans(mat4::identity()),
    n(name)
{
}


cloud::~cloud(void)
{
    delete varr;
    delete rng_varr;
}


static bool crlf_getline(std::ifstream &s, std::string &str)
{
    if (!std::getline(s, str))
        return false;

    if (str.back() == '\r')
        str.pop_back();

    return true;
}


struct property {
    enum {
        FLOAT,
        UCHAR
    } type;
    std::string name;
};

void cloud::load(std::ifstream &s)
{
    std::string line;
    int vertices = -1;
    std::list<property> properties;


    crlf_getline(s, line);
    if (line != "ply")
        throw std::invalid_argument("File is not in PLY format");

    while (crlf_getline(s, line)) {
        if (line.empty())
            continue;

        std::stringstream line_ss(line);

        std::string cmd;
        line_ss >> cmd;

        if (cmd == "format") {
            std::string ftype;
            line_ss >> ftype;
            if (ftype != "ascii")
                throw std::invalid_argument("File is not in ASCII format");

            float fver;
            line_ss >> fver;
            if (fver != 1.f)
                throw std::invalid_argument("Invalid PLY version");
        } else if (cmd == "element") {
            if (vertices >= 0)
                throw std::invalid_argument("Multiple element definitions");

            std::string etype;
            line_ss >> etype;
            if (etype != "vertex")
                throw std::invalid_argument("Non-vertex type");

            line_ss >> vertices;
            if (vertices < 0)
                throw std::invalid_argument("Vertex count negative");
        } else if (cmd == "property") {
            if (vertices < 0)
                throw std::invalid_argument("property misplaced (no element found yet)");

            property prop;

            std::string type;
            line_ss >> type;
            if (type == "float") {
                prop.type = property::FLOAT;
            } else if (type == "uchar") {
                prop.type = property::UCHAR;
            } else {
                throw std::invalid_argument("Unknown property type");
            }

            line_ss >> prop.name;
            properties.push_back(prop);
        } else if (cmd == "end_header") {
            break;
        } else if (cmd == "comment") {
            continue;
        } else {
            throw std::invalid_argument("Unknown header command");
        }
    }


    if (vertices < 0)
        throw std::invalid_argument("Vertex count unspecified");

    if (properties.empty())
        throw std::invalid_argument("Properties unspecified");


    while ((vertices-- > 0) && crlf_getline(s, line)) {
        std::stringstream line_ss(line);
        point pt;

        pt.position = vec3::zero();
        pt.normal   = vec3::zero();
        pt.color    = vec3(1.f, 1.f, 1.f);

        for (const auto &prop: properties) {
            float real_val;

            switch (prop.type) {
                case property::FLOAT: {
                    line_ss >> real_val;
                    break;
                }

                case property::UCHAR: {
                    int val;
                    line_ss >> val;
                    real_val = val / 255.f;
                    break;
                }
            }

            if (prop.name == "x")          pt.position.x() = real_val;
            else if (prop.name == "y")     pt.position.y() = real_val;
            else if (prop.name == "z")     pt.position.z() = real_val;
            else if (prop.name == "nx")    pt.normal.x()   = real_val;
            else if (prop.name == "ny")    pt.normal.y()   = real_val;
            else if (prop.name == "nz")    pt.normal.z()   = real_val;
            else if (prop.name == "red")   pt.color.r()    = real_val;
            else if (prop.name == "green") pt.color.g()    = real_val;
            else if (prop.name == "blue")  pt.color.b()    = real_val;
        }

        p.push_back(pt);
    }

    if (vertices >= 0)
        throw std::invalid_argument("Unexpected EOF");

    varr_valid = rng_varr_valid = density_valid = false;
}


void cloud::store(std::ofstream &s) const
{
    s << "ply" << std::endl;
    s << "format ascii 1.0" << std::endl;
    s << "element vertex " << p.size() << std::endl;
    s << "property float x" << std::endl;
    s << "property float y" << std::endl;
    s << "property float z" << std::endl;
    s << "property float nx" << std::endl;
    s << "property float ny" << std::endl;
    s << "property float nz" << std::endl;
    s << "property uchar red" << std::endl;
    s << "property uchar green" << std::endl;
    s << "property uchar blue" << std::endl;
    s << "end_header" << std::endl;

    for (const point &pt: p) {
        s << pt.position.x() << ' '
          << pt.position.y() << ' '
          << pt.position.z() << ' '
          << pt.normal.x() << ' '
          << pt.normal.y() << ' '
          << pt.normal.z() << ' '
          << static_cast<int>(pt.color.r() * 255) << ' '
          << static_cast<int>(pt.color.g() * 255) << ' '
          << static_cast<int>(pt.color.b() * 255) << std::endl;
    }
}


vertex_array *cloud::vertex_array(void)
{
    if (!varr || !varr_valid) {
        if (!varr)
            varr = new dake::gl::vertex_array;

        varr->set_elements(p.size());
        varr->bind();

        vertex_attrib *va_p = varr->attrib(0);
        vertex_attrib *va_n = varr->attrib(1);
        vertex_attrib *va_c = varr->attrib(2);

        va_p->format(3);
        va_n->format(3);
        va_c->format(3);

        static_assert(sizeof(p[0].position) == 3 * sizeof(float), "invalid size of point.position");
        static_assert(sizeof(p[0].normal)   == 3 * sizeof(float), "invalid size of point.normal");
        static_assert(sizeof(p[0].color)    == 3 * sizeof(float), "invalid size of point.color");

        va_p->data(p.data(), p.size() * sizeof(point));
        va_n->reuse_buffer(va_p);
        va_c->reuse_buffer(va_p);

        va_p->load(sizeof(point), offsetof(point, position));
        va_n->load(sizeof(point), offsetof(point, normal));
        va_c->load(sizeof(point), offsetof(point, color));

        varr_valid = true;
    }

    return varr;
}


vertex_array *cloud::rng_vertex_array(int k)
{
    if (!rng_varr || !rng_varr_valid || (k != rng_k)) {
        if (!rng_varr) {
            rng_varr = new dake::gl::vertex_array;
        }

        std::vector<std::pair<vec3, vec3>> lines;
        rng r(*this, k);

        for (const rng::edge &e: r) {
            // TODO: Maybe we could do something with indices here (instead of
            // giving the full coordinates to OpenGL)
            lines.emplace_back(p[e.i].position, p[e.j].position);
        }

        rng_varr->set_elements(lines.size() * 2);
        rng_varr->bind();

        vertex_attrib *va_p = rng_varr->attrib(0);
        va_p->format(3);
        va_p->data(lines.data());
        va_p->load();

        rng_k = k;
        rng_varr_valid = true;
    }

    return rng_varr;
}


void cloud::cull_outliers(float cull_ratio, int k)
{
    assert((cull_ratio >= 0) && (cull_ratio <= 1));

    if (!density_valid) {
        recalc_density(k);
        density_valid = true;
    }

    // Sort descending regarding the density
    std::sort(p.begin(), p.end(), [](const point &p1, const point &p2) { return p1.density > p2.density; });

    p.resize(lrint(p.size() * (1.f - cull_ratio)));

    varr_valid = rng_varr_valid = density_valid = false;
}


void cloud::recalc_density(int k)
{
    init_progress("Density (%p %)", p.size());

    kd_tree<3> kdt(*this, INT_MAX, 10);

    int i = 0;
    for (point &pt: p) {
        std::vector<const point *> knn = kdt.knn(pt.position, k);

        // The vector is ordered with the neighbor the most distant being last
        float r = (knn.back()->position - pt.position).length();

        pt.density = k / (static_cast<float>(M_PI) * r * r);

        announce_progress(++i);
    }

    reset_progress();
}


void cloud::recalc_normals(int k, bool orientation)
{
    size_t point_count = p.size();
    assert(point_count <= INT_MAX);

    int thread_count = std::thread::hardware_concurrency();


    init_progress("Normals (%p %)", point_count);

    kd_tree<3> kdt(*this, INT_MAX, 10);

    // I tried to call this function from the RNG constructor (which calculates
    // both the kd tree and KNN), but it did not work out so well. Either way,
    // as the MST problem takes much more time (for my computer at least), it's
    // probably not so important to optimize this anyway.
    auto normal_calc_thread = [&](int thread_index) {
        for (int i = thread_index; i < static_cast<int>(point_count); i += thread_count) {
            std::vector<const point *> knn(kdt.knn(p[i].position, k));

            vec3 expectation = inject(knn, vec3::zero(), [](const vec3 &p1, const point *p2) { return p1 + p2->position; }) / k;
            mat3 cov_mat;

            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    cov_mat[c][r] = inject(map<float>(knn, [&](const point *p1) { return (p1->position[r] - expectation[r]) * (p1->position[c] - expectation[c]); }), 0.f, sum) / k;
                }
            }

            // Always nice to have compatible libraries
            Eigen::Matrix3f eigen_cov_mat(cov_mat);
            Eigen::EigenSolver<Eigen::Matrix3f> cov_mat_solver(eigen_cov_mat);

            Eigen::Vector3f eigen_eigenvalues(cov_mat_solver.eigenvalues().real());
            Eigen::Matrix3f eigen_eigenvectors(cov_mat_solver.eigenvectors().real());

            vec3 eigenvalues(vec3::from_data(eigen_eigenvalues.data()));
            mat3 eigenvectors(mat3::from_data(eigen_eigenvectors.data()));
            // dake is pretty useless for eigenvalue stuff, as it invokes ruby for
            // every single matrix (I was too lazy to either implement it myself or
            // at least embed ruby)

            int min_ev_index = 0;
            float min_ev = HUGE_VALF;
            for (int i = 0; i < 3; i++) {
                if (eigenvalues[i] < min_ev) {
                    min_ev = eigenvalues[i];
                    min_ev_index = i;
                }
            }

            p[i].normal = eigenvectors[min_ev_index].normalized();


            if (!thread_index) {
                announce_progress(i + 1);
            }
        }
    };

    std::thread *threads[thread_count];
    for (int i = 0; i < thread_count; i++) {
        threads[i] = new std::thread(normal_calc_thread, i);
    }
    for (int i = 0; i < thread_count; i++) {
        threads[i]->join();
        delete threads[i];
    }


    varr_valid = rng_varr_valid = density_valid = false;


    rng r(*this, k);

    std::vector<std::vector<rng::edge>> rng_point_edges(point_count);
    for (const rng::edge &e: r) {
        rng_point_edges[e.i].push_back(e);
        rng_point_edges[e.j].push_back(e);
    }


    init_progress("MST (%p %)", point_count);

    // Now use Prim-Dijkstra for finding a minimal spanning tree
    bool *has_vertex = static_cast<bool *>(calloc(point_count, sizeof *has_vertex));
    std::priority_queue<rng::edge, std::vector<rng::edge>, rng::edge_compare_backwards> st_point_edges;
    std::vector<rng::edge> st_edges;
    st_edges.reserve(point_count - 1);

    // Just start at some random point
    has_vertex[0] = true;

    for (const rng::edge &e: rng_point_edges[0]) {
        st_point_edges.push(e);
    }

    for (size_t vertices_found = 1; vertices_found < point_count; vertices_found++) {
        const rng::edge *new_edge;
        do {
            if (st_point_edges.empty()) {
                new_edge = nullptr;
                break;
            }

            new_edge = &st_point_edges.top();
            st_point_edges.pop();
        } while (has_vertex[new_edge->i] && has_vertex[new_edge->j]);

        if (!new_edge) {
            reset_progress();
            std::stringstream msg;
            msg << "The constructed RNG graph has multiple components (failed at " << vertices_found << " of " << point_count << " points); try increasing k (the neighbor count for the nearest neighbor search)";
            throw std::logic_error(msg.str());
        }

        st_edges.push_back(*new_edge);
        int new_vertex = has_vertex[new_edge->i] ? new_edge->j : new_edge->i;
        has_vertex[new_vertex] = true;

        for (const rng::edge &e: rng_point_edges[new_vertex]) {
            st_point_edges.push(e);
        }

        announce_progress(vertices_found + 1);
    }


    init_progress("Homogenizing (%p %)", point_count);


    if (orientation) {
        p[0].normal = -p[0].normal;
    }

    // I like it although it's kind of strange
    memset(has_vertex, 0, point_count * sizeof *has_vertex);
    has_vertex[0] = true;
    for (size_t vertices_found = 1; vertices_found < point_count;) {
        for (const rng::edge &e: st_edges) {
            if (has_vertex[e.i] ^ has_vertex[e.j]) {
                int old_vertex = has_vertex[e.i] ? e.i : e.j;
                int new_vertex = has_vertex[e.i] ? e.j : e.i;

                if (p[old_vertex].normal.dot(p[new_vertex].normal) < 0.f) {
                    p[new_vertex].normal = -p[new_vertex].normal;
                }

                has_vertex[new_vertex] = true;
                vertices_found++;
            }
        }

        announce_progress(vertices_found);
    }

    free(has_vertex);

    varr_valid = rng_varr_valid = density_valid = false;


    reset_progress();
}


void cloud_manager::load_new(std::ifstream &s, const std::string &name)
{
    c->emplace_back(name);

    try {
        c->back().load(s);
    } catch (...) {
        c->pop_back();
        throw;
    }
}


void cloud_manager::unify(float resolution, const std::string &name)
{
    cloud unified(*c, resolution, name);

    delete c;
    c = new std::list<cloud>;
    c->push_back(unified);
}


struct correspondence {
    vec4 p1, p2;
    float distance;

    correspondence(void) {}
    correspondence(const vec3 &pt1, const vec3 &pt2): p1(pt1), p2(pt2)
    { p1.w() = 1.f; p2.w() = 1.f; distance = (p1 - p2).length(); }

    bool operator<(const correspondence &c) const
    { return distance < c.distance; }
};


void cloud_manager::icp(size_t m, size_t n, float p)
{
    if (c->size() != 2) {
        throw std::invalid_argument("ICP can only be done iff exactly two point clouds are loaded");
    }

    if (n > c->front().points().size()) {
        n = c->front().points().size();
    } else if (!n) {
        throw std::invalid_argument("n must be positive");
    }

    if (p < 0) {
        p = 0;
    } else if (p >= 1) {
        throw std::invalid_argument("p may not be 100 % or more");
    }

    init_progress("ICP (%p %)", m);

    std::vector<correspondence> correspondences;
    std::vector<const point *> remaining_points;
    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

    kd_tree<3> kdt(c->back());

    for (size_t iteration = 0; iteration < m; iteration++) {
        correspondences.clear();
        correspondences.reserve(n);

        remaining_points.clear();
        remaining_points.reserve(c->front().points().size());

        for (const point &pt: c->front().points()) {
            remaining_points.push_back(&pt);
        }

        mat4 trans(c->back().transformation().inverse() * c->front().transformation());

        for (size_t i = 0; i < n; i++) {
            size_t idx = std::uniform_int_distribution<size_t>(0, remaining_points.size() - 1)(rng);
            const point *pt = remaining_points[idx];

            vec3 trans_coord(trans * vec4(pt->position.x(), pt->position.y(), pt->position.z(), 1.f));

            const point *nn = kdt.knn(trans_coord, 1).front();

            correspondences.emplace_back(pt->position, nn->position);
        }

        std::sort(correspondences.begin(), correspondences.end());

        size_t rem = n - static_cast<size_t>(p * n);
        if (!rem) {
            rem = 1;
        }

        correspondences.resize(rem);


        // Transform all points to the global coordinate system; then register
        // them in global coordinates and apply the first cloud's original
        // transformation onto the calculated result to obtain the new
        // transformation for the second cloud.

        // Points from the first cloud
        auto p = map<vec3>(correspondences, [&](const correspondence &cr) { return vec3(c->front().transformation() * cr.p1); });
        // Points from the second cloud (TODO: As these are the nearest
        // neighbor from the first cloud, there may be duplicates; maybe this is
        // is undesirable)
        auto q = map<vec3>(correspondences, [&](const correspondence &cr) { return vec3(c->back().transformation() * cr.p2); });

        assert(p.size() == rem);
        assert(q.size() == rem);

        vec3 p_centroid = inject(p, sum) / rem;
        vec3 q_centroid = inject(q, sum) / rem;

        auto p_rel = map(p, [&](const vec3 &pi) { return pi - p_centroid; });
        auto q_rel = map(q, [&](const vec3 &qi) { return qi - q_centroid; });

        mat3 H = inject(map<mat3>(range<>(0, rem - 1), [&](int i) { return q_rel[i] * p_rel[i].transposed(); }), sum) / rem;

#define JUST_GIVE_IN_TO_EIGEN

#if defined(USE_EIGEN_FOR_MANUAL_SVD_AND_FAIL_UTTERLY)
        // SVD
        Eigen::Matrix3f H_tn(H * H.transposed());
        Eigen::EigenSolver<Eigen::Matrix3f> H_tn_solver(H_tn);
        Eigen::Matrix3f H_tn_result(H_tn_solver.eigenvectors().real());
        mat3 U(mat3::from_data(H_tn_result.data()));

        Eigen::Matrix3f H_nt(H.transposed() * H);
        Eigen::EigenSolver<Eigen::Matrix3f> H_nt_solver(H_nt);
        Eigen::Matrix3f H_nt_result(H_nt_solver.eigenvectors().real());
        mat3 Vt(mat3::from_data(H_nt_result.data()).transposed());
#elif defined(USE_RUBY_FOR_MANUAL_SVD_AND_DONT_WORK_ON_WINDOWS_AND_BE_HORRIBLY_SLOW_ON_LINUX)
        mat3 U(H.svd_U());
        mat3 Vt(H.svd_V().transposed());
#elif defined(JUST_GIVE_IN_TO_EIGEN)
        Eigen::Matrix3f EH(H);
        Eigen::JacobiSVD<Eigen::Matrix3f> svd(EH, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Eigen::Matrix3f EU(svd.matrixU().real()), EV(svd.matrixV().real());
        mat3 U(mat3::from_data(EU.data()));
        mat3 Vt(mat3::from_data(EV.data()).transposed());
#endif

        mat3 diag = mat3::diagonal(1.f, 1.f, (Vt * U).det());

        mat3 R = Vt * diag * U;
        if (R.det() < 0.f) {
            diag[2][2] = -diag[2][2];
            R = Vt * diag * U;
        }

        vec3 t = p_centroid - R * q_centroid;

        mat4 new_trans(R);
        new_trans[3] = vec4(t.x(), t.y(), t.z(), 1.f);

        c->back().transformation() = new_trans * c->front().transformation();


        ro->invalidate();

        announce_progress(iteration);
    }

    reset_progress();
}
