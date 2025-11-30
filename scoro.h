#ifndef SCORO_H_
#define SCORO_H_
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifndef CR_STACK_SIZE
#   define CR_STACK_SIZE 128
#endif

#define CR_STATUS_BLOCKED 0
#define CR_STATUS_FINISHED -1
#define CR_STATUS_YIELDED -2

typedef struct {
  jmp_buf env;
  int isset;
  int status;
  unsigned char stack[CR_STACK_SIZE];
  char* last_saved_adress;
} Cr;

#define cr_begin(cr)                          \
  volatile Cr __begin;                        \
  do {                                        \
  do {                                        \
    if ((cr)->isset) {                        \
      memcpy((cr)->last_saved_adress, &((cr)->stack[0]), CR_STACK_SIZE);\
      longjmp((cr)->env, 0);                  \
    }                                         \
  } while (0)
#define cr_label(cr, stat)                    \
  do {                                        \
    (cr)->isset = 1;                          \
    (cr)->status = (stat);                    \
    (cr)->last_saved_adress =  ((char*)&__begin)-CR_STACK_SIZE; \
    memcpy(&((cr)->stack[0]), (cr)->last_saved_adress, CR_STACK_SIZE);  \
    for (int i = 0; i < CR_STACK_SIZE; ++i); \
    setjmp((cr)->env);                        \
  } while (0)
#define cr_end(cr) cr_label(cr, CR_STATUS_FINISHED); } while(0)
#define cr_yield(cr)                          \
  do {                                        \
    cr_label(cr, CR_STATUS_YIELDED);          \
    if (cr_status(cr) == CR_STATUS_YIELDED) { \
      (cr)->status = CR_STATUS_BLOCKED;       \
      return;                                 \
    }                                         \
  } while (0)
#define cr_status(cr) (cr)->status

#endif /* SCORO_H_ */
