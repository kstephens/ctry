CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Dctry_PTHREAD
CFLAGS += -I.
CFLAGS += -pthread

default: test

TESTS_C:=$(shell ls t/*.t.c)
TESTS:=$(TESTS_C:.c=)

p:
	@echo '$(v)=$($(v))'

test: $(TESTS)
	@set -xe; for t in $(TESTS); do $$t; done

debug: t/t1.t
	lldb t/t1.t

ctry.o : ctry.c ctry.h

clean:
	rm -f *.o *.i t/*.t
