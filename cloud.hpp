#ifndef CLOUD_HPP
#define CLOUD_HPP

#include <cfenv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <list>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <dake/math/matrix.hpp>
#include <dake/helper/traits.hpp>
#include <dake/gl/vertex_array.hpp>
#include <dake/gl/vertex_attrib.hpp>

#include "point.hpp"


namespace std
{

template<> struct hash<dake::math::vec3i> {
    typedef dake::math::vec3i argument_type;
    typedef size_t value_type;

    enum {
        P1 = 131071,
        P2 = 524287,
        P3 = 8191
    };

    value_type operator()(const argument_type &v) const
    {
        return static_cast<value_type>(v.x()) * P1
             + static_cast<value_type>(v.y()) * P2
             + static_cast<value_type>(v.z()) * P3;
    }
};

}


class cloud {
    public:
        cloud(const std::string &name = "(unnamed)");

        template<typename Container, typename std::enable_if<dake::helper::is_iterable<Container>::value && std::is_same<typename Container::value_type, cloud>::value>::type * = nullptr>
        cloud(const Container &input, float resolution, const std::string &name = "(unnamed)"):
            trans(dake::math::mat4::identity()),
            n(name)
        {
            using namespace dake::math;

            // Let's just pray the user has enough memory for this
            std::unordered_map<vec3i, std::list<point>> clusters;
            int round_mode = fegetround();
            fesetround(FE_DOWNWARD);

            for (const cloud &c: input) {
                const mat4 &pos_trans = c.trans;
                mat3 norm_trans(pos_trans);
                norm_trans.transposed_invert();

                for (const point &p: c.points()) {
                    vec3 global_pos(pos_trans * vec4(p.position.x(), p.position.y(), p.position.z(), 1.f));
                    vec3 index_flt(global_pos / resolution);
                    vec3i index(lrint(index_flt.x()), lrint(index_flt.y()), lrint(index_flt.z()));

                    clusters[index].emplace_back(vec3(global_pos), norm_trans * p.normal, p.color, p.density);
                }
            }

            fesetround(round_mode);

            for (auto cluster: clusters) {
                point result(vec3::zero(), vec3::zero(), vec3::zero(), 0.f);
                size_t cpc = cluster.second.size(); // cluster point count

                for (auto p: cluster.second) {
                    result.position += p.position / cpc;
                    result.normal   += p.normal;
                    result.color    += p.color    / cpc;
                    result.density  += p.density  / cpc;
                }

                if (result.normal.length()) {
                    result.normal.normalize();
                }

                p.push_back(std::move(result));
            }
        }

        ~cloud(void);


        void load(std::ifstream &s);
        void store(std::ofstream &s) const;


        const std::vector<point> &points(void) const
        { return p; }
        std::vector<point> &points(void)
        { varr_valid = false; return p; }

        const dake::math::mat4 &transformation(void) const
        { return trans; }
        dake::math::mat4 &transformation(void)
        { return trans; }

        dake::gl::vertex_array *vertex_array(void);
        dake::gl::vertex_array *normal_vertex_array(void);

        const std::string &name(void) const
        { return n; }
        std::string &name(void)
        { return n; }


    private:
        std::vector<point> p;
        dake::math::mat4 trans;
        dake::gl::vertex_array *varr = nullptr;
        bool varr_valid = false;
        std::string n;
};


class cloud_manager {
    public:
        cloud_manager(void): c(new std::list<cloud>) {}
        ~cloud_manager(void) { delete c; }

        const std::list<cloud> &clouds(void) const
        { return *c; }
        std::list<cloud> &clouds(void)
        { return *c; }

        void load_new(std::ifstream &s, const std::string &name = "(unnamed)")
        { c->emplace_back(name); c->back().load(s); }

        void unify(float resolution, const std::string &name = "(unnamed)");


    private:
        std::list<cloud> *c;
};

#endif
