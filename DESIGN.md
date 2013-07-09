- Actor model built on top of an event queue.
- Actor communications built on top of MPI message queue.

- A director on each process manages the event queue for that process.
- When an actor is born, the director adds them to the queue.
- A director pops the top actor off the queue and runs it.
- If that actor is still alive after that run, the director adds
  it back to the end of the queue.
  If the actor is dead after the run, it is freed etc and
  not added back to the queue.
- Directors must be capable of both executing a fixed number of
  actors and executing until all of the actors have died.

- Actor creation and deletion will be handled by a factory class.
- The factory class will map actor initialization functions to
  integers, so they can be referenced over MPI.
- When an actor class is defined, it must be registered with the
  factory class.
- All actors must be registered on all participating processes in the
  same order.

- A distributed factory class, inheriting from the factory class,
  will handle requests to create new actors.
- An actor will request that an actor be born.
  The distributed factory class will then generate a unique id and determine
  which process the actor will be born on.
  It will then send a message to that process requesting a birth and
  return the id and rank to the requesting actor.
- The director class will periodically ask the distributed factory class
  if it has any actors waiting to be created. At which point, the
  director will request they be created and add them to the event queue.
- Load balancing will be implemented by round robin actor births.
  This opens plenty of potential for load imbalance.
  A better system would be a weighted round robin approach.

- An actor will consist of 3 phases: initialization, running and destruction.
- Initialization will be done through the class constructor.
  In this stage, an actor will be unable to send any messages or request
  that any other actors be born.
- Destruction will be done through the class destructor.
  Similarly, actors should be unable to send any messages or request
  that any other actors be born.
- The running stage will be implemented as an execution of a main() method
  on the actor.
  A newly defined actor will override this method and place whatever logic
  is required in here.
  When the main function is executed, the actor will have access to a
  messaging system.
- The messaging system must be capable of sending arbitrary data and a
  message tag as defined by the user.
  Messages sent will be identified by actors using their MPI tag.
  That is, an actor with id 5 will request messages with a tag 5.
  As such, to send tags along with arbitrary data, a double barrelled
  message will be sent: the first containing metadata and the second
  containing actual data.
