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

#ifndef _ctry_h_
#define _ctry_h_

#include <setjmp.h>

typedef struct ctry_context_t {
  const char *file;
  int line;
  const char *func;
} ctry_context_t;

typedef struct ctry_exc_t {
  int code;
  ctry_context_t cntx;
  int data_n;
  void *data[4];
} ctry_exc_t;

typedef struct ctry_t {
  jmp_buf _jb;
  struct ctry_t *_prev;
  int _state;
  unsigned int
    _begin   : 1,
    _body    : 1,
    _raise   : 1,
    _catch   : 1,
    _finally : 1,
    _end     : 1,
    _again   : 1,
    _exc_pending : 1;
  ctry_exc_t _exc;
  ctry_context_t _begin_at, _body_at, _raise_at, _catch_at, _finally_at, _end_at;
  void *unused[4];
} ctry_t;

typedef struct ctry_thread_t {
  ctry_t *curr;
  void (*uncaught)(ctry_exc_t *exc, void *data);
  void *uncaught_data;
  void (*abort)();
} ctry_thread_t;

extern ctry_thread_t ctry_thread_defaults;
ctry_thread_t *ctry_thread_current();

#define ctry_CONTEXT_PARAMS const char *file, int line, const char *func,
#define ctry_CONTEXT_ARGS   __FILE__, __LINE__, __FUNCTION__,
void ctry_begin__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_body__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_raise__(ctry_CONTEXT_PARAMS int code, int data_n, ...);
void ctry_return__(ctry_CONTEXT_PARAMS ctry_t *t);;
int  ctry_catch__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_finally__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_end__(ctry_CONTEXT_PARAMS ctry_t *t);
int  ctry_again__(ctry_CONTEXT_PARAMS ctry_t *t);
ctry_exc_t *ctry_exc();

#define ctry_raise(CODE, ...)                      \
  ctry_raise__(ctry_CONTEXT_ARGS (CODE), __VA_ARGS__)

#define ctry_RETURN_(N,X)                                    \
  return (ctry_return__(ctry_CONTEXT_ARGS &_ctry_##N), (X))

#define ctry_BEGIN_(N)                                       \
do {                                                         \
  ctry_t _ctry_##N;                                          \
  ctry_begin__(ctry_CONTEXT_ARGS &_ctry_##N);                \
  setjmp(_ctry_##N._jb);                                     \
  do {                                                       \
    switch ( _ctry_##N._state ) {
#define ctry_BODY_(N)                                        \
    case 0:                                                  \
      ctry_body__(ctry_CONTEXT_ARGS &_ctry_##N);

#define ctry_CATCH_(N,CODE)                                  \
    break;                                                   \
    case (CODE):                                             \
      ctry_catch__(ctry_CONTEXT_ARGS &_ctry_##N);

#define ctry_CATCH_ANY_(N)                                   \
    break;                                                   \
    default:                                                 \
      if ( ctry_catch__(ctry_CONTEXT_ARGS &_ctry_##N) ) break;

#define ctry_FINALLY_(N)                                     \
    break;                                                   \
    case -1:                                                 \
      ctry_finally__(ctry_CONTEXT_ARGS &_ctry_##N);

#define ctry_END_(N)                                         \
    }                                                        \
  } while ( ctry_again__(ctry_CONTEXT_ARGS &_ctry_##N) );    \
  ctry_end__(ctry_CONTEXT_ARGS &_ctry_##N);                  \
} while (0)

#define ctry_BEGIN     ctry_BEGIN_(_here)
#define ctry_BODY      ctry_BODY_(_here)
#define ctry_CATCH(CODE)  ctry_CATCH_(_here, (CODE))
#define ctry_CATCH_ANY ctry_CATCH_ANY_(_here)
#define ctry_FINALLY   ctry_FINALLY_(_here)
#define ctry_END       ctry_END_(_here)
#define ctry_RETURN(X) ctry_RETURN_(_here, (X))

#endif

