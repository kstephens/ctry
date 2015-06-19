#include "try.h"
#include <stdlib.h>
#include <string.h> /* memset */
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

static try_t *curr; /* NOT THREAD-SAFE */

int try_stop;

void try_debug_stop_at()
{
  fprintf(stderr, "\ntry_debug_stop_at()\n");
}

void try_stop_at()
{
  if ( try_stop )
    try_debug_stop_at();
}

#define try_SET_CONTEXT_(X)                \
  X._file = file;                         \
  X._line = line;                         \
  X._func = func;
#define try_SET_CONTEXT(NAME) try_SET_CONTEXT_(t->NAME)

void try_begin__(try_CONTEXT_PARAMS, try_t *t)
{
  assert(t);
  memset(t, 0, sizeof(*t));
  t->_jmpcode = -1;
  t->_begin = 1;
  try_SET_CONTEXT(_begin_at);
  t->_prev = curr;
  curr = t;
}

void try_uncaught_default(try_exc_t *exc, void *data)
{
  fprintf(stderr, "\ntsy: UNCAUGHT: %d: raised at %s:%d %s\n",
          (int) exc->_e,
          exc->_cntx._file, exc->_cntx._line, exc->_cntx._func);
  abort();
}

void (*try_uncaught)(try_exc_t *exc, void *data) = try_uncaught_default;
void *try_uncaught_data;

void try_raise_exc(try_exc_t *exc)
{
  try_t* t = curr;
  try_stop_at();
  assert(exc);
  assert(exc->_e > 0);
  if ( ! t ) {
    (try_uncaught ? try_uncaught : try_uncaught_default)(exc, try_uncaught_data);
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

void try_do_finally(try_t *t)
{
  if ( ! t->_finally ) {
    t->_finally = 1;
    t->_state = -1;
    t->_again = 1;
  }
}

void try_body__(try_CONTEXT_PARAMS, try_t *t)
{
  assert(t);
  t->_body = 1;
  try_SET_CONTEXT(_body_at);
}

void try_raise__(try_CONTEXT_PARAMS, int e, int data_n, ...)
{
  try_exc_t exc = { 0 };
  assert(e > 0);
  assert(data_n >= 0);
  assert(data_n < 4);
  exc._e = e;
  try_SET_CONTEXT_(exc._cntx);
  exc._data_n = data_n;
  {
    va_list va;
    va_start(va, data_n);
    for ( int i = 0; i < data_n; ++ i )
      exc._data[i] = va_arg(va, void*);
    va_end(va);
  }
  try_raise_exc(&exc);
}

void try_catch__(try_CONTEXT_PARAMS, try_t *t, int e)
{
  assert(t);
  assert(e == t->_exc._e);
  t->_catch = 1;
  try_SET_CONTEXT(_catch_at);
  t->_exc_pending = 0;
}

try_exc_t *try_exc()
{
  assert(curr);
  return &curr->_exc;
}

void try_finally__(try_CONTEXT_PARAMS, try_t *t)
{
  assert(t);
  t->_finally = 1;
  try_SET_CONTEXT(_finally_at);
  t->_state = -2;
  t->_again = 0;
}

void try_end__(try_CONTEXT_PARAMS, try_t *t)
{
  assert(t);
  assert(t->_begin == 1);
  assert(t->_end   == 0);
  t->_end = 1;
  try_SET_CONTEXT(_end_at);
  curr = t->_prev;
  if ( t->_exc_pending ) {
    t->_exc_pending = 0;
    try_raise_exc(&t->_exc);
  }
}

int try_again__(try_CONTEXT_PARAMS, try_t *t)
{
  try_do_finally(t);
  return t->_again;
}
