#ifndef KD_TREE_HPP
#define KD_TREE_HPP

#include <algorithm>
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
class kd_tree {
    public:
        typedef std::vector<const point *>::iterator iterator;
        typedef std::vector<const point *>::const_iterator const_iterator;

        kd_tree(std::vector<const point *> &points, unsigned start, unsigned end, unsigned max_depth, unsigned min_points)
        {
            initialize(points, start, end, max_depth, min_points);
        }

        kd_tree(const cloud &c, unsigned max_depth = 10, unsigned min_points = 10)
        {
            root_point_reference = new std::vector<const point *>;
            root_point_reference->reserve(c.points().size());

            for (const point &pt: c.points()) {
                root_point_reference->push_back(&pt);
            }

            initialize(*root_point_reference, 0, c.points().size(), max_depth, min_points);
        }

        ~kd_tree(void)
        {
            delete left_child;
            delete right_child;
            delete root_point_reference;
        }

        kd_tree *left(void) const
        { return left_child; }
        kd_tree *right(void) const
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

        void dump(unsigned level_indentation = 2, unsigned indentation = 0) const;


    private:
        std::vector<const point *> *root_point_reference = nullptr;
        iterator start_it, end_it;
        kd_tree *left_child = nullptr, *right_child = nullptr;

        typedef dake::math::vec<K, float> vector;


        void initialize(std::vector<const point *> &points, unsigned start, unsigned end, unsigned max_depth, unsigned min_points)
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

            int dimension = inject(
                    map<std::pair<int, float>>(range<>(0, K - 1), [&](int i) { return std::make_pair(i, minmax.second[i] - minmax.first[i]); }),
                    std::make_pair(0, 0.f),
                    [](const std::pair<int, float> &out, const std::pair<int, float> &in) {
                        return in.second > out.second ? in : out;
                    }
                ).first;

            std::sort(start_it, end_it, [&dimension](const point *a, const point *b) { return a->position[dimension] < b->position[dimension]; });

            // Let's do this literally according to the task (take the median
            // and divide according to that)
            float median;
            if ((end - start) % 2) {
                median = points[start + (end - start - 1) / 2]->position[dimension];
            } else {
                median = .5f * (points[start + (end - start) / 2]->position[dimension] + points[start + (end - start) / 2 + 1]->position[dimension]);
            }

            unsigned first_exceeding_median;
            for (first_exceeding_median = start;
                 (first_exceeding_median < end) &&
                 (points[first_exceeding_median]->position[dimension] <= median);
                 first_exceeding_median++);

            if ((first_exceeding_median == start) || (first_exceeding_median == end)) {
                // Putting all the elements in one tree doesn't make much
                // sense, therefore we should deviate from the task here and
                // put half the elements in the left and the other half in the
                // right tree.
                first_exceeding_median = (start + end) / 2;
            }

            left_child  = new kd_tree<K>(points, start, first_exceeding_median, max_depth - 1, min_points);
            right_child = new kd_tree<K>(points,  first_exceeding_median, end,  max_depth - 1, min_points);
        }

};

#endif
