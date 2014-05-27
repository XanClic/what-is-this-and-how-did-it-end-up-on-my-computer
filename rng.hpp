#ifndef RNG_HPP
#define RNG_HPP

#include <cassert>
#include <functional>
#include <mutex>
#include <vector>

#include "cloud.hpp"
#include "point.hpp"


// 乾杯 for ambiguous abbreviations
// and cheers for any actual alliteration

// (This class is about Riemann's Neighborhood Graph or something similar)

class rng {
    public:
        struct edge {
            int i, j;
            float weight;

            edge(int _i, int _j, float w): i(_i), j(_j), weight(w) { assert((i < j) || (!i && !j)); }

            bool operator<(const edge &e) const { return weight < e.weight; }
            bool operator==(const edge &e) const { return (i == e.i) && (j == e.j); }
        };


        rng(cloud &c, int k);


        // Sort edges ascending by weight
        void sort(void);


        std::vector<edge> &edges(void) { return edge_vector; }
        const std::vector<edge> &edges(void) const { return edge_vector; }


        std::vector<edge>::iterator begin(void) { return edge_vector.begin(); }
        std::vector<edge>::iterator end(void) { return edge_vector.end(); }

        std::vector<edge>::const_iterator begin(void) const { return edge_vector.begin(); }
        std::vector<edge>::const_iterator end(void) const { return edge_vector.end(); }


    private:
        std::vector<edge> edge_vector;
        std::mutex edge_mutex;
};

#endif
