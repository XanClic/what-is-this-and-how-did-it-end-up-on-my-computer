#include <dake/gl/gl.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <list>
#include <stdexcept>
#include <sstream>
#include <string>
#include <unordered_map>
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

        // Because I'm lazy
        std::vector<vec3> lines;
        kd_tree<3> kdt(*this, INT_MAX, 10);

        for (const point &pt: p) {
            std::vector<const point *> knn = kdt.knn(pt.position, k);

            for (const point *nn: knn) {
                lines.push_back(pt.position);
                lines.push_back(nn->position);
            }
        }

        rng_varr->set_elements(lines.size());
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
    kd_tree<3> kdt(*this, INT_MAX, 10);

    for (point &pt: p) {
        std::vector<const point *> knn = kdt.knn(pt.position, k);

        // The vector is ordered with the neighbor the most distant being last
        float r = (knn.back()->position - pt.position).length();

        pt.density = k / (static_cast<float>(M_PI) * r * r);
    }
}


void cloud::recalc_normals(int k, bool orientation)
{
    kd_tree<3> kdt(*this, INT_MAX, 10);

    for (point &pt: p) {
        std::vector<const point *> knn = kdt.knn(pt.position, k);

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

        pt.normal = eigenvectors[min_ev_index].normalized();
    }

    varr_valid = rng_varr_valid = density_valid = false;


    size_t point_count = p.size();
    assert(point_count <= INT_MAX);

    // TODO: The RNG could be its own class
    std::unordered_map<std::pair<int, int>, float> rng;
    std::vector<std::pair<int, int>> rng_edges;

    for (int i = 0; i < static_cast<int>(point_count); i++) {
        // TODO: We already did this above
        std::vector<const point *> knn = kdt.knn(p[i].position, k);

        for (const point *nn: knn) {
            // Actually, someone told me this is correct (I had a FIXME
            // here and thought it to be broken as hell)
            ptrdiff_t uj = nn - p.data();
            assert((uj >= 0) && (static_cast<size_t>(uj) < point_count) && (&p[uj] == nn));

            int j = static_cast<int>(uj);

            // We don't want loops
            if ((i == j) || (rng.find(std::make_pair(i, j)) != rng.end())) {
                continue;
            }

            float weight = 1.f - fabs(p[i].normal.dot(nn->normal));
            rng[std::make_pair(i, j)] = weight;
            rng[std::make_pair(j, i)] = weight;

            rng_edges.push_back(std::make_pair(i, j));
        }
    }

    // Sort rng_edges ascending by weight
    std::sort(rng_edges.begin(), rng_edges.end(), [&rng](const std::pair<int, int> &e1, const std::pair<int, int> &e2) { return rng[e1] < rng[e2]; });

    // Now use Prim-Dijkstra for finding a minimal spanning tree
    bool *has_vertex = static_cast<bool *>(calloc(point_count, sizeof *has_vertex));
    std::vector<std::pair<int, int>> st_edges;
    size_t vertices_found = 0;

    has_vertex[rng_edges.front().first] = true;
    vertices_found++;

    while (vertices_found < point_count) {
        std::pair<int, int> new_edge(0, 0);
        // I'd like to cull inner edges of the spanning tree afterwards so
        // they don't have to be searched here; however, std::remove_if() is
        // slow on std::vector (which is no surprise, but someone whom I
        // thought I could trust told me otherwise (that the cache could
        // handle it all)), but using std::list (or something similar) makes
        // std::sort() require operator-() on std::pair<>. Using std::set to
        // avoid the std::sort() does not like my comparison lambda, so this
        // would require an own class as well. An own class on the other
        // hand would not have access to the rng map.
        // So, I just don't cull and run through the whole vector here.
        for (const std::pair<int, int> &edge: rng_edges) {
            if (has_vertex[edge.first] ^ has_vertex[edge.second]) {
                new_edge = edge;
                break;
            }
        }
        if (new_edge.first == new_edge.second) {
            throw std::logic_error("The constructed RNG graph has multiple components; try increasing k (the neighbor count for the nearest neighbor search)");
        }

        st_edges.push_back(new_edge);
        int new_vertex = has_vertex[new_edge.first] ? new_edge.second : new_edge.first;
        has_vertex[new_vertex] = true;
        vertices_found++;
    }

    if (orientation) {
        p[0].normal = -p[0].normal;
    }

    // I like it although it's kind of strange
    memset(has_vertex, 0, point_count * sizeof *has_vertex);
    has_vertex[0] = true;
    vertices_found = 1;
    while (vertices_found < point_count) {
        for (const std::pair<int, int> &edge: st_edges) {
            if (has_vertex[edge.first] ^ has_vertex[edge.second]) {
                int old_vertex = has_vertex[edge.first] ? edge.first : edge.second;
                int new_vertex = has_vertex[edge.first] ? edge.second : edge.first;

                if (p[old_vertex].normal.dot(p[new_vertex].normal) < 0.f) {
                    p[new_vertex].normal = -p[new_vertex].normal;
                }

                has_vertex[new_vertex] = true;
                vertices_found++;
            }
        }
    }

    free(has_vertex);
}


void cloud_manager::unify(float resolution, const std::string &name)
{
    cloud unified(*c, resolution, name);

    delete c;
    c = new std::list<cloud>;
    c->push_back(unified);
}
