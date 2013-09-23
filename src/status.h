#ifndef STATUS_H_
#define STATUS_H_

namespace ActorModel {


/**
 * Status
 *
 * The Status class provides information about the next message
 * waiting in the pipeline.
 */
class Status {
public:
    Status():
        _msg_state(0), _mpi_status()
    {}

    Status(int source, int tag, MPI_Comm comm) {
        MPI_Iprobe(source, tag, comm, &_msg_state, &_mpi_status);
    }


    // The source rank of the incoming message
    int source(void) {
        return _mpi_status.MPI_SOURCE;
    }

    // The message tag of the incoming message
    int tag(void) {
        return _mpi_status.MPI_TAG;
    }


    // The state of messages waiting in the pipeline
    int msg_state(void) {
        return _msg_state;
    }

    // Check if a message is waiting in the pipeline
    bool is_waiting(void) {
        return (msg_state() == MSG_WAITING);
    }

    // Get the count in MPI_BYTE of the incoming message
    int get_count(void) {
        int count;
        MPI_Get_count(&_mpi_status, MPI_BYTE, &count);

        return count;
    }


    // Message waiting status values
    enum {
        MSG_WAITING = 1,
        NO_MSG_WAITING = 0
    };

private:

    MPI_Status _mpi_status;
    int _msg_state;
};


}  // namespace ActorModel

#endif  // STATUS_H_
