#ifndef CLOUD_HPP
#define CLOUD_HPP

#include <dake/gl/gl.hpp>

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


#ifndef FE_DOWNWARD
#include <cstdint>

#define FE_TONEAREST  0x0000
#define FE_DOWNWARD   0x0400
#define FE_UPWARD     0x0800
#define FE_TOWARDZERO 0x0c00

static inline int fesetround(int mode)
{
    uint16_t cw;
    __asm__ __volatile__ ("fnstcw %0" : "=m"(cw));
    cw &= ~(FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO);
    cw |= mode;
    __asm__ __volatile__ ("fldcw %0" :: "m"(cw));
    return 0;
}

static inline int fegetround(void)
{
    uint16_t cw;
    __asm__ __volatile__ ("fnstcw %0" : "=m"(cw));
    return cw & (FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO);
}
#endif


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
        return value_hash(static_cast<value_type>(v.x()) * P1
                        + static_cast<value_type>(v.y()) * P2
                        + static_cast<value_type>(v.z()) * P3);
    }


    private:
        std::hash<value_type> value_hash;
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

            std::unordered_map<vec3i, point_counter> result;
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

                    auto entry = result.find(index);
                    if (entry == result.end()) {
                        result.emplace(index, point_counter(global_pos, norm_trans * p.normal, p.color, p.density));
                    } else {
                        // Let's just pray this doesn't overflow
                        entry->second.position += vec3(global_pos);
                        entry->second.normal   += norm_trans * p.normal;
                        entry->second.color    += p.color;
                        entry->second.density  += p.density;
                        entry->second.count++;
                    }
                }
            }

            fesetround(round_mode);

            for (const auto &pc: result) {
                const auto &pt = pc.second;

                p.emplace_back(pt.position / pt.count, pt.normal, pt.color / pt.count, pt.density / pt.count);

                if (p.back().normal.length()) {
                    p.back().normal.normalize();
                }
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
        dake::gl::vertex_array *rng_vertex_array(int k);

        const std::string &name(void) const
        { return n; }
        std::string &name(void)
        { return n; }


        void cull_outliers(float cull_ratio, int k = 10);
        void recalc_density(int k);
        void recalc_normals(int k, bool orientation = false);


    private:
        std::vector<point> p;
        dake::math::mat4 trans;
        dake::gl::vertex_array *varr = nullptr, *rng_varr = nullptr;
        bool varr_valid = false, rng_varr_valid = false, density_valid = false;
        int rng_k = -1;
        std::string n;

        struct point_counter: public point {
            point_counter(dake::math::vec3 p, dake::math::vec3 n, dake::math::vec3 c, float d): point(p, n, c, d), count(1) {}
            size_t count;
        };
};


class cloud_manager {
    public:
        cloud_manager(void): c(new std::list<cloud>) {}
        ~cloud_manager(void) { delete c; }

        const std::list<cloud> &clouds(void) const
        { return *c; }
        std::list<cloud> &clouds(void)
        { return *c; }

        void load_new(std::ifstream &s, const std::string &name = "(unnamed)");
        void unify(float resolution, const std::string &name = "(unnamed)");
        void icp(size_t m, size_t n, float p);


    private:
        std::list<cloud> *c;
};

#endif
