#include <iostream>
#include <cmath>

#include <mpi.h>

/**
 * super_quick_test is a super lightweight (and super ugly) test framework
 * for use with MPI.
 */

bool super_quick_test_check;
MPI_Comm super_quick_test_comm;
int super_quick_test_rank;

#define INIT_SQT() { \
    MPI_Comm_dup(MPI_COMM_WORLD, &super_quick_test_comm); \
    MPI_Comm_rank(super_quick_test_comm, &super_quick_test_rank); \
}

#define RUN_TEST(test_name) { \
    MPI_Barrier(super_quick_test_comm); \
    if(super_quick_test_rank == 0) { \
        std::cout << "Running " << #test_name << std::endl; \
    } \
    \
    super_quick_test_check = true; \
    test_name(); \
    { \
        int reduce_check; \
        int reduce_val = (super_quick_test_check)? 0 : 1; \
        MPI_Allreduce( \
            &reduce_val, &reduce_check, 1, \
            MPI_INT, MPI_SUM, super_quick_test_comm \
        ); \
        \
        super_quick_test_check = reduce_check == 0; \
    } \
    if(!super_quick_test_check && super_quick_test_rank == 0) { \
        std::cout << #test_name << " failed" << std::endl; \
    } \
}

#define REQUIRE(expression) { \
    bool result = (expression); \
    if(!(result)) { \
        std::cout << #expression << " failed on rank " \
                  << super_quick_test_rank \
                  << " (line " << __LINE__  << ")" << std::endl; \
    } \
    super_quick_test_check &= (result); \
}


bool sqt_fleq(double a, double b) {
    double diff = std::abs(a - b);
    return diff < 1e-4;
}
