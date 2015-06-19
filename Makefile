CFLAGS += -g

default: test-try.t.i test

test: test-try.t
	./test-try.t

debug: test-try.t
	lldb ./test-try.t

try.o : try.c try.h

test-try.t: test-try.t.c try.o
	$(CC) $(CFLAGS) test-try.t.c -o $@ try.o

test-try.t.i: test-try.t.c try.h
	$(CC) $(CFLAGS) -E test-try.t.c -o $@

clean:
	rm -f *.o *.i *.t
