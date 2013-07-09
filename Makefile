CPP=mpicxx

TESTS=actor_test

.PHONY: all
all: check

TESTS=actor_test

%.o: %.cc
	$(CPP) -c -o $@ $<

%_test: %_test.cc
	$(CPP) -o $@ $^


.PHONY: check
check: $(TESTS)
	for test in $(TESTS); do mpiexec -n 2 ./$$test; done;

.PHONY: clean
clean:
	-rm $(TESTS)
