#include "./super_quick_test.h"

#include "../src/actor.h"
#include "../src/director.h"

using namespace ActorModel;


struct TestMessageDataType {
    int a;
    int b;
    double c;
};

void test_message(void) {
    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    Message message;

    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int send_rank = (rank+1)%size;
    int recv_rank = (rank-1+size)%size;


    // Send a number of messages
    for(int send_tag=0; send_tag < 5; send_tag++) {
        TestMessageDataType data1 = {rank, send_tag, 0.1};

        Message::send<TestMessageDataType>(
            send_rank, send_tag, &data1, 1, comm
        );
    }

    // Ensure all messages have sent.
    MPI_Barrier(comm);

    // Run through tags backwards to ensure tag is important and not
    // message send/receive order
    for(int recv_tag=4; recv_tag >= 0; recv_tag--) {
        message.receive(MPI_ANY_SOURCE, recv_tag, comm);

        REQUIRE(
            message.data_size() == sizeof(TestMessageDataType)
        );
        REQUIRE(
            message.data_size<TestMessageDataType>() == 1
        );

        REQUIRE(message.source() == recv_rank);
        REQUIRE(message.tag() == recv_tag);

        TestMessageDataType data1 = message.data<TestMessageDataType>();

        REQUIRE(data1.a == recv_rank);
        REQUIRE(data1.b == recv_tag);
        REQUIRE(sqt_fleq(data1.c, 0.1));
    }

    REQUIRE(!Status(MPI_ANY_SOURCE, MPI_ANY_TAG, comm).is_waiting());
    REQUIRE(!message.receive(MPI_ANY_SOURCE, MPI_ANY_TAG, comm));

    // Ensure all messages have been exchanged
    MPI_Barrier(comm);

    // Test array sending and receiving
    int array1_size = 10;
    int array1[array1_size];

    for(int i=0; i<array1_size; i++) {
        array1[i] = i;
    }

    // Send and receive message
    Message::send<int>(
        send_rank, 0, array1, array1_size, comm
    );

    // Ensure all messages are sent
    MPI_Barrier(comm);
    
    message.receive(MPI_ANY_SOURCE, 0, comm);

    REQUIRE(message.data_size<int>() == array1_size);

    int recv_array1[array1_size];

    message.data(recv_array1, array1_size);

    for(int i=0; i<array1_size; i++) {
        REQUIRE(recv_array1[i] == array1[i]);
    }


    MPI_Comm_free(&comm);
}



struct TestCompoundMessageDataType1 {
    int a;
    int b;
    double c;
};

struct TestCompoundMessageDataType2 {
    double x;
    double y;
    int z;
};

void test_compound_message(void) {
    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    CompoundMessage message;

    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int send_rank = (rank+1)%size;
    int recv_rank = (rank-1+size)%size;

    // Send a number of messages
    for(int send_tag=0; send_tag < 5; send_tag++) {
        TestCompoundMessageDataType1 data1 = {rank, send_tag, 0.1};
        TestCompoundMessageDataType2 data2 = {0.0, 0.1*rank, rank};

        CompoundMessage::send_message
            <TestCompoundMessageDataType1, TestCompoundMessageDataType2>
            (send_rank, send_tag, &data1, 1, &data2, comm);
    }

    // Ensure all messages have sent.
    MPI_Barrier(comm);

    // Run through tags backwards to ensure tag is important and not
    // message send/receive order
    for(int recv_tag=4; recv_tag >= 0; recv_tag--) {
        message.receive_message(MPI_ANY_SOURCE, recv_tag, comm);

        REQUIRE(
            message.data_size() == sizeof(TestCompoundMessageDataType1)
        );
        REQUIRE(
            message.data_size<TestCompoundMessageDataType1>() == 1
        );
        REQUIRE(
            message.metadata_size() == sizeof(TestCompoundMessageDataType2)
        );

        REQUIRE(message.source() == recv_rank);
        REQUIRE(message.tag() == recv_tag);

        TestCompoundMessageDataType1 data1 =
            message.data<TestCompoundMessageDataType1>();

        REQUIRE(data1.a == recv_rank);
        REQUIRE(data1.b == recv_tag);
        REQUIRE(sqt_fleq(data1.c, 0.1));


        TestCompoundMessageDataType2 data2 =
            message.metadata<TestCompoundMessageDataType2>();

        REQUIRE(sqt_fleq(data2.x, 0.0));
        REQUIRE(sqt_fleq(data2.y, 0.1*recv_rank));
        REQUIRE(data2.z == recv_rank);
    }

    REQUIRE(!message.receive_message(MPI_ANY_SOURCE, MPI_ANY_TAG, comm));

    // Ensure all messages have been exchanged
    MPI_Barrier(comm);

    // Test array sending and receiving
    int array1_size = 10;
    int array1[array1_size];
    int data2 = array1_size;

    for(int i=0; i<array1_size; i++) {
        array1[i] = i;
    }

    // Send and receive message
    CompoundMessage::send_message<int, int>(
        send_rank, 0, array1, array1_size, &data2, comm
    );

    // Ensure all messages are sent
    MPI_Barrier(comm);
    
    message.receive_message(MPI_ANY_SOURCE, 0, comm);

    int recv_metadata = message.metadata<int>();
    REQUIRE(recv_metadata == array1_size);
    int recv_array1[recv_metadata];

    message.data(recv_array1, recv_metadata);

    for(int i=0; i<array1_size; i++) {
        REQUIRE(recv_array1[i] == array1[i]);
    }

    MPI_Comm_free(&comm);

}


void test_global_ids(void) {
    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    int rank;
    int size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int num_tries = 5;
    for(int i=0; i<num_tries; i++) {
        int id = Id::new_global_id();

        // Send the generated id to rank 0 for checking
        MPI_Bsend(&id, 1, MPI_INT, 0, 0, comm);
    }

    MPI_Barrier(comm);

    // If rank 0, receive all ids and compare them together
    if(rank == 0) {
        int ids[num_tries*size];

        for(int i=0; i<num_tries*size; i++) {
            MPI_Recv(
                &ids[i], 1, MPI_INT,
                MPI_ANY_SOURCE, 0, comm,
                MPI_STATUS_IGNORE
            );
        }

        for(int i=0; i<num_tries*size; i++)
        for(int j=0; j<num_tries*size; j++) {
            if(i==j) continue;

            REQUIRE(ids[i] != ids[j]);
        }
    }

    MPI_Comm_free(&comm);
}


class TestActorInheritance: public Actor {
public:
    TestActorInheritance(): checkInt(11), ext_checkInt(NULL) {}
    void main(void) {
        checkInt=22;
    }
    ~TestActorInheritance() {
        if(ext_checkInt) {
            *ext_checkInt=33;
        }
    }

    int checkInt;
    int *ext_checkInt;
};


void test_actor_inheritance(void) {
    int ext_checkInt=0;

    TestActorInheritance *test_actor = new TestActorInheritance;
    test_actor->ext_checkInt = &ext_checkInt;

    REQUIRE(test_actor->checkInt == 11);

    Actor *actor = test_actor;

    actor->main();
    REQUIRE(test_actor->checkInt == 22);

    delete actor;
    REQUIRE(ext_checkInt == 33);
}



int checkTestActorFactory=0;

class TestActorFactory1: public Actor {
    void main(void){}

    ~TestActorFactory1(void) {
        checkTestActorFactory=22;
    }
};

class TestActorFactory2: public Actor {
    void main(void){}
    ~TestActorFactory2(void) {
        checkTestActorFactory=33;
    }
};

void test_actor_factory(void) {
    Factory<Actor> actor_factory;

    actor_factory.register_child<TestActorFactory1>();
    actor_factory.register_child<TestActorFactory2>();

    int id1 = actor_factory.get_id<TestActorFactory1>();
    int id2 = actor_factory.get_id<TestActorFactory2>();

    checkTestActorFactory = 0;

    Actor *actor1 = actor_factory.create_from_id(id1);
    Actor *actor2 = actor_factory.create_from_id(id2);

    REQUIRE(checkTestActorFactory ==  0);

    delete actor1;
    REQUIRE(checkTestActorFactory == 22);

    delete actor2;
    REQUIRE(checkTestActorFactory == 33);
}

class TestDistributedFactoryParent {
public:
    virtual int test(void) {
        return 0;
    }
};

class TestDistributedFactoryChild: public TestDistributedFactoryParent {
public:
    virtual int test(void) {
        return 1;
    }
};

void test_distributed_factory(void) {
    DistributedFactory<TestDistributedFactoryParent> distributed_factory;

    MPI_Comm comm;
    int rank;
    int size;

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    Id my_id = distributed_factory.new_global_id(rank);

    int num_children = 5*size;

    DistributedFactory<TestDistributedFactoryParent>::Child child;

    distributed_factory.register_child<TestDistributedFactoryChild>();

    if(rank == 0) {
        for(int i=0; i<num_children; i++) {
            distributed_factory.request_distributed_child<
                TestDistributedFactoryChild
            >();
        }
    }

    // Ensure everyone has sent all their requests and they've all been
    // received
    MPI_Barrier(comm);

    bool child_waiting = false;

    if(distributed_factory.is_child_waiting()) {
        child_waiting = true;

        child = distributed_factory.generate_requested_child();

        int check = child.child->test();

        REQUIRE(check == 1);
    }

    REQUIRE(child_waiting);
}



struct BigData {
    double a;
    double b;
    double c;
    double d;
};

class CommunicationActor: public Actor {
public:
    CommunicationActor(): parent(-1, -1) {}
    void main(void){
        if(parent.rank() == -1) {
            Message message;
            if(get_message(&message)) {
                parent = message.data<Id>();
            }
        } else {
            BigData somedata = {5.1, 6.2, 7.3, 8.4};
            send_message<BigData>(parent, somedata, 0);

            die();
        }
    }

    Id parent;
};

class CommunicationActorManager: public Actor {
public:
    CommunicationActorManager(): initialized(false) {}

    void main(void){
        if(!initialized) {
            my_child = give_birth<CommunicationActor>();

            Id my_id = id();
            send_message<Id>(my_child, my_id, 0);

            initialized = true;
        }


        Message my_message;

        if(get_message(&my_message)) {
            if(my_message.tag() == 0) {
                received = my_message.data<BigData>();
            } 

            die();
        }
    }

    bool initialized;
    Id my_child;

    BigData received;
};


void test_actor_communication(void) {
    Director director;
    director.register_actor<CommunicationActor>();

    CommunicationActorManager *actor;
    
    if(director.is_root()) {
        actor = director.add_actor<CommunicationActorManager>();
    }

    director.run();

    if(director.is_root()) {
        REQUIRE(sqt_fleq(actor->received.a, 5.1));
        REQUIRE(sqt_fleq(actor->received.b, 6.2));
        REQUIRE(sqt_fleq(actor->received.c, 7.3));
        REQUIRE(sqt_fleq(actor->received.d, 8.4));
    }
}



class TestActorBirthAndDeath1: public Actor {
public:
    void main(void){
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        MPI_Bsend(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

        die();
    }
};

class TestActorBirthAndDeathManager: public Actor {
public:
    void main(void) {
        for(int i=0; i<birth_count; ++i) {
            give_birth<TestActorBirthAndDeath1>();
        }

        die();
    }

    // Should hopefully be enough to ensure at least one birth
    // on a different process
    int birth_count;
};


void test_actor_birth_and_death(void) {
    Director director;

    director.register_actor<TestActorBirthAndDeath1>();

    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Create 5 times as many actors as there are processes.
    // This should, hopefully, ensure that at least one actor
    // is defined on each process
    int birth_count = 5*size;

    if(director.is_root()) {
        TestActorBirthAndDeathManager *actor =
            director.add_actor<TestActorBirthAndDeathManager>();

        actor->birth_count = birth_count;
    }

    director.run();

    if(rank == 0) {
        bool all_processes_found = true;

        // Track number of times each rank is received
        int ranks[size];
        for(int i=0; i<size; i++) ranks[i] = 0;

        for(int i=0; i<birth_count; ++i) {
            int recv_rank;

            MPI_Recv(
                &recv_rank, 1, MPI_INT,
                MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE
            );

            ranks[recv_rank]++;
        }

        // Test that every rank has been sent at least once
        for(int i=0; i<size; i++) {
            all_processes_found &= (ranks[1] != 0);
        }

        REQUIRE(all_processes_found);
    }

}


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int *buffer = new int[1000];
    MPI_Buffer_attach(buffer, 1000*sizeof(int));

    INIT_SQT();

    RUN_TEST(test_message);

    RUN_TEST(test_compound_message);

    RUN_TEST(test_global_ids);

    RUN_TEST(test_actor_inheritance);

    RUN_TEST(test_actor_factory);

    RUN_TEST(test_distributed_factory);

    RUN_TEST(test_actor_communication);

    RUN_TEST(test_actor_birth_and_death);

    MPI_Finalize();
}
