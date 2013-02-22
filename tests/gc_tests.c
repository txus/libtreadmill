#include "minunit.h"
#include <treadmill/gc.h>

#define assert_heap_size(A) mu_assert(TmHeap_size(heap) == (A), "Wrong heap size. Expected " #A)
#define assert_white_size(A) mu_assert(TmHeap_white_size(heap) == (A), "Wrong white size. Expected " #A)
#define assert_ecru_size(A) mu_assert(TmHeap_ecru_size(heap) == (A), "Wrong ecru size. Expected " #A)
#define assert_grey_size(A) mu_assert(TmHeap_grey_size(heap) == (A), "Wrong grey size. Expected " #A)
#define assert_black_size(A) mu_assert(TmHeap_black_size(heap) == (A), "Wrong black size. Expected " #A)

#define BOTTOM  heap->bottom
#define TOP     heap->top
#define SCAN    heap->scan
#define FREE    heap->free

typedef struct object_s {
  TmHeader gc;
  int health;
} Object;

Object*
Object_new(TmHeap *heap)
{
  Object *obj = (Object*)Tm_allocate(heap);
  obj->health = 100;
  return obj;
}

void
Object_destroy(Object *self)
{
  free(self);
}

void
test_release(void *value)
{
  Object_destroy((Object*)value);
}

char *test_TmHeap_new()
{
  TmHeap *heap = TmHeap_new(10, 10, sizeof(Object), test_release);

  assert_heap_size(10);

  assert_white_size(10);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(0);

  TmHeap_destroy(heap);
  return NULL;
}

char *test_TmHeap_grow()
{
  TmHeap *heap = TmHeap_new(3, 10, sizeof(Object), test_release);

  TmHeap_grow(heap, 2);

  assert_heap_size(5);
  assert_white_size(5);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(0);

  TmHeap_destroy(heap);
  return NULL;
}

char *test_TmHeap_allocate()
{
  TmHeap *heap = TmHeap_new(10, 10, sizeof(Object), test_release);

  Object *obj  = Object_new(heap);
  TmCell *cell = obj->gc.cell;

  mu_assert(cell == FREE->prev, "Cell should be right before the free pointer");

  assert_heap_size(10);

  assert_white_size(9);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(1);

  TmHeap_destroy(heap);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_TmHeap_new);
  mu_run_test(test_TmHeap_grow);
  mu_run_test(test_TmHeap_allocate);

  return NULL;
}

RUN_TESTS(all_tests);

