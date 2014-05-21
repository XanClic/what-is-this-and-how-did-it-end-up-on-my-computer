#include <cstdio>

#include "kd_tree.hpp"
#include "point.hpp"


// Who needs other dimensions anyway
template<> void kd_tree_node<3>::dump(unsigned level_indentation, unsigned indentation) const
{
    if (leaf()) {
        printf("%*s%p\n", indentation, "", this);
    } else {
        printf("%*s%p (dim %i, split %f)\n", indentation, "", this, split_dim, split_val);
    }
    indentation += level_indentation;

    if (leaf()) {
        for (const point *pt: *this) {
            printf("%*s(%f; %f; %f)\n", indentation, "", pt->position.x(), pt->position.y(), pt->position.z());
        }
    } else {
        left_child ->dump(level_indentation, indentation);
        right_child->dump(level_indentation, indentation);
    }
}


template<> void kd_tree<3>::dump(unsigned level_indentation, unsigned indentation) const
{
    root_node->dump(level_indentation, indentation);
}
