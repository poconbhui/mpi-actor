#ifndef SIMULATION_H_
#define SIMULATION_H_

#include <sys/time.h>
#include <vector>

#include "../src/actor.h"
#include "./cell.h"
#include "./frog.h"

/**
 * Simulation managing actor.
 *
 * This actor is used to add the initial actors to the
 * simulation and to initialize them.
 */
class Simulation: public ActorModel::Actor {
public:
    Simulation(): _director(NULL) {}

    void main(void) {
        // Receive and output cell data
        Message message;
        while(get_message(&message)) {
            switch(message.tag()) {

                // Receive and output requested cell data.
                case Cell::POPULATION_DATA: {
                    Cell::PopulationData population_data =
                        message.data<Cell::PopulationData>();

                    ActorModel::Id cell_id = message.sender();
                    int cellnum=-1;
                    // Find which cell's data we just received
                    for(int i=0; i<_cell_list_size; i++) {
                        ActorModel::Id test = _cell_list[i];

                        if(
                            test.rank() == cell_id.rank() &&
                            test.gid() == cell_id.gid()
                        ) {
                            cellnum = i;

                            break;
                        }
                    }

                    // Output the data to the screen
                    std::cout << "DATA: ("
                              << cellnum << ","
                              << population_data.populationInflux << ","
                              << population_data.infectionLevel << ")"
                              << std::endl;
                } break;

                // Track the current number of frogs in the system
                case Frog::REGISTER_ACTOR: {
                    bool is_alive = message.data<bool>();

                    if(is_alive) _frog_count++;
                    else         _frog_count--;
                } break;

            }
        }

        // Find current time
        double now = second();

        // Perform yearly actions
        if(now > _year_end) {
            // Increment current year
            _current_year++;

            // Reset year timer
            _year_end = now + _year_length;

            if(_current_year <= _years_to_model) {
                // Add a little padding before output
                std::cout << std::endl;

                // Output current year
                std::cout << "YEAR: " << _current_year << std::endl;

                // Output frog population
                std::cout << "FROG POPULATION: " << _frog_count << std::endl;

                if(_frog_count > _max_frog_count) {
                    std::cout
                        << "ERROR: Frog count exceeded "
                        << _max_frog_count
                        << "!"
                        << std::endl;

                    _director->end();

                    // return early without requesting cell data
                    return;
                }

                // Request Cell data to (hopefully) output on next tick
                for(int i=0; i<_cell_list_size; i++) {
                    // The following assumes messages arrive in order

                    // Request cell data
                    Cell::PopulationDataRequest request;
                    request.tag = Cell::POPULATION_DATA;
                    request.reply = id();
                    send_message<Cell::PopulationDataRequest>(
                        _cell_list[i], request, Cell::POPULATION_DATA
                    );

                    // Clean cell in monsoon
                    Cell::PopulationData data = {0, 0};
                    send_message<Cell::PopulationData>(
                        _cell_list[i], data, Cell::SET_POPULATION_DATA
                    );
                }
            } else {
                // The simulation time is up. Kill the director.
                _director->end();
            }

        }

        // Output frogs at requested frequency
        if(now > _next_frog_output) {
            _next_frog_output = now + _frog_output_interval;

            // Output frog population
            std::cout << "FROG POPULATION: " << _frog_count << std::endl;
        }


    }


    /**
     * Call this function once to initialize the simulation
     */
    void initialize(
        ActorModel::Director *director,

        int initial_frog_count=34,
        int initial_infected_frog_count=0,
        int max_frog_count=100,
        double frog_output_interval=0.5,

        int cell_list_size=16,

        float year_length=2.0,
        int years_to_model=100
    ) {
        _director = director;

        _initial_frog_count = initial_frog_count;
        _initial_infected_frog_count = initial_infected_frog_count;
        _max_frog_count = max_frog_count;
        _frog_output_interval = frog_output_interval;

        _cell_list_size = cell_list_size;

        _year_length = year_length;
        _years_to_model = years_to_model;

        _frog_count = 0;
        _current_year = 0;
        _year_end = 0.0;
        _next_frog_output = 0.0;


        // Generate grid of cells
        _cell_list.resize(_cell_list_size);
        for(int i=0; i<_cell_list_size; i++) {
            _cell_list[i] = give_birth<Cell>();
        }


        // Generate and initialize frogs
        for(int i=0; i<_initial_frog_count; i++) {
            Frog::Coords coords = {0.0, 0.0};
            ActorModel::Id my_id = id();

            ActorModel::Id frog_id = Frog::give_birth_and_initialize(
                this, &_cell_list[0], _cell_list_size, coords, my_id
            );


            // Infect some frogs, as requested
            if(i < _initial_infected_frog_count) {
                bool infected=true;
                send_message<bool>(
                    frog_id, infected, Frog::INFECTION_STATUS
                );
            }
        }
    }

    double second(void) {
        struct timeval tp;
        struct timezone tzp;

        gettimeofday(&tp, &tzp);

        return ( ((double) tp.tv_sec) + ((double) tp.tv_usec) * 1.0e-6 );
    }


private:

    int _initial_frog_count;
    int _initial_infected_frog_count;
    int _max_frog_count;
    double _frog_output_interval;

    double _year_length;  // Length of a year in seconds
    int _years_to_model;

    ActorModel::Director *_director;

    int _current_year;
    double _year_end;
    double _next_frog_output;

    std::vector<ActorModel::Id> _cell_list;
    int _cell_list_size;

    int _frog_count;
};

#endif  // SIMULATION_H_
