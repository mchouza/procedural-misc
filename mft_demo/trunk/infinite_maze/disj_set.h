#ifndef DISJ_SET_H
#define DISJ_SET_H

class DisjSet
{
    unsigned* parents_;
    unsigned* ranks_;

public:
    DisjSet(unsigned max_num_elems);
    virtual ~DisjSet();

    void makeSet(unsigned elem_index);
    void join(unsigned elem_index_1, unsigned elem_index_2);
    unsigned find(unsigned elem_index);
};

#endif
