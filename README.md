ctry
====

A C library that provides `try`/`catch`/`finally` syntax.

Features
--------

* Uses `setjmp()`.
* Supports `pthreads`.
* Does not use `malloc()`.
* Well-tested.

Caveats
-------

* Probably not compatble with C++ exceptions.
* Local flow-control, e.g. `break`, `continue`, `return`, `goto` outside of `crty_BEGIN`/`END` blocks may corrupt `ctry` dynamic block state.

Examples
--------

```C
enum {
  CANNOT_OPEN_FILE = 1,
};

FILE* open_file(const char *path)
{
  FILE *f;
  if ( ! (f = fopen(path, "r")) ) {
    ctry_raise(CANNOT_OPEN_FILE, 1, path);
  }
  return f;
}

int main(int arg, char **argv)
{
  ctry_BEGIN {
    ctry_BLOCK {
      FILE *f = open_file("non-existent.txt");
      char buf[1024] = { 0 };
      fread(buf, sizeof(buf[0]), sizeof(buf) - 1, f);
    }
    ctry_CATCH(CANNOT_OPEN_FILE) {
      ctry_return(1);
    }
  } ctry_END;
  
  return 0;
}

```
