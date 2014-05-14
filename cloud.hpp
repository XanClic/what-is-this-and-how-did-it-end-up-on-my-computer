#ifndef CLOUD_HPP
#define CLOUD_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <dake/math/matrix.hpp>
#include <dake/gl/vertex_array.hpp>
#include <dake/gl/vertex_attrib.hpp>

#include "point.hpp"


class cloud {
    public:
        cloud(void);


        void load(std::ifstream &s);
        void store(std::ofstream &s);


        const std::vector<point> &points(void) const
        { return p; }

        const dake::math::mat4 &transformation(void) const
        { return trans; }
        dake::math::mat4 &transformation(void)
        { return trans; }

        dake::gl::vertex_array *vertex_array(void);


    private:
        std::vector<point> p;
        dake::math::mat4 trans;
        dake::gl::vertex_array *varr = nullptr;
        bool varr_valid = false;
};

#endif
