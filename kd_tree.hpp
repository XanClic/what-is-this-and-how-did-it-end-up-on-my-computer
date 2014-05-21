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
                if (end - start == 1) {
                    pt = *start_it;
                }
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

            // This is correct for odd (start - end) and for even (start - end)
            // we have the choice between this and the previous element. We have
            // to choose one, so just take this.
            unsigned median_index = (start + end) / 2;
            float median = points[median_index]->position[split_dim];

            // Increase median_index until it points to an element with a
            // different (= higher) value so we can then sort all elements with
            // .position[split_dim] <= median into the left tree
            for (;
                 (median_index < end) &&
                 (points[median_index]->position[split_dim] <= median);
                 median_index++);

            // For the first to occur, min_points needs to be zero. For the
            // second to occur, there need to be multiple points with the same
            // .position[split_dim] value as the median, which would be really
            // bad (but may very well occur). To fix this, we'd have to either
            // try a different split_dim, or even better, reduce the
            // median_index until it works. Both are against the task given, so
            // I won't do it and just chicken out here.
            assert((median_index > start) && ((end - start == 2) || (median_index < end)));

            pt = points[--median_index];

            left_child  = new kd_tree_node<K>(points, start, median_index, max_depth - 1, min_points);

            if (end - start > 2) {
                right_child = new kd_tree_node<K>(points, median_index + 1, end, max_depth - 1, min_points);
            }
        }

        ~kd_tree_node(void)
        {
            delete left_child;
            delete right_child;
        }

        // All points p here meet the constraint p.position[split_dimension()] <= split_point()[split_dimension()]
        kd_tree_node *left(void) const
        { return left_child; }
        // All points p here meet the constraint p.position[split_dimension()] >  split_point()[split_dimension()]
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
        const point *split_point(void) const
        { return pt; }

        void dump(unsigned level_indentation = 2, unsigned indentation = 0) const;


    private:
        const point *pt = nullptr;
        iterator start_it, end_it;
        kd_tree_node *left_child = nullptr, *right_child = nullptr;
        int split_dim = -1;

        typedef dake::math::vec<K, float> vector;
};


template<unsigned K>
class kd_tree {
    public:
        kd_tree(const cloud &c, unsigned max_depth = UINT_MAX, unsigned min_points = 1)
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
