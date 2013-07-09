#include "./frog.h"

#include "../test/super_quick_test.h"
#include "../src/actor.h"
#include "../src/director.h"

#include "./grid.h"
#include "./cell.h"


using namespace ActorModel;


class TestFrogSetup: public Actor {
public:
    void main(void){}

    void send_grid(Id *cell_list, int cell_count, Id frog_id) {
        send_message<Id>(frog_id, cell_list, cell_count, Frog::CELL_LIST);
    }

    void send_starting_position(float x, float y, Id frog_id) {
        Frog::Coords coords = {x, y};
        send_message<Frog::Coords>(frog_id, coords, Frog::INITIAL_COORDS);
    }

    void send_register_actor(Id frog_id) {
        Id my_id = get_id();
        send_message<Id>(frog_id, my_id, Frog::REGISTER_ACTOR);
    }

    int receive_registration() {
        Message message;
        if(get_message(&message)) {
            REQUIRE(message.tag() == Frog::REGISTER_ACTOR);

            if(message.data<bool>()) return 1;
            else                     return 0;
        }

        // No message received!
        return -1;
    }

    void send_infection_status(bool is_infected, Id frog_id) {
        send_message<bool>(frog_id, is_infected, Frog::INFECTION_STATUS);
    }

    void kill_frog(Id frog_id) {
        bool ignore=true;
        send_message<bool>(frog_id, ignore, Frog::DIE);
    }

    void set_cell_population(int pop, int inf, Id cell_id) {
        Cell::PopulationData data = {pop, inf};
        send_message<Cell::PopulationData>(
            cell_id, data, Cell::SET_POPULATION_DATA
        );
    }
        
};

void test_frog_setup(void) {
    Director director;

    if(director.is_root()) {
        TestFrogSetup *test =
            director.add_actor<TestFrogSetup>();
        Frog *frog = director.add_actor<Frog>();

        Grid grid;

        Cell *cells[grid.num_cells];

        for(int i=0; i<grid.num_cells; i++) {
            cells[i] = director.add_actor<Cell>();
        }

        for(int i=0; i<grid.num_cells; i++) {
            grid.cell_ids[i] = cells[i]->get_id();
        }

        // Send frog tracker id
        test->send_register_actor(frog->get_id());

        // Send cell data to frog and receive it
        test->send_grid(grid.cell_ids, grid.num_cells, frog->get_id());
        frog->main();

        for(int i=0; i<grid.num_cells; i++) {
            REQUIRE(frog->cell_list(i).rank == cells[i]->get_id().rank);
            REQUIRE(frog->cell_list(i).id   == cells[i]->get_id().id);
        }

        // Require that frog has done nothing until initial
        // position has been set
        for(int i=0; i<grid.num_cells; i++) {
            // receive potential hop
            cells[i]->main();
            REQUIRE(cells[i]->populationInflux() == 0);
            REQUIRE(cells[i]->infectionLevel() == 0);
        }

        // Set initial position and return cell data
        float coord_x = 0.1;
        float coord_y = 0.2;
        test->send_starting_position(coord_x, coord_y, frog->get_id());
        frog->main();

        // Frog should wait until cells have replied with population data
        REQUIRE(sqt_fleq(frog->coords().x, coord_x));
        REQUIRE(sqt_fleq(frog->coords().y, coord_y));
        for(int i=0; i<grid.num_cells; i++) {
            cells[i]->main();
        }

        // Frog should hop now
        frog->main();

        // Check that population influx has now increased
        int totalPopulationInflux = 0;
        int totalInfectionLevel = 0;
        for(int i=0; i<grid.num_cells; i++) {
            cells[i]->main();
            totalPopulationInflux += cells[i]->populationInflux();
            totalInfectionLevel   += cells[i]->infectionLevel();
        }
        REQUIRE(totalPopulationInflux == 1);
        REQUIRE(totalInfectionLevel == 0);
    }
}


void test_frog_cell_interaction(void) {
    Director director;

    if(director.is_root()) {
        TestFrogSetup *test =
            director.add_actor<TestFrogSetup>();
        Frog *frog = director.add_actor<Frog>();

        Grid grid;

        Cell *cells[grid.num_cells];
        for(int i=0; i<grid.num_cells; i++) {
            cells[i] = director.add_actor<Cell>();
        }

        for(int i=0; i<grid.num_cells; i++) {
            grid.cell_ids[i] = cells[i]->get_id();
        }

        // Send cell data to frog and receive it
        test->send_grid(grid.cell_ids, grid.num_cells, frog->get_id());
        test->send_starting_position(0.0, 0.0, frog->get_id());
        test->send_register_actor(frog->get_id());
        frog->main();

        // Update cells after frog hop
        for(int i=0; i<grid.num_cells; i++) {
            cells[i]->main();
        }


        // Test healthy Frog affects cell populations appropriately
        {
            int initialPopulationInflux = 0;
            int initialInfectionLevel   = 0;
            for(int i=0; i<grid.num_cells; i++) {
                initialPopulationInflux += cells[i]->populationInflux();
                initialInfectionLevel   += cells[i]->infectionLevel();
            }

            frog->main();

            int finalPopulationInflux = 0;
            int finalInfectionLevel   = 0;
            for(int i=0; i<grid.num_cells; i++) {
                cells[i]->main();

                finalPopulationInflux += cells[i]->populationInflux();
                finalInfectionLevel   += cells[i]->infectionLevel();
            }

            REQUIRE(finalPopulationInflux - initialPopulationInflux == 1);
            REQUIRE(finalInfectionLevel - initialInfectionLevel == 0);
        }


        // Test sick Frog affects cell populations appropriately
        {
            int initialPopulationInflux = 0;
            int initialInfectionLevel   = 0;
            for(int i=0; i<grid.num_cells; i++) {
                initialPopulationInflux += cells[i]->populationInflux();
                initialInfectionLevel   += cells[i]->infectionLevel();
            }

            test->send_infection_status(true, frog->get_id());
            frog->main();

            int finalPopulationInflux = 0;
            int finalInfectionLevel   = 0;
            for(int i=0; i<grid.num_cells; i++) {
                cells[i]->main();

                finalPopulationInflux += cells[i]->populationInflux();
                finalInfectionLevel   += cells[i]->infectionLevel();
            }

            REQUIRE(finalPopulationInflux - initialPopulationInflux == 1);
            REQUIRE(finalInfectionLevel - initialInfectionLevel == 1);
        }

    }
}


void test_frog_cell_history(void) {
    Director director;

    if(director.is_root()) {
        TestFrogSetup *test =
            director.add_actor<TestFrogSetup>();
        Frog *frog = director.add_actor<Frog>();

        Grid grid;
        Cell *test_cell = director.add_actor<Cell>();

        for(int i=0; i<grid.num_cells; i++) {
            grid.cell_ids[i] = test_cell->get_id();
        }

        // Send cell data to frog
        test->send_grid(grid.cell_ids, grid.num_cells, frog->get_id());
        test->send_starting_position(0.0, 0.0, frog->get_id());

        int totalPopulationInflux = 0;
        for(int i=0; i<Frog::test_birth_hop_count; i++) {
            // (possibly) receive cell population data and hop
            frog->main();

            // This is the operation we expect to match in Frog
            totalPopulationInflux += test_cell->populationInflux();

            // Receive hop from frog and send population data
            test_cell->main();
        }

        // Frog has only received pop_his_len-1 data points.
        REQUIRE(frog->totalPopulationInflux() == totalPopulationInflux);

        // Receiving 1 more should reset the counter
        frog->main();
        REQUIRE(frog->totalPopulationInflux() == 0);


        int infection_history[Frog::infectionLevel_history_length];
        for(int i=0; i<Frog::infectionLevel_history_length; i++) {
            infection_history[i] = 0;
            REQUIRE(frog->infectionLevels()[i] == 0);
        }

        // soak up all messages to test_cell
        test_cell->main();

        for(int i=0; i<2*Frog::infectionLevel_history_length; i++) {
            // Make some obvious pattern to track
            if (i<10 || i%2 == 0) {
                test->send_infection_status(true, frog->get_id());
            } else {
                test->send_infection_status(true, frog->get_id());
            }

            frog->main();
            test_cell->main();

            // Only start tracking long after a regular array would
            // have given up
            if(i >= Frog::infectionLevel_history_length) {
                infection_history[i-Frog::infectionLevel_history_length] =
                    test_cell->infectionLevel();
            }
        }
        frog->main();

        for(int i=0; i<Frog::infectionLevel_history_length; i++) {
            REQUIRE(
                frog->infectionLevels()[i] ==
                    infection_history[Frog::infectionLevel_history_length-1 - i]

            );
        }

    }
}


void test_sick_frogs(void) {
    Director director;

    director.register_actor<Frog>();

    if(director.is_root()) {
        TestFrogSetup *test =
            director.add_actor<TestFrogSetup>();
        Frog *frog = director.add_actor<Frog>();

        Grid grid;
        Cell *test_cell = director.add_actor<Cell>();

        for(int i=0; i<grid.num_cells; i++) {
            grid.cell_ids[i] = test_cell->get_id();
        }

        // Send cell data to frogs
        test->send_grid(grid.cell_ids, grid.num_cells, frog->get_id());
        test->send_starting_position(0.0, 0.0, frog->get_id());
        test->send_register_actor(frog->get_id());


        // Run for far longer than the frog is expected to live
        // after getting infected
        // (which should be ~6*700)
        for(int i=0; i<50*Frog::test_death_hop_count; i++) {
            frog->main();
            test_cell->main();
        }

        // Expect a live and uninfected frog
        REQUIRE(!frog->is_infected());
        REQUIRE(!frog->is_dead());

        // Set frog history to a highly infected state
        test->set_cell_population(0, 500000, test_cell->get_id());
        test_cell->main();
        REQUIRE(test_cell->infectionLevel() == 500000);
        test->send_infection_status(false, frog->get_id());
        for(int i=0; i<Frog::infectionLevel_history_length; i++) {
            frog->main();
        }

        // Run for a time where we expect the frog to become infected,
        // but before we expect him to die
        for(int i=0; i<0.5*Frog::test_death_hop_count; i++) {
            frog->main();
            test_cell->main();
        }

        // Expect a sick frog
        REQUIRE(frog->is_infected());
        REQUIRE(!frog->is_dead());

        // Run again with the infected frog until it is dead
        for(int i=0; i<50*Frog::test_death_hop_count; i++) {
            frog->main();
            test_cell->main();

            if(frog->is_dead()) break;
        }

        // Expect an ex-frog
        REQUIRE(frog->is_dead());
    }
}


void test_frog_birth(void) {
    Director director;

    director.register_actor<Frog>();

    Cell *test_cell = NULL;
    Frog *frog = NULL;

    TestFrogSetup *test;

    if(director.is_root()) {
        test = director.add_actor<TestFrogSetup>();
        frog = director.add_actor<Frog>();

        test_cell = director.add_actor<Cell>();

        Grid grid;
        for(int i=0; i<grid.num_cells; i++) {
            grid.cell_ids[i] = test_cell->get_id();
        }

        // Send cell data to frogs
        test->send_grid(grid.cell_ids, grid.num_cells, frog->get_id());
        test->send_starting_position(0.0, 0.0, frog->get_id());
        test->send_register_actor(frog->get_id());

        // Expect frog to register its birth
        frog->main();
        REQUIRE(test->receive_registration() == 1);

        // Set population influx to the value of (almost) maximum probability
        test->set_cell_population(2000, 0, test_cell->get_id());
        test_cell->main();
        REQUIRE(test_cell->populationInflux() == 2000);
    }


    // Get current load
    int load = director.get_load();

    // Find sum of loads across processes
    int initial_global_load = 0;
    MPI_Allreduce(
        &load, &initial_global_load, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD
    );

    // Run for a time where it is very likely the frog has given birth
    director.run(50*Frog::test_birth_hop_count);

    // Get current load
    load = director.get_load();

    // Find sum of loads across processes
    int global_load=0;
    MPI_Allreduce(&load, &global_load, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    // We expect the total load to have increased from frog births
    REQUIRE(global_load > initial_global_load);


    // Kill the frog and test deregistration
    if(director.is_root()) {
        // Soak up all extra registrations
        while(test->receive_registration() != -1);

        // Send kill signal to frog
        test->kill_frog(frog->get_id());
        frog->main();

        // Expect a death registration
        REQUIRE(test->receive_registration() == 0);
    }

    // Give the director time to dump the dead frog
    director.run(50);

    // Expect cell population influx to continue increasing
    // after base frog has been killed due to children
    int initialPopulationInflux;
    if(director.is_root()) {
        initialPopulationInflux = test_cell->populationInflux();
    }

    // Run until we're sure at least one frog has hopped
    director.run(100);

    if(director.is_root()) {
        int finalPopulationInflux = test_cell->populationInflux();

        REQUIRE(finalPopulationInflux > initialPopulationInflux);
    }

}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    void* buffer = ::operator new(100000);
    MPI_Buffer_attach(buffer, 100000);


    INIT_SQT();

    RUN_TEST(test_frog_setup);
    RUN_TEST(test_frog_cell_interaction);
    RUN_TEST(test_frog_cell_history);
    RUN_TEST(test_sick_frogs);
    RUN_TEST(test_frog_birth);

    MPI_Finalize();
}
