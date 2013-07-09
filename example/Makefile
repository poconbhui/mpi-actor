CPP = mpicxx

frog_deps = provided-functions/frog-functions.c provided-functions/ran2.c

simulation: $(frog_deps) main.cc
	$(CPP) -o $@ $^

TESTS = cell_test frog_test

frog_test: $(frog_deps)

%.o: %.cc
	$(CPP) -c -o $@ $<

%_test: %_test.cc
	$(CPP) -o $@ $^


.PHONY: check
check: $(TESTS)
	for test in $(TESTS); do mpiexec -n 2 ./$$test; done;

.PHONY: clean
clean:
	-rm $(TESTS) simulation *.o
