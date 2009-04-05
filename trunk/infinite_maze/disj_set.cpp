#include "disj_set.h"

DisjSet::DisjSet(unsigned max_num_elems)
{
    parents_ = new unsigned[max_num_elems];
    ranks_ = new unsigned[max_num_elems];
    for (unsigned ei = 0; ei < max_num_elems; ei++)
    {
        parents_[ei] = ei;
        ranks_[ei] = 0;
    }
}

DisjSet::~DisjSet()
{
    delete[] parents_;
    delete[] ranks_;
}

unsigned DisjSet::find(unsigned elem_index)
{
    if (parents_[elem_index] != elem_index)
        parents_[elem_index] = find(parents_[elem_index]);
    return parents_[elem_index];
}

void DisjSet::join(unsigned elem_index_1, unsigned elem_index_2)
{
    unsigned root_1 = find(elem_index_1);
    unsigned root_2 = find(elem_index_2);

    if (ranks_[root_1] > ranks_[root_2])
    {
        parents_[root_2] = parents_[root_1];
    }
    else if (ranks_[root_1] < ranks_[root_2])
    {
        parents_[root_1] = parents_[root_2];
    }
    else if (root_1 != root_2)
    {
        parents_[root_2] = parents_[root_1];
        ranks_[root_1]++;
    }
}
