#define ctry_PTHREAD 1
#include "ctry.h"
#include <assert.h>
#include <stdio.h>
#if ctry_PTHREAD
#include <pthread.h>
#include <signal.h>
#endif

#define AT() printf("\n  at %s %s:%d\n", __FUNCTION__, __FILE__, __LINE__)
// #define AT() (void) 0

static void test_body_wo_raise()
{
  int body = 0, catch_1 = 0, catch_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      body ++;
    }
    ctry_CATCH(1) {
      catch_1 ++;
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
  } ctry_END;

  assert(body == 1);
  assert(catch_1 == 0);
  assert(catch_2 == 0);
}

static void test_raise_w_catch()
{
  int catch_1 = 0, catch_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(1, 0);
      assert(! "reached");
    }
    ctry_CATCH(1) {
      catch_1 ++;
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
  } ctry_END;

  assert(catch_1 == 1);
  assert(catch_2 == 0);
}

static void nested_body()
{
  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(2, 0);
      assert(! "reached");
    }
    ctry_CATCH(3);
  } ctry_END;
}

static void test_raise_w_nested_body()
{
  int catch_1 = 0, catch_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      nested_body();
      assert(! "reached");
    }
    ctry_CATCH(1) {
      catch_1 ++;
    }
    ctry_CATCH_ANY {
      catch_2 ++;
    }
  } ctry_END;

  assert(catch_1 == 0);
  assert(catch_2 == 1);
}


static void nested_catch_w_reraise_2(int *catch_2)
{
  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(2, 0);
      assert(! "reached");
    }
    ctry_CATCH(2) {
      (*catch_2) ++;
      ctry_raise(1, 0);
      assert(! "reached");
    }
  } ctry_END;
}

static void test_nested_catch_w_reraise()
{
  int catch_1 = 0, catch_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      nested_catch_w_reraise_2(&catch_2);
      assert(! "reached");
    }
    ctry_CATCH(1) {
      catch_1 ++;
    }
  } ctry_END;

  assert(catch_2 == 1);
  assert(catch_1 == 1);
}


static void t5h(int *catch_1, int *finally)
{
  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(2, 0);
      assert(! "reached");
    }
    ctry_CATCH(1) {
      (*catch_1) ++;
    }
    ctry_FINALLY {
      (*finally) ++;
    }
  } ctry_END;
}

static void test_nested_w_2_finally()
{
  int catch_1 = 0, catch_2 = 0, finally_1 = 0, finally_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      t5h(&catch_1, &finally_2);
      assert(! "reached");
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
    ctry_FINALLY {
      finally_1 ++;
    }
  } ctry_END;
  assert(catch_2 == 1);
  assert(finally_1 == 1);
  assert(catch_1 == 0);
  assert(finally_2 == 1);
}


static void *uncaught_data;
static ctry_exc_t uncaught_exc;
static void uncaught_handler(ctry_exc_t *exc, void *data)
{
  uncaught_exc = *exc;
  uncaught_data = data;  
}
static void test_uncaught_exc_handler()
{
  int catch_2 = 0, finally = 0;
  ctry_thread_t thr_save = *ctry_thread_current();
  ctry_thread_current()->uncaught = uncaught_handler;
  ctry_thread_current()->uncaught_data = &thr_save;

  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(1, 2, (void*) 1, (void*) 2);
      assert(! "reached");
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
    ctry_FINALLY {
      finally ++;
    }
  } ctry_END;
  assert(catch_2 == 0);
  assert(finally == 1);
  assert(uncaught_data == &thr_save);
  assert(uncaught_exc.code == 1);
  assert(uncaught_exc.data_n == 2);
  assert(uncaught_exc.data[0] == (void*) 1);
  assert(uncaught_exc.data[1] == (void*) 2);
  assert(uncaught_exc.data[2] == 0);

  *ctry_thread_current() = thr_save;
}

static void test_catch_all_wo_raise()
{
  int catch_1 = 0;

  ctry_BEGIN {
    ctry_BODY {
    }
    ctry_CATCH_ANY {
      catch_1 ++;
    }
  } ctry_END;

  assert(catch_1 == 0);
}


static void test_raise_in_catch()
{
  int catch_1 = 0, catch_2 = 0, finally_1 = 0, finally_2 = 0;
  ctry_BEGIN {
    ctry_BODY {
      ctry_BEGIN {
        ctry_BODY {
          ctry_raise(1, 0);
          assert(! "reached");
        }
        ctry_CATCH(1) {
          catch_1 ++;
          ctry_raise(2, 0);
          assert(! "reached");
        }
        ctry_FINALLY {
          finally_1 ++;
        }
      } ctry_END;
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
    ctry_FINALLY {
      finally_2 ++;
    }
  } ctry_END;
  assert(catch_1 == 1);
  assert(catch_2 == 1);
  assert(finally_1 == 1);
  assert(finally_2 == 1);
}

static void test_raise_in_finally()
{
  int catch_1 = 0, catch_2 = 0, finally_1 = 0, finally_2 = 0;
  ctry_BEGIN {
    ctry_BODY {
      ctry_BEGIN {
        ctry_BODY {
          ctry_raise(1, 0);
          assert(! "reached");
        }
        ctry_CATCH(1) {
          catch_1 ++;
        }
        ctry_FINALLY {
          finally_1 ++;
          ctry_raise(2, 0);
          assert(! "reached");
        }
      } ctry_END;
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
    ctry_FINALLY {
      finally_2 ++;
    }
  } ctry_END;
  assert(catch_1 == 1);
  assert(catch_2 == 1);
  assert(finally_1 == 1);
  assert(finally_2 == 1);
}

static void test_raise_in_finally_wo_catch()
{
  int catch_2 = 0, finally_1 = 0, finally_2 = 0;

  ctry_BEGIN {
    ctry_BODY {
      ctry_BEGIN {
        ctry_BODY {
          ctry_raise(1, 0);
          assert(! "reached");
        }
        ctry_FINALLY {
          finally_1 ++;
          ctry_raise(2, 0);
          assert(! "reached");
        }
      } ctry_END;
    }
    ctry_CATCH(2) {
      catch_2 ++;
    }
    ctry_FINALLY {
      finally_2 ++;
    }
  } ctry_END;
  assert(catch_2 == 1);
  assert(finally_1 == 1);
  assert(finally_2 == 1);
}

static void test_raise_in_finally_uncaught()
{
  int finally_1 = 0;
  ctry_thread_t thr_save = *ctry_thread_current();
  ctry_thread_current()->uncaught = uncaught_handler;
  ctry_thread_current()->uncaught_data = &thr_save;

  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(1, 0);
      assert(! "reached");
    }
    ctry_FINALLY {
      finally_1 ++;
      ctry_raise(2, 0);
      assert(! "reached");
    }
  } ctry_END;
  assert(finally_1 == 1);
  assert(uncaught_data == &thr_save);
  assert(uncaught_exc.code == 2);

  *ctry_thread_current() = thr_save;
}

#ifdef ctry_PTHREAD
static int thr1_finally, thr2_finally;
static void* thr1_f(void *data)
{
  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(1, 0);
    }
    ctry_FINALLY {
      thr1_finally ++;
    }
  } ctry_END;
  return thr1_f;
}

static void* thr2_f(void *data)
{
  ctry_BEGIN {
    ctry_BODY {
      ctry_raise(2, 0);
    }
    ctry_FINALLY {
      thr2_finally ++;
    }
  } ctry_END;
  return thr2_f;
}

static void test_pthread_isolation()
{
  pthread_t thr1, thr2;
  void *thr1_v = 0, *thr2_v = 0;
  ctry_BEGIN {
    ctry_BODY {
      assert(! pthread_create(&thr1, 0, thr1_f, 0));
      assert(! pthread_create(&thr2, 0, thr2_f, 0));
    }
    ctry_FINALLY {
      assert(! pthread_join(thr1, &thr1_v));
      assert(! pthread_join(thr2, &thr2_v));
    }
  } ctry_END;
  assert(thr1_v == thr1_f);
  assert(thr2_v == thr2_f);
  assert(thr1_finally == 1);
  assert(thr2_finally == 1);
}
#endif

#define T(N) printf("%s %s ...\n", argv[0], #N); N (); printf("%s %s  OK\n", argv[0], #N)
int main(int argc, char **argv)
{
  T(test_body_wo_raise);
  T(test_raise_w_catch);
  T(test_raise_w_nested_body);
  T(test_nested_catch_w_reraise);
  T(test_nested_w_2_finally);
  T(test_uncaught_exc_handler);
  T(test_catch_all_wo_raise);
  T(test_raise_in_catch);
  T(test_raise_in_finally);
  T(test_raise_in_finally_wo_catch);
  T(test_raise_in_finally_uncaught);
#if ctry_PTHREAD
  T(test_pthread_isolation);
#endif
  return 0;
}

#include "ctry.c"
