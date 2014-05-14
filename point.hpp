#ifndef POINT_HPP
#define POINT_HPP

#include <dake/math/matrix.hpp>


struct point {
    dake::math::vec3 position, normal;
    dake::math::vec3 color;
    float density;
};

#endif
