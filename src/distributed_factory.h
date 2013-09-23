#ifndef ACTOR_ACTOR_DISTRIBUTER_H_
#define ACTOR_ACTOR_DISTRIBUTER_H_

#include <mpi.h>

#include "./factory.h"
#include "./id.h"
#include "./status.h"

namespace ActorModel {


/**
 * DistributedFactory
 *
 * The distributed factory class manages balancing instances of classes
 * across processes. One process can request that an instance is created
 * and another will receive the request to create it.
 *
 * As it requires a collective routine to initialize it, it must be
 * initialized simultaneously by all processes using it and have the
 * appropriate communicator passed to it.
 */
template<class F>
class DistributedFactory: public Factory<F> {
public:
    DistributedFactory(MPI_Comm comm_in=MPI_COMM_WORLD) {
        MPI_Comm_dup(comm_in, &_distributer_comm);

        MPI_Comm_rank(_distributer_comm, &_comm_rank);
        MPI_Comm_size(_distributer_comm, &_comm_size);

        _current_rank = _comm_rank;
    }

    ~DistributedFactory() {
        // Clean up all outstanding requests
        while(is_child_waiting()){
            int request[5];
            get_requested_child_data(request);
        }

        MPI_Comm_free(&_distributer_comm);
    }


    // Request that an instance of T be created on some process.
    // If no rank is specified, an appropriate rank will be chosen
    // in a balanced manner.
    enum{ BIRTH_REQUEST };
    template<class T>
    Id request_distributed_child(int rank=-1) {
        int factory_id = Factory<F>::template get_id<T>();
        Id child_id = new_global_id(rank);

        int request[3] = {
            factory_id,
            child_id.rank(), child_id.gid(),
        };

        MPI_Bsend(
            request, 3, MPI_INT, child_id.rank(),
            BIRTH_REQUEST, _distributer_comm
        );

        return child_id;
    }

    // Check if there are any outstanding requests for a child to be
    // created.
    bool is_child_waiting(void) {
        Status status(MPI_ANY_SOURCE, BIRTH_REQUEST, _distributer_comm);

        return status.is_waiting();
    }


    // A simple structure to return an instance of a child, a subclass
    // of F, along with an id for that child and the id of the parent.
    struct Child {
        F *child;
        Id child_id;
    };


    // Generate a child that is waiting to be created.
    // A null child is created if none is waiting.
    Child generate_requested_child(void) {
        if(is_child_waiting()) {
            Child new_child;

            int request[3];
            get_requested_child_data(request);
        
            int factory_id = request[0];
            new_child.child = Factory<F>::create_from_id(factory_id);

            new_child.child_id = Id(request[1], request[2]);

            return new_child;
        } else {
            Child null_child;

            null_child.child = NULL;
            null_child.child_id = Id(-1, -1);

            return null_child;
        }
    }


    // Get an id that is unique across processes, along with a
    // rank to place a child on.
    Id new_global_id(int rank=-1) {
        if(rank < 0) {
            rank = _current_rank;

            // Increment rank to use next.
            _current_rank = (_current_rank+1) % _comm_size;
        }

        int global_id = Id::new_global_id();

        Id id(rank, global_id);

        return id;
    }

private:
    // Receive data from an incoming message
    void get_requested_child_data(int request[3]) {
        MPI_Recv(
            request, 3, MPI_INT,
            MPI_ANY_SOURCE, BIRTH_REQUEST, _distributer_comm,
            MPI_STATUS_IGNORE
        );
    }

    MPI_Comm _distributer_comm;

    int _comm_rank;
    int _comm_size;

    int _current_rank;
};


}  // namespace ActorModel

#endif  // ACTOR_ACTOR_DISTRIBUTER_H_
