/*
Copyright (c) 2015, Kurt Stephens
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#include "ctry.h"
#include <stdlib.h>
#include <string.h> /* memset */
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if ctry_PTHREAD
#include <pthread.h>
#include <signal.h>
#endif

void ctry_abort_default()
{
#if ctry_PTHREAD
  // pthread_kill(pthread_self(), SIGABRT);
  pthread_cancel(pthread_self());
#else
  abort();
#endif
}

void ctry_abort()
{
  ctry_thread_current()->abort();
}

void ctry_uncaught_default(ctry_exc_t *exc, void *data)
{
  FILE *out = stderr;
  void *thr = 0;
#if ctry_PTHREAD
  thr = (void*) pthread_self();
#endif
  fflush(out);
  fprintf(out, "\n  ctry: thr %p: UNCAUGHT: %d: raised at %s:%d %s\n",
          thr,
          (int) exc->code,
          exc->cntx.file, exc->cntx.line, exc->cntx.func);
  fflush(out);
  ctry_abort();
}

ctry_thread_t ctry_thread_defaults = {
  0,
  ctry_uncaught_default,
  0,
  ctry_abort_default,
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
    assert(thr);
    *thr = ctry_thread_defaults;
    pthread_setspecific(thread_key, thr);
  }
#else
  thr = &ctry_thread_defaults;
#endif
  return thr;
}

void _ctry_assert_failed(ctry_CONTEXT_PARAMS const char *expr)
{
  FILE *out = stderr;
  fflush(out);
  fprintf(out, "\n  ctry_assert(%s) %s:%d %s\n", expr, file, line, func);
  fflush(out);
  ctry_abort();
}

#ifdef assert
#undef assert
#endif
#define assert(X) \
  do { if ( ! (X) ) _ctry_assert_failed(ctry_CONTEXT_ARGS #X); } while(0)

#define ctry_SET_CONTEXT_(X)             \
  X.file = file;                         \
  X.line = line;                         \
  X.func = func;
#define ctry_SET_CONTEXT(NAME) ctry_SET_CONTEXT_(t->NAME)

void ctry_begin__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  memset(t, 0, sizeof(*t));
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
  assert(exc->code > 0);
  if ( ! t ) {
    ctry_thread_t *thr = ctry_thread_current();
    (thr->uncaught ? thr->uncaught : ctry_uncaught_default)(exc, thr->uncaught_data);
    return;
  }
  assert(t->_begin == 1);
  assert(t->_body  == 1);
  assert(t->_end   == 0);

  t->_exc = *exc;

  t->_raise = 1;
  t->_raise_at = exc->cntx;
  t->_state = t->_exc.code;
  t->_exc_pending = 1;
  longjmp(t->_jb, 1);
}

void ctry_do_finally(ctry_t *t)
{
  // If the FINALLY case has not been run,
  // loop once more and run it, if
  // it exists.
  // Note: The break in CATCH_ANY default case
  // will be run if there is no FINALLY case.
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

void ctry_raise__(ctry_CONTEXT_PARAMS int code, int data_n, ...)
{
  ctry_exc_t exc = { 0 };
  assert(code > 0);
  assert(data_n >= 0);
  assert(data_n < 4);
  exc.code = code;
  ctry_SET_CONTEXT_(exc.cntx);
  exc.data_n = data_n;
  {
    va_list va;
    int i;
    va_start(va, data_n);
    for ( i = 0; i < data_n; ++ i )
      exc.data[i] = va_arg(va, void*);
    va_end(va);
  }
  ctry_raise_exc(&exc);
}

int ctry_catch__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  assert(t);
  // Do not jump into CATCH_ANY default case when
  // there is no FINALLY case for _state == -1.
  if ( t->_state == -1 )
    return 1;
  assert(t->_exc.code > 0);
  t->_catch = 1;
  ctry_SET_CONTEXT(_catch_at);
  t->_exc_pending = 0;
  return 0;
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
  assert(t->_body  == 1);
  assert(t->_end   == 0);
  t->_end = 1;
  ctry_SET_CONTEXT(_end_at);
  ctry_thread_current()->curr = t->_prev;
  // If there is a pending (uncaught) exc,
  // raise it in the parent ctry_BODY.
  if ( t->_exc_pending ) {
    t->_exc_pending = 0;
    ctry_raise_exc(&t->_exc);
  }
}

int ctry_again__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  ctry_do_finally(t);
  int again = t->_again;
  t->_again = 0;
  return again;
}

void ctry_return__(ctry_CONTEXT_PARAMS ctry_t *t)
{
  ctry_end__(ctry_CONTEXT_ARGS t);
}

