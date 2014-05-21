#ifndef KD_TREE_HPP
#define KD_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>
#include <dake/math/matrix.hpp>
#include <dake/container/algorithm.hpp>
#include <dake/container/range.hpp>
#include <dake/helper/function.hpp>

#include "cloud.hpp"
#include "point.hpp"


template<unsigned K>
class kd_tree_node {
    public:
        typedef std::vector<const point *>::iterator iterator;
        typedef std::vector<const point *>::const_iterator const_iterator;

        kd_tree_node(std::vector<const point *> &points, unsigned start, unsigned end, unsigned max_depth, unsigned min_points):
            start_it(points.begin()), end_it(points.end())
        {
            using namespace dake::container;
            using namespace dake::helper;

            start_it = points.begin();
            std::advance(start_it, start);

            end_it = points.begin();
            std::advance(end_it, end);

            if ((end - start < min_points * 2) || !max_depth) {
                return;
            }

            vector min_start, max_start;
            for (unsigned i = 0; i < K; i++) {
                min_start[i] =  HUGE_VALF;
                max_start[i] = -HUGE_VALF;
            }

            std::pair<vector, vector> minmax = inject(
                    start_it,
                    end_it,
                    std::make_pair(min_start, max_start),
                    [](const std::pair<vector, vector> &out, const point *in) {
                        vector min, max;
                        for (unsigned i = 0; i < K; i++) {
                            min[i] = minimum(out.first [i], in->position[i]);
                            max[i] = maximum(out.second[i], in->position[i]);
                        }
                        return std::make_pair(min, max);
                    }
                );

            split_dim = inject(
                    map<std::pair<int, float>>(range<>(0, K - 1), [&](int i) { return std::make_pair(i, minmax.second[i] - minmax.first[i]); }),
                    std::make_pair(0, 0.f),
                    [](const std::pair<int, float> &out, const std::pair<int, float> &in) {
                        return in.second > out.second ? in : out;
                    }
                ).first;

            // Calculate the median
            std::sort(start_it, end_it, [&](const point *a, const point *b) { return a->position[split_dim] < b->position[split_dim]; });

            if ((end - start) % 2) {
                split_val = points[start + (end - start - 1) / 2]->position[split_dim];
            } else {
                split_val = .5f * (points[start + (end - start) / 2]->position[split_dim] + points[start + (end - start) / 2 + 1]->position[split_dim]);
            }

            unsigned first_exceeding_median;
            for (first_exceeding_median = start;
                 (first_exceeding_median < end) &&
                 (points[first_exceeding_median]->position[split_dim] <= split_val);
                 first_exceeding_median++);

            assert((first_exceeding_median > start) && (first_exceeding_median < end));

            left_child  = new kd_tree_node<K>(points, start, first_exceeding_median, max_depth - 1, min_points);
            right_child = new kd_tree_node<K>(points,  first_exceeding_median, end,  max_depth - 1, min_points);
        }

        ~kd_tree_node(void)
        {
            delete left_child;
            delete right_child;
        }

        // All points p here meet the constraint p.position[split_dimension()] <= split_value()
        kd_tree_node *left(void) const
        { return left_child; }
        // All points p here meet the constraint p.position[split_dimension()] >  split_value()
        kd_tree_node *right(void) const
        { return right_child; }

        bool leaf(void) const
        { return !left_child; }

        iterator begin(void)
        { return start_it; }
        iterator end(void)
        { return end_it; }

        const_iterator begin(void) const
        { return start_it; }
        const_iterator end(void) const
        { return end_it; }

        int split_dimension(void) const
        { return split_dim; }
        float split_value(void) const
        { return split_val; }

        void dump(unsigned level_indentation = 2, unsigned indentation = 0) const;


    private:
        iterator start_it, end_it;
        kd_tree_node *left_child = nullptr, *right_child = nullptr;
        int split_dim = -1;
        float split_val = 0.f;

        typedef dake::math::vec<K, float> vector;
};


template<unsigned K>
class kd_tree {
    public:
        kd_tree(const cloud &c, unsigned max_depth = 10, unsigned min_points = 10)
        {
            point_references.reserve(c.points().size());

            for (const point &pt: c.points()) {
                point_references.push_back(&pt);
            }

            root_node = new kd_tree_node<K>(point_references, 0, point_references.size(), max_depth, min_points);
        }

        ~kd_tree(void)
        {
            delete root_node;
        }

        kd_tree_node<K> *root(void) const
        { return root_node; }

        void dump(unsigned level_indentation = 2, unsigned indentation = 0) const;


    private:
        std::vector<const point *> point_references;
        kd_tree_node<K> *root_node;
};

#endif
