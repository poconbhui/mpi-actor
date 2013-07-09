#ifndef ACTOR_ID_H_
#define ACTOR_ID_H_


namespace ActorModel {


/**
 * Id
 *
 * The id class is used throughout ActorModel to refer to instances of
 * actors in the system. It quickly identifies actors by providing
 * the rank on which they live and an id (hopefully) unique to them.
 */
class Id {
public:
    Id(): rank(0), id(0) {}
    Id(int rank_in, int id_in): rank(rank_in), id(id_in) {}

    /**
     * Get an id, guaranteed to be unique across all processes
     *
     * The scheme used is that each process produces numbers where
     * number % rank == 0. This is done by initializing an id to
     * their rank, and then incrementing by the total number of
     * processes.
     */
    static int new_global_id(void) {
        static int last_id = -1;
        static int mpi_rank;
        static int mpi_size;

        // If uninitialized, initialize
        if(last_id == -1) {
            MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
            MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

            last_id = mpi_rank;
        }

        last_id += mpi_size;

        return last_id;
    }


    int rank;
    int id;
};


}  // namespace ActorModel


#endif  // ACTOR_ID_H_
