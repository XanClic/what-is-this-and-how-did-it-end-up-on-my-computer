#include <cassert>
#include <climits>
#include <cmath>
#include <functional>
#include <thread>
#include <utility>
#include <vector>

#include "cloud.hpp"
#include "kd_tree.hpp"
#include "point.hpp"
#include "rng.hpp"
#include "window.hpp"


rng::rng(cloud &c, int k)
{
    size_t point_count = c.points().size();
    assert(point_count <= INT_MAX);

    int thread_count = std::thread::hardware_concurrency();

    init_progress("RNG (%p %)", point_count);


    kd_tree<3> kdt(c, INT_MAX, 10);

    auto rng_thread = [&](int thread_index) {
        for (int i = thread_index; i < static_cast<int>(point_count); i += thread_count) {
            std::vector<const point *> knn(kdt.knn(c.points()[i].position, k));

            for (const point *nn: knn) {
                // Actually, someone told me this is correct (I had a FIXME
                // here and thought it to be broken as hell)
                ptrdiff_t uj = nn - c.points().data();
                assert((uj >= 0) && (static_cast<size_t>(uj) < point_count) && (&c.points()[uj] == nn));

                int j = static_cast<int>(uj);

                // We don't want loops
                if (i == j) {
                    continue;
                }

                // TODO: lock-free
                edge_mutex.lock();
                float weight = 1.f - fabs(c.points()[i].normal.dot(nn->normal));
                if (i < j) {
                    edge_vector.emplace_back(i, j, weight);
                } else {
                    edge_vector.emplace_back(j, i, weight);
                }
                edge_mutex.unlock();
            }

            if (!thread_index) {
                announce_progress(i + 1);
            }
        }
    };

    std::thread *threads[thread_count];
    for (int i = 0; i < thread_count; i++) {
        threads[i] = new std::thread(rng_thread, i);
    }
    for (int i = 0; i < thread_count; i++) {
        threads[i]->join();
        delete threads[i];
    }


    // Remove duplicates by first sorting and then removing consecutive
    // duplicates
    std::sort(edge_vector.begin(), edge_vector.end(), [](const edge &e1, const edge &e2) { return (e1.i == e2.i) ? (e1.j < e2.j) : (e1.i < e2.i); });

    std::vector<edge> dups_removed;
    for (auto it = edge_vector.begin(); it != edge_vector.end();) {
        const edge &e = *it;
        dups_removed.push_back(e);

        do {
            ++it;
        } while ((it != edge_vector.end()) && (*it == e));
    }
    edge_vector = std::move(dups_removed);

    reset_progress();
}


void rng::sort(void)
{
    std::sort(edge_vector.begin(), edge_vector.end());
}
