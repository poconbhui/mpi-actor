#ifndef CELL_H_
#define CELL_H_

#include "../src/actor.h"


class Cell: public ActorModel::Actor {
public:
    Cell(): _populationInflux(0), _infectionLevel(0) {}

    // Special message data types
    struct PopulationData {
        int populationInflux;
        int infectionLevel;
    };
    struct PopulationDataRequest {
        int tag;
        ActorModel::Id reply;
    };

    // Cell message tags
    enum {
        /**
         * Message tag: LANDED
         * Message data: bool
         *
         * Inform the cell it has been landed upon.
         * The boolean value signifies whether the actor
         * landing is infected (true) or not (false)
         */
        LANDED,

        /**
         * Message tag: POPULATION_DATA
         * Message data: PopulationDataRequest
         * Message reply: PopulationData
         *
         * Request the current population data from the cell.
         * The PopulationDataRequest struct should be filled
         * and sent to the cell, informing it of what Actor Id
         * to reply to and what tag to reply with.
         * The cell replies with the PopulationData type.
         */
        POPULATION_DATA,

        /**
         * Message tag: SET_POPULATION_DATA
         * Message type: PopulationData
         *
         * When the cell receives this message, it will set its
         * current populationInflux and infection_level values
         * to whatever values it received in the message.
         *
         * This can be used to initialize the values of the cell
         * or to set them to 0 at any time.
         */
        SET_POPULATION_DATA,

        /**
         * Message tag: DIE
         * Message type: bool
         *
         * When a cell receives this message, it will die.
         * The message passed is ignored.
         */
        DIE
    };

    void main(void) {
        Message message;

        while(get_message(&message)) {
            switch(message.tag()) {

                case LANDED: {
                    bool is_infected = message.data<bool>();

                    _populationInflux++;
                    if(is_infected) _infectionLevel++;
                } break;

                case POPULATION_DATA: {
                    PopulationDataRequest request =
                        message.data<PopulationDataRequest>();

                    PopulationData data;
                    data.populationInflux = _populationInflux;
                    data.infectionLevel   = _infectionLevel;

                    send_message<PopulationData>(
                        request.reply, data, request.tag
                    );
                } break;

                case SET_POPULATION_DATA: {
                    PopulationData data = message.data<PopulationData>();

                    _populationInflux = data.populationInflux;
                    _infectionLevel   = data.infectionLevel;
                } break;

                case DIE: {
                    die();
                } break;

            }
        }
    }

    /*
     * Accessors
     */
    int populationInflux(void) {
        return _populationInflux;
    }

    int infectionLevel(void) {
        return _infectionLevel;
    }

private:
    // The total number of actors that have landed on the cell
    int _populationInflux;

    // The total number of infected actors that have landed on the cell
    int _infectionLevel;
};


#endif  // CELL_H_
