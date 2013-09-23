#ifndef ACTOR_DIRECTOR_H_
#define ACTOR_DIRECTOR_H_

#include <mpi.h>
#include <queue>

#include "./id.h"
#include "./actor.h"
#include "./distributed_factory.h"


namespace ActorModel {


/**
 * Director
 *
 * The director class manages how actors are initialized, scheduled
 * and executed.
 *
 * It requires the use of a collective routine to begin with, so all
 * processes using the director must initialize it simultaneously
 * and pass in the appropriate communicator.
 */
class Director {
public:

    Director(MPI_Comm comm_in=MPI_COMM_WORLD, int sync_interval=1):
        _actor_distributer(comm_in),
        _is_ended(false),
        _sync_interval(sync_interval),
        _tick_count(0)
    {
        // Constructor synchronized by MPI_Com_dup

        // Set up MPI communicators and data
        MPI_Comm_dup(comm_in, &_actor_comm);
        MPI_Comm_dup(comm_in, &_director_comm);

        MPI_Comm_rank(_director_comm, &_comm_rank);
        MPI_Comm_size(_director_comm, &_comm_size);
    }

    ~Director() {

        // Synchronize destructors
        MPI_Barrier(_director_comm);

        // Empty out the actor queue
        empty_queue();

        // Clean up actor messages
        {
            Message message;
            while(message.receive(MPI_ANY_SOURCE, MPI_ANY_TAG, _actor_comm));
        }

        // Clean up director messages
        {
            Message message;
            while(message.receive(MPI_ANY_SOURCE, MPI_ANY_TAG, _director_comm));
        }


        // Free all communicators
        MPI_Comm_free(&_actor_comm);
        MPI_Comm_free(&_director_comm);
    }

    // Define a root director to easily run stuff on just one process
    bool is_root(void) {
        return _comm_rank == 0;
    }


    // Add an actor to the cast and return a reference to that actor.
    // This is the main method of getting data in and out of the
    // cast.
    template<class T>
    T* add_actor(void) {
        T *new_actor = new T;

        new_actor->initialize_comms(
            _actor_distributer.new_global_id(_comm_rank),
            _actor_comm,
            &_actor_distributer
        );

        ActorWrap actor_wrap(new_actor, false);

        _actor_queue.push(actor_wrap);

        return new_actor;
    }

    // Register an actor type for use in factory classes.
    // Every process the director is running on must register all
    // actor used in the cast, and must all register them in the same order.
    template<class T>
    void register_actor(void) {
        _actor_distributer.register_child<T>();
    }

    // Get the current load the director is under. That is,
    // the current number of actors it's managing.
    int get_load(void) {
        return _actor_queue.size();
    }


    // Get the total load over all directors.
    // This is a collective routine. All processes must call
    // this at the same time.
    int get_global_load(void) {
        int global_load;
        int my_load = get_load(); // Current Load

        MPI_Allreduce(
            &my_load, &global_load, 1, MPI_INT, MPI_SUM, _director_comm
        );

        return global_load;
    }


    // Request that all directors stop and the run ends
    enum { END };
    void end(void) {
        for(int i=0; i<_comm_size; i++) {
            int is_ended = 1;
            MPI_Bsend(&is_ended, 1, MPI_INT, i, END, _director_comm);
        }
    }



    // Run the process for the requested number of ticks.
    // If no parameter is passed in, or a negative one is, the director
    // will run until it ends.
    void run(int ticks=0) {
        int end_tick_count=_tick_count + ticks;
        while(!_is_ended && (_tick_count < end_tick_count || ticks <= 0)) {
            _tick_count++;

            sync_states();

            if(_actor_queue.empty()) continue;

            // Get the next actor in the queue
            ActorWrap actor_wrap = _actor_queue.front();
            _actor_queue.pop();

            // Run the actor's main function
            actor_wrap.actor->main();

            // Add the actor back to the end of the queue if they're not dead
            if(!actor_wrap.actor->is_dead()) {
                _actor_queue.push(actor_wrap);
            } else {
                if(actor_wrap.deletable == true) {
                    delete actor_wrap.actor;
                }
            }
        }

        _is_ended = false;
    }


private:
    /*
     * Global state management
     */

    // Check if any directors on any processes have died.
    bool get_global_ended(void) {
        Status status(MPI_ANY_SOURCE, END, _director_comm);

        int global_done = 0;
        if(status.is_waiting()) {
            MPI_Recv(
                &global_done, 1, MPI_INT,
                MPI_ANY_SOURCE, MPI_ANY_TAG, _director_comm,
                MPI_STATUS_IGNORE
            );
        }

        // Expect sum to be 0 unless at least one director is dead
        return (global_done != 0);
    }

    // Share information between directors, like global ended status
    // and the global load.
    void sync_states(void) {

        // Add waiting actors
        add_waiting_actors();

        // Check if end request has been made
        _is_ended |= get_global_ended();

        // Do a check for finished loads every _sync_interval ticks
        if((_tick_count % _sync_interval) == 0) {

            // Generate requested actors before comparing loads
            MPI_Barrier(_director_comm);
            add_waiting_actors();

            int global_load = get_global_load();
            _is_ended |= (global_load == 0);
        }
    }


    /*
     * Actor execution management
     */

    // A wrapper class to go in our queue, so we know
    // whose memory we are managing
    struct ActorWrap {
        ActorWrap(Actor* actor_in, bool deletable_in):
            actor(actor_in), deletable(deletable_in)
        {}

        Actor* actor;
        bool deletable;
    };

    // Clean out actor queue
    void empty_queue(void) {
        while(!_actor_queue.empty()) {
            ActorWrap actor_wrap = _actor_queue.front();
            _actor_queue.pop();

            if(actor_wrap.deletable) {
                delete actor_wrap.actor;
            }
        }
    }

    std::queue<ActorWrap> _actor_queue;


    /*
     * Distributer actor management
     */

    // Add any actors waiting to be born to the cast.
    void add_waiting_actors(void) {
        while(_actor_distributer.is_child_waiting()) {
            DistributedFactory<Actor>::Child new_actor_data =
                _actor_distributer.generate_requested_child();

            Actor* new_actor = new_actor_data.child;
            Id actor_id  = new_actor_data.child_id;

            new_actor->initialize_comms(
                actor_id, _actor_comm, &_actor_distributer
            );

            _actor_queue.push(ActorWrap(new_actor, true));
        }
    }

    DistributedFactory<Actor> _actor_distributer;


    MPI_Comm _actor_comm;
    MPI_Comm _director_comm;

    int _comm_rank;
    int _comm_size;

    bool _is_ended;
    int _sync_interval;
    int _tick_count;
};


}  // namespace ActorModel

#endif  // ACTOR_DIRECTOR_H_
