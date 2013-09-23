#ifndef ACTOR_COMPOUND_MESSAGE_H_
#define ACTOR_COMPOUND_MESSAGE_H_


#include "./message.h"


namespace ActorModel {


/**
 * CompoundMessage
 *
 * This class sends and receives 2 messages in a row to specific tags.
 * It is expected to be used exclusively on a particular communicator
 * as it makes the assumption that all messages passed on this communicator
 * are passed in pairs.
 *
 * It sends Metadata and Data types.
 * Metadata is limited to fixed types, but Data types may be fixed or
 * array types.
 */
class CompoundMessage {
public:

    // Get data sizes
    template<class T>
    int metadata_size(void) {
        return _metadata.data_size<T>();
    }
    int metadata_size(void) {
        return _metadata.data_size();
    }

    template<class T>
    int data_size(void) {
        return _data.data_size<T>();
    }
    int data_size(void) {
        return _data.data_size();
    }


    // Receive metadata from received message.
    // This does a straight cast of data, so be sure you
    // know what type data you've received!
    template<class T>
    T metadata(void) {
        return _metadata.data<T>();
    }

    // Receive data from received message. Same caveat as above.
    template<class T>
    T data(void) {
        return _data.data<T>();
    }

    // Receive array data from received message.
    template<class T>
    void data(T *buffer, size_t count) {
        _data.data<T>(buffer, count);
    }


    // Find some information about the message.
    int source(void) { return _metadata.source(); }
    int tag(void) { return _metadata.tag(); }


    // Send a compound message
    template<class DT, class MDT>
    static void send_message(
        int send_rank, int send_tag,
        DT *data, size_t data_count,
        MDT *metadata, MPI_Comm comm
    ) {
        // Send metadata
        Message::send<MDT>(send_rank, send_tag, metadata, 1, comm);

        // Send data
        Message::send<DT>(send_rank, send_tag, data, data_count, comm);
    }

    // Send a single data message
    template<class DT, class MDT>
    static void send_message(
        int send_rank, int send_tag,
        DT &data,
        MDT *metadata, MPI_Comm comm
    ) {
        send_message<DT, MDT>(send_rank, send_tag, data, 1, metadata, comm);
    }


    // Receive a compound message.
    bool receive_message(int source, int tag, MPI_Comm comm) {

        if(Status(source, tag, comm).is_waiting()) {

            if(!_metadata.receive(source, tag, comm)) {
                return false;
            }
            if(!_data.receive(_metadata.source(), _metadata.tag(), comm)) {
                return false;
            }

            return true;
        }

        return false;
    }


private:

    // metadata and data messages
    Message _metadata;
    Message _data;
};


}  // namespace ActorModel


#endif  // ACTOR_COMPOUND_MESSAGE_H_
