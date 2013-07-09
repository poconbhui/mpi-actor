Blue Nose Disease Simulation
----------------------------

# Build the simulation:
make simulation

# Run the simulation (eg on 8 cores)
mpiexec -n 8 ./simulation

# Default parameters are as follows
initial_frog_count = 34;
infected_frog_count = 1;
max_frog_count = 100;
frog_output_interval = 0.1; // in seconds

num_cells = 16;

year_length = 0.5; // in seconds
years_to_model = 100;


# Arguments to simulation are in the same order
mpiexec -n 8 ./simulation (initial_frog_count, (infected_frog_count, (max_frog_count, (frog_output_interval, (num_cells, (year_length, years_to_model))))))


# Submit with the example submission script
qsub example_submission.sh
