#include <cstdio>

#include "kd_tree.hpp"
#include "point.hpp"


// Who needs other dimensions anyway
template<> void kd_tree_node<3>::dump(unsigned level_indentation, unsigned indentation) const
{
    if (!pt) {
        printf("%*s%p\n", indentation, "", this);
    } else {
        printf("%*s%p (dim %i, split (%f; %f; %f))\n", indentation, "", this, split_dim, pt->position.x(), pt->position.y(), pt->position.z());
    }
    indentation += level_indentation;

    if (leaf()) {
        if (!pt) {
            for (const point *ptl: *this) {
                printf("%*s(%f; %f; %f)\n", indentation, "", ptl->position.x(), ptl->position.y(), ptl->position.z());
            }
        }
    } else {
        if (left_child) {
            left_child ->dump(level_indentation, indentation);
        }
        if (right_child) {
            right_child->dump(level_indentation, indentation);
        }
    }
}


template<> void kd_tree<3>::dump(unsigned level_indentation, unsigned indentation) const
{
    root_node->dump(level_indentation, indentation);
}
