#ifndef GRID_H_
#define GRID_H_

#include "../src/id.h"


struct Grid {
public:
    // Hard coded grid size
    static const int num_cells = 16;

    // A list of ids of every cell in the grid
    ActorModel::Id cell_ids[num_cells];
};


#endif  // GRID_H_
