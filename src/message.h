#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "./status.h"


namespace ActorModel {


/**
 * Message
 *
 * This class handles memory allocation for sending and receiving
 * messages. It also handles checking for messages and providing
 * message information through the Status subclass
 *
 * Either a simple copy operator can be used to retreive data, or
 * an array pointer can be passed in with a length, and that many
 * elements will be copied to the pointer using the copy operator.
 *
 * This passes messages using MPI_BYTE, so it likely won't work on
 * a hetrogenous system!
 * It also assumes that the size of the MPI_BYTE data type is the
 * same as char.
 *
 * This class assumes that std::vector holds data in contiguous memory.
 * There is a slightly fluffy line in the pre 2003 standard that
 * a vendor could use to justify using non-contiguous memory,
 * but it is believed that none have done so.
 * If this were the case, hopefully the tests for this class should fail.
 *
 * MPI_Bsend is used to avoid the need to manually handle data allocation
 * and deallocation and waiting for messages to finish sending etc.
 * The data passed in is simply copied to the attached MPI buffer
 * and the program continues without blocking.
 */
class Message {
public:

    // Get data size
    template<class T>
    int data_size(void) { return _data.size()/sizeof(T); }
    int data_size(void) { return _data.size(); }


    // Receive data from received message.
    // This does a straight cast of data, so be sure you
    // know what type data you've received!
    template<class T>
    T data(void) {
        return *reinterpret_cast<T*>(&_data[0]);
    }

    // Receive array data from received message.
    template<class T>
    void data(T *buffer, size_t count) {
        T *t_data = reinterpret_cast<T*>(&_data[0]);

        for(size_t i=0; i<count; i++) {
            buffer[i] = t_data[i];
        }
    }


    // Find some information about the message.
    int source(void) { return _status.source(); }
    int tag(void) { return _status.tag(); }


    // Send a message
    template<class DT>
    static void send(
        int send_rank, int send_tag, DT *data, size_t data_count, MPI_Comm comm
    ) {
        MPI_Bsend(
            data, data_count*sizeof(DT), MPI_BYTE,
            send_rank, send_tag,
            comm
        );
    }

    // Send a single data message
    template<class DT>
    static void send(
        int send_rank, int send_tag, DT &data, MPI_Comm comm
    ) {
        send<DT>(send_rank, send_tag, &data, 1, comm);
    }


    // Receive a message.
    bool receive(int source, int tag, MPI_Comm comm) {

        _status = Status(source, tag, comm);

        if(_status.is_waiting()) {

            source = _status.source();
            tag    = _status.tag();


            /**
             * Get size of incoming data
             *
             * Get size in bytes. _data and _metadata containers
             * are in char, which roughly translates to MPI_BYTE.
             *
             * MPI_BYTE is specifically used, because MPI_CHAR can
             * perform some unexpected byte swapping on hetrogeneous
             * architectures.
             * This is unwelcome because we are reading bytes directly
             * and unsafely casting them to the expected data type.
             */
            int count = _status.get_count();

            // If count undefined, an error has occurred. Exit early.
            if(count == MPI_UNDEFINED) return false;


            /*
             * Resize data and receive
             */
            _data.resize(count);

            MPI_Status ignore;
            MPI_Recv(
                &(_data[0]), count, MPI_BYTE,
                source, tag, comm,
                &ignore
            );


            // Everything completed successfully
            return true;
        }
        else {

            // No message has been received
            return false;
        }
    }


private:

    // Vector storing message data
    std::vector<char> _data;

    // Data about origins of last message
    Status _status;
};

}  // namespace ActorModel


#endif  // MESSAGE_H_
