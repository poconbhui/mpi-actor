#include "./cell.h"

#include "../test/super_quick_test.h"
#include "../src/actor.h"
#include "../src/director.h"

using namespace ActorModel;

class TestPopulationCount: public Actor {
public:
    void main(void) {}

    void land_on_cell(bool is_sick) {
        send_message<bool>(cell_id, is_sick, Cell::LANDED);
    }

    bool receive_population_data(int tag) {
        Message message;
        if(get_message(&message)) {
            REQUIRE(message.tag() == tag);

            cell_population = message.data<Cell::PopulationData>();

            return true;
        }

        return false;
    }

    void request_population_data(int tag) {
        Cell::PopulationDataRequest data;
        data.tag = tag;
        data.reply = get_id();

        send_message<Cell::PopulationDataRequest>(
            cell_id, data, Cell::POPULATION_DATA
        );
    }

    void set_population_data(int pop, int inf) {
        Cell::PopulationData data = {pop, inf};
        send_message<Cell::PopulationData>(
            cell_id, data, Cell::SET_POPULATION_DATA
        );
    }

    Id cell_id;

    Cell::PopulationData cell_population;
};

void test_population_count(void) {
    Director director;

    if(director.is_root()) {
        TestPopulationCount *test_registration =
            director.add_actor<TestPopulationCount>();
        Cell *cell = director.add_actor<Cell>();

        test_registration->cell_id = cell->get_id();

        bool is_sick;


        // Land on Cell and update cell
        is_sick = false;
        test_registration->land_on_cell(is_sick);
        cell->main();

        REQUIRE(cell->populationInflux() == 1);
        REQUIRE(cell->infectionLevel() == 0);


        // Make it sick and land again
        is_sick = true;
        test_registration->land_on_cell(is_sick);
        cell->main();

        REQUIRE(cell->populationInflux() == 2);
        REQUIRE(cell->infectionLevel() == 1);


        // Request population data

        // check range of tags
        for(int tag=0; tag<3; tag++) {
            test_registration->cell_population.populationInflux = 0;
            test_registration->cell_population.infectionLevel = 0;

            test_registration->request_population_data(tag);
            cell->main();
            test_registration->receive_population_data(tag);

            REQUIRE(test_registration->cell_population.populationInflux == 2);
            REQUIRE(test_registration->cell_population.infectionLevel == 1);
        }


        // Request Cell sets its population data
        test_registration->set_population_data(5,6);
        cell->main();

        REQUIRE(cell->populationInflux() == 5);
        REQUIRE(cell->infectionLevel() == 6);
    }
}


class TestPoisonPill: public Actor {
public:
    void main(void){}

    void kill_cell(Id cell_id) {
        bool data = true;
        send_message<bool>(cell_id, data, Cell::DIE);
    }
};

void test_poison_pill(void) {
    Director director;

    if(director.is_root()) {
        TestPoisonPill *test_pp = director.add_actor<TestPoisonPill>();
        Cell *cell = director.add_actor<Cell>();

        REQUIRE(!cell->is_dead());

        test_pp->kill_cell(cell->get_id());
        cell->main();

        REQUIRE(cell->is_dead());
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    void* buffer = ::operator new(1000);
    MPI_Buffer_attach(buffer, 1000);


    INIT_SQT();

    RUN_TEST(test_population_count);

    RUN_TEST(test_poison_pill);

    MPI_Finalize();
}
