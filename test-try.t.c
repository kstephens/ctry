#include "try.h"
#include <assert.h>
#include <stdio.h>

// #define AT() printf("\n  at %s %s:%d\n", __FUNCTION__, __FILE__, __LINE__)
#define AT() (void) 0

static void t1()
{
  int body = 0, catch = 0;

  try_BEGIN {
    try_BODY {
      body = 1;
    }
    try_CATCH(1) {
      catch = 1;
    }
    try_CATCH(2) {
      catch = 2;
    }
  } try_END;

  assert(body == 1);
  assert(catch == 0);
}

static void t2()
{
  int caught = 0;

  try_BEGIN {
    try_BODY {
      try_raise(1, 0);
      assert(! "reached");
    }
    try_CATCH(1) {
      caught = 1;
    }
    try_CATCH(2) {
      caught = 2;
    }
  } try_END;

  assert(caught == 1);
}

static void t3h()
{
  try_BEGIN {
    try_BODY {
      try_raise(2, 0);
      assert(! "reached");
    }
    try_CATCH(3);
  } try_END;
}

static void t3()
{
  int catch = 0;

  try_BEGIN {
    try_BODY {
      t3h();
      assert(! "reached");
    }
    try_CATCH(1) {
      catch = 1;
    }
    try_CATCH(2) {
      catch = 2;
    }
  } try_END;

  assert(catch == 2);
}


static int t4h_caught;
static void t4h()
{
  try_BEGIN {
    try_BODY {
      try_raise(2, 0);
      assert(! "reached");
    }
    try_CATCH(2) {
      t4h_caught = 2;
      try_raise(1, 0);
      assert(! "reached");
    }
  } try_END;
}

static void t4()
{
  int caught = 0;

  try_BEGIN {
    try_BODY {
      t4h();
      assert(! "reached");
    }
    try_CATCH(1) {
      caught = 1;
    }
  } try_END;

  assert(t4h_caught == 2);
  assert(caught == 1);
}


static void t5t()
{
  AT();
  try_raise(2, 0);
  assert(! "reached");
}

static int t5h_catch, t5h_finally;
static void t5h()
{
  try_BEGIN {
    try_BODY {
      AT();
      t5t();
    }
    try_CATCH(1) {
      AT();
      t5h_catch = 1;
    }
    try_FINALLY {
      AT();
      t5h_finally = 1;
    }
  } try_END;
}

static void t5()
{
  int catch = 0, finally = 0;

  try_BEGIN {
    try_BODY {
      AT();
      t5h();
      assert(! "reached");
    }
    try_CATCH(2) {
      AT();
      catch = 2;
    }
    try_FINALLY {
      AT();
      finally = 1;
    }
  } try_END;
  assert(catch == 2);
  assert(finally == 1);
  assert(t5h_catch == 0);
  assert(t5h_finally == 1);
}


static void *uncaught_data;
static void uncaught(try_exc_t *exc, void *data)
{
  uncaught_data = data;  
}
static void t6()
{
  int body_end = 0, catch = 0, finally = 0;

  try_uncaught = uncaught;
  try_uncaught_data = t6;

  try_BEGIN {
    try_BODY {
      AT();
      try_raise(1, 0);
      assert(! "reached");
    }
    try_CATCH(2) {
      AT();
      catch = 2;
    }
    try_FINALLY {
      AT();
      finally = 1;
    }
  } try_END;
  assert(catch == 0);
  assert(finally == 1);
  assert(uncaught_data == t6);
}

#define T(N) printf("test: %s ...\n", #N); N (); printf("test: %s: OK\n", #N)
int main(int argc, char **argv)
{
  T(t1);
  T(t2);
  T(t3);
  T(t3);
  T(t4);
  T(t5);
  T(t6);
  return 0;
}
