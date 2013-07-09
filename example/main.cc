#include <cstdlib>
#include <iostream>

#include "../src/director.h"
#include "./simulation.h"

#include "./cell.h"
#include "./frog.h"

int main(int argc, char *argv[]) {
    // scope ensures classes are destroyed before MPI_Finalize
    MPI_Init(&argc, &argv); {

        // Attach a big enough message buffer to run the simulation
        int buffer_size = 50000;
        void *buffer = ::operator new(buffer_size);
        MPI_Buffer_attach(buffer, buffer_size);

        /**
         * Create a director.
         *
         * The director class needs to be destroyed before MPI_Finalize is
         * called because the destructor performs some MPI routines.
         *
         * Director should communicate over MPI_COMM_WORLD and have
         * a very long sync interval.
         */
        ActorModel::Director director(MPI_COMM_WORLD, 50000);

        // Register the actors used in the model
        director.register_actor<Cell>();
        director.register_actor<Frog>();

        // Initialize Frog's RNG
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        RNG_state = -1-rank;
        initialiseRNG(&RNG_state);

        // Set up our simulation parameters
        if(director.is_root()) {
            Simulation *simulation = director.add_actor<Simulation>();

            // Some default values
            int initial_frog_count = 34;
            int infected_frog_count = 1;
            int max_frog_count = 100;
            double frog_output_interval = 0.005; // in seconds

            int num_cells = 16;

            double year_length = 0.01; // in seconds
            int years_to_model = 100;

            // Parse our input parameters, if any
            if(argc > 1) initial_frog_count = atoi(argv[1]);
            if(argc > 2) infected_frog_count = atoi(argv[2]);
            if(argc > 3) max_frog_count = atoi(argv[3]);
            if(argc > 4) frog_output_interval = atof(argv[4]);

            /*
             * Ignore this argument, because biologist's enumeration
             * scheme is hard coded for 16 cells.
             */
            // if(argc > 5) num_cells = atoi(argv[5]);

            if(argc > 6) year_length = atof(argv[6]);
            if(argc > 7) years_to_model = atoi(argv[7]);

            simulation->initialize(
                &director,

                initial_frog_count,
                infected_frog_count,
                max_frog_count,
                frog_output_interval,

                num_cells,

                year_length,
                years_to_model
            );
        }

        director.run();

    } MPI_Finalize();

    return EXIT_SUCCESS;
}
