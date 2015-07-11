ctry
====

A C library that provides `try`/`catch`/`finally` syntax.

Features
--------

* Uses `setjmp()`.
* Supports `pthreads`.
* Does not use `malloc()` (except for `pthread_getspecific()`).
* Well-tested.

Caveats
-------

* Probably not compatble with C++ exceptions.
* Local flow-control, e.g. `break`, `continue`, `return`, `goto` in or out of `crty_BEGIN`/`END` blocks may corrupt `ctry` dynamic  state.

Examples
--------

```C
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "ctry.h"

static FILE* open_file(const char *path, const char *mode)
{
  FILE *f;
  if ( ! (f = fopen(path, mode)) ) {
    ctry_raise(errno, 2, path, mode);
  }
  return f;
}

static int do_it()
{
  ctry_BEGIN {
    ctry_BODY {
      FILE *f = open_file("non-existent.txt", "r");
      char buf[1024] = { 0 };
      fread(buf, sizeof(buf[0]), sizeof(buf) - 1, f);
    }
    ctry_CATCH_ANY {
      ctry_exc_t *exc = ctry_exc();
      fprintf(stderr, "Error: %s (%d): %s(\"%s\", \"%s\")\n",
              strerror(exc->code), exc->code, exc->cntx.func, exc->data[0], exc->data[1]);
      ctry_RETURN(1);
    }
  } ctry_END;
  
  return 0;
}

int main(int argc, char **argv)
{
  assert(do_it() == 1);
  return 0;
}
```
