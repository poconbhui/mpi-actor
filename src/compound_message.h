#ifndef ACTOR_COMPOUND_MESSAGE_H_
#define ACTOR_COMPOUND_MESSAGE_H_


namespace ActorModel {


/**
 * CompoundMessage
 *
 * This class handles memory allocation for sending and receiving
 * messages, and also sends and receives 2 messages in a row to specific tags.
 *
 * Either a simple copy operator can be used to retreive data, or
 * an array pointer can be passed in with a length, and that many
 * elements will be copied to the pointer using the copy operator.
 *
 * Metadata is limited to fixed types.
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
 */
class CompoundMessage {
public:

    // Get data sizes
    template<class T>
    int metadata_size(void) { return _metadata.size()/sizeof(T); }
    int metadata_size(void) { return _metadata.size(); }

    template<class T>
    int data_size(void) { return _data.size()/sizeof(T); }
    int data_size(void) { return _data.size(); }


    // Receive metadata from received message.
    // This does a straight cast of data, so be sure you
    // know what type data you've received!
    template<class T>
    T metadata(void) {
        return *reinterpret_cast<T*>(&_metadata[0]);
    }

    // Receive data from received message. Same caveat as above.
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
    int source(void) { return _source; }
    int tag(void) { return _tag; }


    // Send a compound message
    template<class DT, class MDT>
    static void send_message(
        int send_rank, int send_tag,
        DT *data, size_t data_count,
        MDT *metadata, MPI_Comm comm
    ) {
        // Send metadata
        MPI_Bsend(
            metadata, sizeof(MDT), MPI_BYTE,
            send_rank, send_tag,
            comm
        );

        // Send data
        MPI_Bsend(
            data, data_count*sizeof(DT), MPI_BYTE,
            send_rank, send_tag,
            comm
        );
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
        enum { msg_waiting = 1 };

        int msg_state;
        MPI_Status status;
        int count;

        // Check if a message is waiting
        MPI_Iprobe(source, tag, comm, &msg_state, &status);

        if(msg_state == msg_waiting) {

            // Need to do this in case source or tag were
            // MPI_ANY_SOURCE or MPI_ANY_TAG
            source = status.MPI_SOURCE;
            tag    = status.MPI_TAG;

            receive_individual(source, tag, comm, &_metadata)
            receive_individual(source, tag, comm, &_data)

            _source = source;
            _tag    = tag;
        }

        return (msg_state == msg_waiting);
    }


private:

    /**
     * Receive an individual message into the given data vector.
     */
    bool receive_individual(
        int source, int tag, MPI_Comm comm, std::vector<char> *data
    ) {
        MPI_Status status;
        int count;


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
        MPI_Probe(source, tag, comm, &status);
        MPI_Get_count(&status, MPI_BYTE, &count);

        // If count undefined, exit early
        if(count == MPI_UNDEFINED) return false;


        /*
         * Resize data and receive
         */
        data->resize(count);

        MPI_Recv(
            &(data->operator[](0)), count, MPI_BYTE,
            source, tag, comm,
            &status
        );


        return true;
    }


    // Vectors storing message data
    std::vector<char> _metadata;
    std::vector<char> _data;

    // Data about origins of last message
    int _source;
    int _tag;
};


}  // namespace ActorModel


#endif  // ACTOR_COMPOUND_MESSAGE_H_
