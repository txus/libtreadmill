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

typedef struct state_s {
  TmStateHeader gc;
} State;

typedef struct object_s {
  TmObjectHeader gc;
  int health;
  DArray *children;
} Object;

void
test_rootset(DArray *rootset, TmStateHeader *state)
{
  /* DArray_push(rootset, state->) */
  /* state->rootset_fn(rootset, state); */
}

State*
State_new()
{
  State *state = calloc(1, sizeof(State));
  state->gc.rootset = test_rootset;
  return state;
}

Object*
Object_new(TmHeap *heap)
{
  Object *obj = (Object*)Tm_allocate(heap);
  obj->health = 100;
  obj->children = DArray_create(sizeof(Object*), 10);
  return obj;
}

void
Object_relate(Object* parent, Object* child)
{
  DArray_push(parent->children, child);
}

void
Object_destroy(Object *self)
{
  DArray_destroy(self->children);
  free(self);
}

void
test_release(void *value)
{
  Object_destroy((Object*)value);
}

char *test_TmHeap_new()
{
  State *state = State_new();
  TmHeap *heap = TmHeap_new((TmStateHeader*)state, 10, 10, sizeof(Object), test_release);

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
  State *state = State_new();
  TmHeap *heap = TmHeap_new((TmStateHeader*)state, 3, 10, sizeof(Object), test_release);

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
  State *state = State_new();
  TmHeap *heap = TmHeap_new((TmStateHeader*)state, 10, 10, sizeof(Object), test_release);

  Object *obj  = Object_new(heap);
  TmCell *cell = obj->gc.cell;

  mu_assert(cell == FREE->prev, "Cell should be right before the free pointer");
  mu_assert(heap->allocs == 1, "Allocation didn't update the allocs count.");

  assert_heap_size(10);

  assert_white_size(9);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(1);

  TmHeap_destroy(heap);
  return NULL;
}

char *test_TmHeap_allocate_and_flip()
{
  State *state = State_new();
  TmHeap *heap = TmHeap_new((TmStateHeader*)state, 3, 3, sizeof(Object), test_release);

  Object_new(heap);
  Object_new(heap);
  Object_new(heap); // this triggers a flip

  assert_heap_size(6);

  assert_white_size(3); // the newly grown region
  assert_ecru_size(2);  // the two first allocated objects
  assert_grey_size(0);
  assert_black_size(1); // the third allocated object

  TmHeap_destroy(heap);
  return NULL;
}

char *test_TmHeap_allocate_and_flip_twice()
{
  State *state = State_new();
  TmHeap *heap = TmHeap_new((TmStateHeader*)state, 3, 3, sizeof(Object), test_release);

  /*
   * parent1 ->
   *   child11
   *   child12
   */
  Object *parent1 = Object_new(heap);
  Object *child11 = Object_new(heap);
  Object *child12 = Object_new(heap); // this triggers a flip

  Object_relate(parent1, child11);
  Object_relate(parent1, child12);

  Object_new(heap);
  Object_new(heap);
  Object_new(heap); // this triggers another flip

  assert_heap_size(9);

  assert_white_size(3); // the newly grown region
  assert_ecru_size(5);
  assert_grey_size(0);
  assert_black_size(1); // the last allocated object

  TmHeap_destroy(heap);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_TmHeap_new);
  mu_run_test(test_TmHeap_grow);
  mu_run_test(test_TmHeap_allocate);
  mu_run_test(test_TmHeap_allocate_and_flip);
  mu_run_test(test_TmHeap_allocate_and_flip_twice);

  return NULL;
}

RUN_TESTS(all_tests);

