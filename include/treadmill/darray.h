#ifndef _Tm_DArray_h_
#define _Tm_DArray_h_
#include <stdlib.h>
#include <assert.h>
#include <treadmill/_dbg.h>

typedef struct tm_darray_s {
  int end;
  int max;
  size_t element_size;
  size_t expand_rate;
  void **contents;
} Tm_DArray;

Tm_DArray *Tm_DArray_create(size_t element_size, size_t initial_max);

void Tm_DArray_destroy(Tm_DArray *array);
void Tm_DArray_clear(Tm_DArray *array);
int Tm_DArray_expand(Tm_DArray *array);
int Tm_DArray_contract(Tm_DArray *array);
int Tm_DArray_push(Tm_DArray *array, void *el);
void *Tm_DArray_pop(Tm_DArray *array);
void Tm_DArray_clear_destroy(Tm_DArray *array);

#define Tm_DArray_last(A) ((A)->contents[(A)->end - 1])
#define Tm_DArray_first(A) ((A)->contents[0])
#define Tm_DArray_at(A, I) ((A)->contents[(I)])
#define Tm_DArray_end(A) ((A)->end)
#define Tm_DArray_count(A) Tm_DArray_end(A)
#define Tm_DArray_max(A) ((A)->max)

#define DEFAULT_EXPAND_RATE 300

void Tm_DArray_set(Tm_DArray *array, int i, void *el);

static inline void *Tm_DArray_get(Tm_DArray *array, int i)
{
  check(i < array->max, "darray attempt to get past max");
  return array->contents[i];
error:
  return NULL;
}

static inline void *Tm_DArray_remove(Tm_DArray *array, int i)
{
  void *el = array->contents[i];

  array->contents[i] = NULL;

  return el;
}

static inline void *Tm_DArray_new(Tm_DArray *array)
{
  check(array->element_size > 0, "Can't use Tm_DArray_new on 0 size darrays.");

  return calloc(1, array->element_size);

error:
  return NULL;
}

#define Tm_DArray_free(E) free((E))

#endif
