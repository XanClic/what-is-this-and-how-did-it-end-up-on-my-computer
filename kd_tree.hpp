#ifndef KD_TREE_HPP
#define KD_TREE_HPP

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <iterator>
#include <set>
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

        typedef dake::math::vec<K, float> vector;

        struct nearest_neighbor {
            const point *pt;
            float dist;

            nearest_neighbor(const point *p, float d): pt(p), dist(d) {}

            // This function is sufficient for std::set. As std::set considers
            // two objects a, b equal iff !(a < b) && !(b < a), we have to make
            // sure not to compare neighbors based on their distance alone.
            bool operator<(const nearest_neighbor &nn) const
            {
                if (dist != nn.dist) {
                    return dist < nn.dist;
                }

                for (int i = 0; i < pt->position.rows; i++) {
                    // Floating point equality comparisons are fine here, as we
                    // want to check whether they are the *same* points, not
                    // just equal. Any difference (and be it yet so small) is
                    // fine.
                    if (pt->position[i] != nn.pt->position[i]) {
                        return pt->position[i] < nn.pt->position[i];
                    }
                }

                // Well then, they are indeed equal.
                return false;
            }
        };

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

            unsigned median_index;

            // I don't even want to talk about this
            for (unsigned median_correction = 0;; median_correction++) {
                if ((end - start) % 2) {
                    median_index = (start + end - 1) / 2;
                } else {
                    median_index = (start + end) / 2 - 1;
                }

                if ((median_correction > median_index) || (median_index - median_correction < start)) {
                    // For this to happen, all points have to have the exact
                    // same coordinates. Just take one in this case and discard
                    // the rest.
                    end_it = start_it;
                    split_dim = -1;
                    return;
                }
                median_index -= median_correction;

                if ((end - start) % 2) {
                    split_val = points[median_index]->position[split_dim];
                } else {
                    split_val = .5f * (points[median_index]->position[split_dim] + points[median_index + 1]->position[split_dim]);
                }

                for (;
                     (median_index < end) &&
                     (points[median_index]->position[split_dim] <= split_val);
                     median_index++);

                assert(median_index > start);

                if (median_index < end) {
                    break;
                }
            }

            left_child  = new kd_tree_node<K>(points, start, median_index, max_depth - 1, min_points);
            right_child = new kd_tree_node<K>(points,  median_index, end,  max_depth - 1, min_points);
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

        // I need the nearest neighbors, but I also need fast access to the
        // farthest nearest neighbor -- therefore, I cannot use
        // std::priority_queue (maybe I can, but using std::set is easier).
        void knn_step(std::set<nearest_neighbor> &queue, const vector &position, unsigned neighbors) const
        {
            if (leaf()) {
                for (const point *pt: *this) {
                    bool full = queue.size() >= neighbors;
                    float dist = (position - pt->position).length();

                    // Add point to the nearest neighbor if the queue is not yet
                    // full or it is nearer than the farthest neighbor
                    if (!full || (dist < queue.rbegin()->dist)) {
                        queue.emplace(pt, dist);

                        if (full) {
                            auto end_it = queue.end();
                            queue.erase(--end_it);
                        }
                    }
                }
            } else {
                bool in_left = position[split_dim] <= split_val;

                (in_left ? left_child : right_child)->knn_step(queue, position, neighbors);

                // Visit the other child as well if the neighbor queue is not
                // yet full or it may contain elements which are nearer than the
                // farthest neighbor
                if ((queue.size() < neighbors) || (fabsf(position[split_dim] - split_val) < queue.rbegin()->dist)) {
                    (in_left ? right_child : left_child)->knn_step(queue, position, neighbors);
                }
            }
        }


    private:
        iterator start_it, end_it;
        kd_tree_node *left_child = nullptr, *right_child = nullptr;
        int split_dim = -1;
        float split_val = 0.f;
};


template<unsigned K>
class kd_tree {
    public:
        kd_tree(const cloud &c, unsigned max_depth = INT_MAX, unsigned min_points = 1)
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

        std::vector<const point *> knn(const dake::math::vec<K, float> &position, unsigned neighbors) const
        {
            assert(neighbors > 0);

            std::set<typename kd_tree_node<K>::nearest_neighbor> queue;

            root_node->knn_step(queue, position, neighbors);

            std::vector<const point *> ret;
            ret.reserve(queue.size());
            for (const auto &nn: queue) {
                ret.push_back(nn.pt);
            }

            return ret;
        }


    private:
        std::vector<const point *> point_references;
        kd_tree_node<K> *root_node;
};

#endif
