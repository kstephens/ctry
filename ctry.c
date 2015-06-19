#include "ctry.h"
#include <stdlib.h>
#include <string.h> /* memset */
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if ctry_PTHREAD
#include <pthread.h>
#endif

void ctry_uncaught_default(ctry_exc_t *exc, void *data)
{
  fprintf(stderr, "\ntsy: UNCAUGHT: %d: raised at %s:%d %s\n",
          (int) exc->_e,
          exc->_cntx._file, exc->_cntx._line, exc->_cntx._func);
  abort();
}

ctry_thread_t ctry_thread_defaults = {
  0,
  ctry_uncaught_default,
};

#if ctry_PTHREAD
static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static pthread_key_t thread_key;
static void thread_init_func()
{
  assert(pthread_key_create(&thread_key, free) == 0);
}
static void ctry_thread_init()
{
  assert(pthread_once(&once_control, thread_init_func) == 0);
}
#endif

ctry_thread_t *ctry_thread_current()
{
  ctry_thread_t *thr = 0;
#if ctry_PTHREAD
  ctry_thread_init();
  if ( ! (thr = pthread_getspecific(thread_key)) ) {
    thr = malloc(sizeof(*thr));
    *thr = ctry_thread_defaults;
    pthread_setspecific(thread_key, thr);
  }
#else
  thr = &ctry_thread_defaults;
#endif
  return thr;
}

#define ctry_SET_CONTEXT_(X)                \
  X._file = file;                         \
  X._line = line;                         \
  X._func = func;
#define ctry_SET_CONTEXT(NAME) ctry_SET_CONTEXT_(t->NAME)

void ctry_begin__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  memset(t, 0, sizeof(*t));
  t->_jmpcode = -1;
  t->_begin = 1;
  ctry_SET_CONTEXT(_begin_at);
  ctry_thread_t *thr = ctry_thread_current();
  t->_prev = thr->curr;
  thr->curr = t;
}

void ctry_raise_exc(ctry_exc_t *exc)
{
  ctry_t* t = ctry_thread_current()->curr;
  assert(exc);
  assert(exc->_e > 0);
  if ( ! t ) {
    ctry_thread_t *thr = ctry_thread_current();
    (thr->uncaught ? thr->uncaught : ctry_uncaught_default)(exc, thr->uncaught_data);
    return;
  }
  assert(t->_begin == 1);
  assert(t->_end   == 0);

  t->_exc = *exc;

  t->_raise = 1;
  t->_raise_at = exc->_cntx;
  t->_state = t->_exc._e;
  t->_exc_pending = 1;
  longjmp(t->_jb, 1);
}

void ctry_do_finally(ctry_t *t)
{
  if ( ! t->_finally ) {
    t->_finally = 1;
    t->_state = -1;
    t->_again = 1;
  }
}

void ctry_body__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  t->_body = 1;
  ctry_SET_CONTEXT(_body_at);
}

void ctry_raise__(ctry_CONTEXT_PARAMS int e, int data_n, ...)
{
  ctry_exc_t exc = { 0 };
  assert(e > 0);
  assert(data_n >= 0);
  assert(data_n < 4);
  exc._e = e;
  ctry_SET_CONTEXT_(exc._cntx);
  exc._data_n = data_n;
  {
    va_list va;
    va_start(va, data_n);
    for ( int i = 0; i < data_n; ++ i )
      exc._data[i] = va_arg(va, void*);
    va_end(va);
  }
  ctry_raise_exc(&exc);
}

void ctry_catch__(ctry_CONTEXT_PARAMS ctry_t *t, int e)
{
  assert(t);
  assert(e == t->_exc._e);
  t->_catch = 1;
  ctry_SET_CONTEXT(_catch_at);
  t->_exc_pending = 0;
}

ctry_exc_t *ctry_exc()
{
  ctry_t *curr = ctry_thread_current()->curr;
  assert(curr);
  return &curr->_exc;
}

void ctry_finally__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  t->_finally = 1;
  ctry_SET_CONTEXT(_finally_at);
  t->_state = -2;
  t->_again = 0;
}

void ctry_end__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  assert(t->_begin == 1);
  assert(t->_end   == 0);
  t->_end = 1;
  ctry_SET_CONTEXT(_end_at);
  ctry_thread_current()->curr = t->_prev;
  if ( t->_exc_pending ) {
    t->_exc_pending = 0;
    ctry_raise_exc(&t->_exc);
  }
}

int ctry_again__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  ctry_do_finally(t);
  return t->_again;
}
