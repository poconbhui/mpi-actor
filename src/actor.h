#ifndef ACTOR_ACTOR_H_
#define ACTOR_ACTOR_H_

#include <iostream>
#include <mpi.h>

#include "./id.h"
#include "./distributed_factory.h"
#include "./compound_message.h"


namespace ActorModel {


/**
 * Actor
 *
 * This is the base class that all actors should inherit.
 * It includes functionality for sending and receiving messages to and
 * from other actors. It can give birth to actors and die.
 *
 * This class is strongly coupled with the Director class, which does
 * much of the initialization and execution of actors.
 */
class Actor {

friend class Director;

public:

    /**
     * The constructor, destructor and main methods can be overloaded by
     * an inheriting class. The main function will be executed in a loop
     * by the Director until the actor dies.
     *
     * No communications or births should be requested in the constructor
     * or the destructor.
     */

    // Constructor
    Actor(): _is_dead(false), _id() {}

    // Destructor
    virtual ~Actor(){}

    // The main function that must be overloaded when defining a new actor.
    virtual void main()=0;


    // Request this actor dies.
    void die(void) {
        _is_dead = true;
    }

    // Check if the actor is dead.
    bool is_dead(void) {
        return _is_dead;
    }


    // Get the id of this actor.
    Id id(void) {
        return _id;
    }


    // Give birth to a child. The id of the child is returned immediately.
    template<class T>
    Id give_birth(void) {
        return _distributed_factory->request_distributed_child<T>();
    }


    /**
     * Pieces for sending tagged messages between actors
     */

    // Message type sent between actors.
    class Message: public CompoundMessage {
    public:
        struct MetaData {
            Id sender_id;
            int tag;
        };

        // Get the id of the sender.
        Id sender(void) {
            return metadata<MetaData>().sender_id;
        }

        // Get the tag on the message (different from MPI tag).
        int tag(void) {
            return metadata<MetaData>().tag;
        }
    };

    // Send an array of data
    template<class T>
    void send_message(Id const& actor_id, T *data, size_t data_count, int tag) {
        Message::MetaData metadata;

        metadata.sender_id = _id;
        metadata.tag       = tag;

        Message::send_message<T, Message::MetaData>(
            actor_id.rank(), actor_id.gid(),
            data, data_count, &metadata, _comm
        );
    }

    // Send an individual datum
    template<class T>
    void send_message(Id const& actor_id, T data, int tag) {
        send_message<T>(actor_id, &data, 1, tag);
    }

    // Check and receive a message if one is waiting.
    bool get_message(Message* my_message) {
        return my_message->receive_message(MPI_ANY_SOURCE, _id.gid(), _comm);
    }


private:

    // Initialize an actor with a given id, communicator
    // and distributed factory.
    void initialize_comms(
        Id id, MPI_Comm comm,
        DistributedFactory<Actor> *distributed_factory
    ) {
        _id = id;
        _comm = comm;
        _distributed_factory = distributed_factory;
    }

    // Death state of an actor.
    bool _is_dead;

    // Ids of the actor.
    Id _id;

    // Communicator to send messages over.
    MPI_Comm _comm;

    // Distributed factory class to use to request births.
    DistributedFactory<Actor> *_distributed_factory;
};


}  // namespace ActorModel


#endif
