#ifndef _ctry_h_
#define _ctry_h_

#include <setjmp.h>

typedef struct ctry_context_t {
  const char *file;
  int line;
  const char *func;
} ctry_context_t;

typedef struct ctry_exc_t {
  int e;
  void *data[4];
  int data_n;
  ctry_context_t cntx;
} ctry_exc_t;

typedef struct ctry_t {
  int _jmpcode;
  jmp_buf _jb;
  struct ctry_t *_prev;
  int _state, _again;
  unsigned int
    _begin   : 1,
    _body    : 1,
    _raise   : 1,
    _catch   : 1,
    _finally : 1,
    _end     : 1,
    _exc_pending : 1;
  ctry_exc_t _exc;
  ctry_context_t _begin_at, _body_at, _raise_at, _catch_at, _finally_at, _end_at;
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
void ctry_raise__(ctry_CONTEXT_PARAMS int e, int data_n, ...);
void ctry_catch__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_finally__(ctry_CONTEXT_PARAMS ctry_t *t);
void ctry_end__(ctry_CONTEXT_PARAMS ctry_t *t);
int  ctry_again__(ctry_CONTEXT_PARAMS ctry_t *t);
ctry_exc_t *ctry_exc();

#define ctry_raise(E, D)                                     \
  ctry_raise__(ctry_CONTEXT_ARGS (E), (D))

#define ctry_BEGIN_(N)                                       \
do {                                                         \
  ctry_t _ctry_##N;                                          \
  ctry_begin__(ctry_CONTEXT_ARGS &_ctry_##N);                \
  _ctry_##N._jmpcode = setjmp(_ctry_##N._jb);                \
  do {                                                       \
    _ctry_##N._again = 0;                                    \
    switch ( _ctry_##N._state ) {
#define ctry_BODY_(N)                                        \
    case 0:                                                  \
      ctry_body__(ctry_CONTEXT_ARGS &_ctry_##N);

#define ctry_CATCH_(N,E)                                     \
    break;                                                   \
    case E:                                                  \
      ctry_catch__(ctry_CONTEXT_ARGS &_ctry_##N);

#define ctry_CATCH_ANY_(N)                                   \
    break;                                                   \
    default:                                                 \
      if ( _ctry_##N._state == -1 ) break;                   \
      ctry_catch__(ctry_CONTEXT_ARGS &_ctry_##N);

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
#define ctry_CATCH(E)  ctry_CATCH_(_here, (E))
#define ctry_CATCH_ANY ctry_CATCH_ANY_(_here)
#define ctry_FINALLY   ctry_FINALLY_(_here)
#define ctry_END       ctry_END_(_here)

#endif

