#ifndef POINT_HPP
#define POINT_HPP

#include <dake/math/matrix.hpp>


struct point {
    point(void) {}

    point(dake::math::vec3 pos, dake::math::vec3 norm, dake::math::vec3 col, float d):
        position(pos), normal(norm), color(col), density(d)
    {}

    dake::math::vec3 position, normal;
    dake::math::vec3 color;
    float density;
};

#endif
