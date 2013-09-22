#ifndef ACTOR_ID_H_
#define ACTOR_ID_H_


namespace ActorModel {


/**
 * Id
 *
 * The Id class is used throughout ActorModel to refer to instances of
 * actors in the system. It quickly identifies actors by providing
 * the rank on which they live and a gid (hopefully) unique to them.
 */
class Id {
public:
    // Initialize an empty Id
    Id(): _rank(0), _gid(0) {}

    // Initialize an Id with a given rank and gid.
    Id(int rank_in, int gid_in): _rank(rank_in), _gid(gid_in) {}


    // Accessor for _rank
    int rank() const {
        return _rank;
    }

    // Accessor for _gid
    int gid() const {
        return _gid;
    }


    /**
     * Get an id, guaranteed to be unique across all processes
     *
     * The scheme used is that each process produces numbers where
     * number % rank == 0. This is done by initializing an id to
     * their rank, and then incrementing by the total number of
     * processes.
     */
    static int new_global_id(void) {
        static int last_gid = -1;
        static int mpi_rank;
        static int mpi_size;

        // If uninitialized, initialize
        if(last_gid == -1) {
            MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
            MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

            last_gid = mpi_rank;
        }

        last_gid += mpi_size;

        return last_gid;
    }


private:
    int _rank;
    int _gid;
};


}  // namespace ActorModel


#endif  // ACTOR_ID_H_
