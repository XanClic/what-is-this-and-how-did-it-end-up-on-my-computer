#include <cstdio>

#include "kd_tree.hpp"
#include "point.hpp"


// Who needs other dimensions anyway
template<> void kd_tree<3>::dump(unsigned level_indentation, unsigned indentation) const
{
    printf("%*s%p\n", indentation, "", this);
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
