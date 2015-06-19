#ifndef _try_h_
#define _try_h_

#include <setjmp.h>

typedef struct try_context_t {
  const char *_file;
  int _line;
  const char *_func;
} try_context_t;

typedef struct try_exc_t {
  int _e;
  void *_data[4];
  int _data_n;
  try_context_t _cntx;
} try_exc_t;

typedef struct try_t {
  int _jmpcode;
  jmp_buf _jb;
  struct try_t *_prev;
  int _state, _again;
  unsigned int
    _begin   : 1,
    _body    : 1,
    _raise   : 1,
    _catch   : 1,
    _finally : 1,
    _end     : 1,
    _exc_pending : 1;
  try_exc_t _exc;
  try_context_t _begin_at, _body_at, _raise_at, _catch_at, _finally_at, _end_at;
} try_t;

typedef struct try_thread_t {
  try_t *curr;
  void (*uncaught)(try_exc_t *exc, void *data);
  void *uncaught_data;
} try_thread_t;

extern try_thread_t try_thread_defaults;
try_thread_t *try_thread_current();

#define try_CONTEXT_PARAMS const char *file, int line, const char *func
#define try_CONTEXT_ARGS   __FILE__, __LINE__, __FUNCTION__
void try_begin__(try_CONTEXT_PARAMS, try_t *t);
void try_body__(try_CONTEXT_PARAMS, try_t *t);
void try_raise__(try_CONTEXT_PARAMS, int e, int data_n, ...);
void try_catch__(try_CONTEXT_PARAMS, try_t *t, int e);
void try_finally__(try_CONTEXT_PARAMS, try_t *t);
void try_end__(try_CONTEXT_PARAMS, try_t *t);
int  try_again__(try_CONTEXT_PARAMS, try_t *t);
try_exc_t *try_exc();

#define try_raise(E, D)                                      \
  try_raise__(try_CONTEXT_ARGS, (E), (D))

#define try_BEGIN_(N)                                        \
do {                                                         \
  try_t _try_##N;                                            \
  try_begin__(try_CONTEXT_ARGS, &_try_##N);                  \
  _try_##N._jmpcode = setjmp(_try_##N._jb);                  \
  do {                                                       \
    _try_##N._again = 0;                                     \
    switch ( _try_##N._state ) {
#define try_BODY_(N)                                         \
    case 0:                                                  \
      try_body__(try_CONTEXT_ARGS, &_try_##N);

#define try_CATCH_(N,E)                                      \
    break;                                                   \
    case E:                                                  \
      try_catch__(try_CONTEXT_ARGS, &_try_##N, (E));

#define try_FINALLY_(N)                                      \
    break;                                                   \
    case -1:                                                 \
      try_finally__(try_CONTEXT_ARGS, &_try_##N);

#define try_END_(N)                                          \
    }                                                        \
      } while ( try_again__(try_CONTEXT_ARGS, &_try_##N) );  \
  try_end__(try_CONTEXT_ARGS, &_try_##N);                    \
} while (0)

#define try_BEGIN     try_BEGIN_(_here)
#define try_BODY      try_BODY_(_here)
#define try_CATCH(E)  try_CATCH_(_here, (E))
#define try_FINALLY   try_FINALLY_(_here)
#define try_END       try_END_(_here)

#endif
