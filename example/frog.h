#ifndef FROG_H_
#define FROG_H_

#include "../src/actor.h"
#include "./provided-functions/frog-functions.h"

#include "./circular_buffer.h"

#include "./grid.h"
#include "./cell.h"

long RNG_state;

class Frog: public ActorModel::Actor {
public:
    Frog():
        _is_infected(false),

        _totalPopulationInflux(0),
        _infectionLevels(),

        _coords(),

        _register_actor(-1, -1),

        _total_hops(0),
        _main_state(0)
    {}

    // Hard coded values in the model
    static const int infectionLevel_history_length = 500;
    static const int test_death_hop_count = 700;
    static const int test_birth_hop_count = 300;

    // Special message data types
    struct Coords {
        float x;
        float y;
    };
    
    // Frog message tags
    enum {
        /**
         * Message tag: POPULATION_DATA
         * Message data Cell::PopulationData
         *
         * A frog receiving this message will update its historical
         * values according to the PopulationData it has received.
         */
        POPULATION_DATA,

        /**
         * Message tag: CELL_LIST
         * Message data: Id[]
         *
         * A frog receiving this message type will set the current
         * grid it moves about to the received grid.
         *
         * A frog must receive at least one of these messages before
         * it is considered initialized and begins moving.
         */
        CELL_LIST,

        /**
         * Message tag: INITIAL_COORDS
         * Message data: Coords
         *
         * A frog receiving this message will set its current coordinates
         * to the value received.
         *
         * A frog must receive at least one of these messages before
         * it is considered initialized and begins moving.
         */
        INITIAL_COORDS,

        /**
         * Message tag: REGISTER_ACTOR
         * Message data Id
         *
         * A frog receiving this message will set the actor it will
         * immediately send a message to the received Id of type
         * bool (true) with message tag REGISTER_ACTOR to indicate that
         * it is alive, and will * message this actor again with type
         * bool (false) and the REGISTER_ACTOR tag when it dies.
         *
         * A frog must receive at least one of these messages before
         * it is considered initialized and begins moving.
         */
        REGISTER_ACTOR,

        /**
         * Message tag: INFECTION_STATUS
         * Message data: bool
         *
         * A frog receiving this message will set its infection status
         * to infected (true) or uninfacted (false) according to the
         * value it receives.
         */
        INFECTION_STATUS,

        /**
         * Message tag: DIE
         * Message data: bool
         *
         * A frog receiving this message will die. The message data
         * will be ignored.
         */
        DIE
    };


    /**
     * _main_state <  INITIALIZED
     *     means the frog is still awaiting some initialization data.
     *
     * _main_state == READY_TO_HOP
     *     means frog has been fully initialized and is ready to hop.
     *
     * _main_state == AWAITING_CELL_DATA
     *     means frog is awaiting population data from cell.
     */
    void main(void) {
        Message message;

        while(get_message(&message)) {
            switch(message.tag()) {

                /*
                 * Initialization messages
                 */
                case CELL_LIST: {
                    int cell_list_size = message.data_size<ActorModel::Id>();

                    _cell_list.resize(cell_list_size);
                    message.data<ActorModel::Id>(
                        &_cell_list[0], cell_list_size
                    );

                    init();
                } break;

                case INITIAL_COORDS: {
                    _coords = message.data<Coords>();

                    init();
                } break;

                case REGISTER_ACTOR: {
                    _register_actor = message.data<ActorModel::Id>();

                    init();
                } break;

                /*
                 * Optional initialization message
                 */
                case INFECTION_STATUS: {
                    _is_infected = message.data<bool>();
                } break;


                /*
                 * Update own data according to cell data
                 */
                case POPULATION_DATA: {
                    Cell::PopulationData data =
                        message.data<Cell::PopulationData>();

                    _totalPopulationInflux += data.populationInflux;

                    _infectionLevels.push(data.infectionLevel);

                    _main_state = READY_TO_HOP;
                } break;

                /* Die! */
                case DIE: {
                    die();
                }

            }
        }

        // Do the regular tasks if initialized
        if(_main_state == READY_TO_HOP) {
            hop();

            test_birth();
            test_disease();
            test_death();

            if(!is_dead()) {
                request_cell_data();
                _main_state = AWAITING_CELL_DATA;
            }
        }
    }


    /**
     * An actor can give birth to a frog and fully initialize
     * it through this function.
     */
    static ActorModel::Id give_birth_and_initialize(
        Actor* parent,
        ActorModel::Id *cell_list, int cell_list_count,
        Coords& coords,
        ActorModel::Id& register_actor
    ) {
        ActorModel::Id child_id = parent->give_birth<Frog>();

        // Initialize child
        parent->send_message<ActorModel::Id>(
            child_id, cell_list, cell_list_count, CELL_LIST
        );
        parent->send_message<Frog::Coords>(
            child_id, coords, INITIAL_COORDS
        );
        parent->send_message<ActorModel::Id>(
            child_id, register_actor, REGISTER_ACTOR
        );

        return child_id;
    }


    /*
     * Accessors
     */
    bool is_infected(void) {
        return _is_infected;
    }

    ActorModel::Id cell_list(size_t i) {
        return _cell_list[i];
    }

    Coords coords(void) {
        return _coords;
    }

    int totalPopulationInflux(void) {
        return _totalPopulationInflux;
    }

    CircularBuffer<int, infectionLevel_history_length> infectionLevels(void) {
        return _infectionLevels;
    }


private:
    /* Update initialization state of frog. */
    void init(void) {
        if(_main_state <  INITIALIZED) _main_state++;
        if(_main_state == INITIALIZED) {
            // Request cell data
            request_cell_data();
            _main_state = AWAITING_CELL_DATA;

            // Inform register of our birth
            bool is_alive = true;
            send_message<bool>(_register_actor, is_alive, REGISTER_ACTOR);
        }
    }

    /* kill frog */
    void die(void) {
        // Die as usual
        ActorModel::Actor::die();

        // Inform register of our death
        bool is_alive = false;
        send_message<bool>(_register_actor, is_alive, REGISTER_ACTOR);
    }

    /*
     * Send a request to the cell we're currently on
     * for its population data.
     */
    void request_cell_data(void) {
        int cell_num = getCellFromPosition(_coords.x, _coords.y);

        Cell::PopulationDataRequest data;
        data.tag = POPULATION_DATA;
        data.reply = get_id();

        send_message<Cell::PopulationDataRequest>(
            _cell_list[cell_num], data, Cell::POPULATION_DATA
        );
    }

    /*
     * Move around the environment.
     */
    void hop(void) {
        Coords old_coords = _coords;
        frogHop(
            old_coords.x, old_coords.y,
            &_coords.x, &_coords.y,
            &RNG_state
        );

        int cell_num = getCellFromPosition(_coords.x, _coords.y);

        send_message<bool>(
            _cell_list[cell_num], _is_infected, Cell::LANDED
        );

        _total_hops++;
    }

    /*
     * Test to see if this frog will give birth to another.
     * If so, give birth.
     */
    void test_birth(void) {
        if(_total_hops % test_birth_hop_count == 0) {
            float averagePopulationInflux =
                static_cast<float>(_totalPopulationInflux)/test_birth_hop_count;

            if(willGiveBirth(averagePopulationInflux, &RNG_state)) {
                give_birth_and_initialize(
                    this,
                    &_cell_list[0], _cell_list.size(),
                    _coords, _register_actor
                );
            }

            // Reset counter
            _totalPopulationInflux = 0;
        }
    }

    /*
     * Test to see if the Frog should catch the disease.
     * If so, set the frog to infected.
     */
    void test_disease(void) {
        float average = 0.0;
        for(int i=0; i<infectionLevel_history_length; i++) {
            average += _infectionLevels[i];
        }
        average = average/infectionLevel_history_length;

        if(willCatchDisease(average, &RNG_state)) {
            _is_infected = true;
        }
    }

    void test_death(void) {
        if(_is_infected && (_total_hops % test_death_hop_count) == 0) {
            if(willDie(&RNG_state)) {
                die();
            }
        }
    }

    // The infection status of the frog
    bool _is_infected;

    // Track the important data influencing birth and death
    // according to the model
    int _totalPopulationInflux;
    CircularBuffer<int, infectionLevel_history_length> _infectionLevels;

    // The current coordinates of the frog
    Coords _coords;

    // The grid the frog lives on
    std::vector<ActorModel::Id> _cell_list;

    // The actor the frog must notify about birth and death
    ActorModel::Id _register_actor;

    // Track the total number of hops a frog has made
    int _total_hops;

    // Track the current state of the main loop
    int _main_state;
    enum {
        INITIALIZED=3,
        READY_TO_HOP=4,
        AWAITING_CELL_DATA=5
    };
};


#endif  // FROG_H_
