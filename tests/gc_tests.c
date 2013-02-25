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
  DArray *registers;
} State;

typedef struct object_s {
  TmObjectHeader gc;
  int health;
  DArray *children;
} Object;

DArray*
test_rootset(TmStateHeader *state_h)
{
  DArray *rootset = DArray_create(sizeof(TmObjectHeader*), 10);
  State *state = (State*)state_h;
  for(int i=0; i<DArray_count(state->registers);i++) {
    DArray_push(rootset, DArray_at(state->registers, i));
  }

  return rootset;
}

void
test_scan_pointers(TmHeap *heap, TmObjectHeader *object, TmCallbackFn callback)
{
  Object *self = (Object*)object;
  for(int i=0; i < DArray_count(self->children); i++) {
    TmObjectHeader *o = (TmObjectHeader*)DArray_at(self->children, i);
    callback(heap, o);
  }
}

State*
State_new()
{
  State *state = calloc(1, sizeof(State));
  state->gc.rootset = test_rootset;
  state->registers = DArray_create(sizeof(Object*), 10);
  return state;
}

void
State_destroy(State *state)
{
  DArray_destroy(state->registers);
  free(state);
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
Object_print(Object *self)
{
  printf("#<Object %p @cell=%p, @health=%i, @children=%i>\n", self, self->gc.cell, self->health, DArray_count(self->children));
}

void
Object_relate(Object* parent, Object* child)
{
  DArray_push(parent->children, child);
}

void
Object_make_root(Object *self, State *state)
{
  DArray_push(state->registers, self);
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

TmHeap*
new_heap(State *state, int size, int growth_rate)
{
  return TmHeap_new(
    (TmStateHeader*)state,
    size,
    growth_rate,
    sizeof(Object),
    test_release,
    test_scan_pointers
    );
}

char *test_TmHeap_new()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 10, 10);

  assert_heap_size(11);

  assert_white_size(11);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(0);

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *test_TmHeap_grow()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 3, 10);

  assert_heap_size(4);

  TmHeap_grow(heap, 2);

  assert_heap_size(6);
  assert_white_size(6);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(0);

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *test_TmHeap_allocate()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 10, 10);

  Object *obj  = Object_new(heap);
  TmCell *cell = obj->gc.cell;

  mu_assert(cell == FREE->prev, "Cell should be right before the free pointer");
  mu_assert(heap->allocs == 1, "Allocation didn't update the allocs count.");

  assert_heap_size(11);

  assert_white_size(10);
  assert_ecru_size(0);
  assert_grey_size(0);
  assert_black_size(1);

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *test_TmHeap_allocate_and_flip()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 3, 3);

  Object_new(heap);
  Object_new(heap);
  Object_new(heap);
  Object_new(heap); // this triggers a flip

  assert_heap_size(7);

  assert_white_size(3); // the newly grown region
  assert_ecru_size(3);  // the three first allocated objects
  assert_grey_size(0);
  assert_black_size(1); // the last allocated object

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *test_TmHeap_allocate_and_flip_twice()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 3, 3);

  /*
   * parent1 ->
   *   child11
   *   child12
   */
  Object *parent1 = Object_new(heap);
  Object_make_root(parent1, state);
  Object *child11 = Object_new(heap);
  Object_relate(parent1, child11);

  Object *child12 = Object_new(heap); // this triggers a flip
  Object_relate(parent1, child12);

  Object_new(heap);
  Object_new(heap);
  Object_new(heap);
  Object_new(heap); // this triggers another flip

  assert_heap_size(10);

  assert_white_size(3); // the newly grown region
  assert_ecru_size(5);
  assert_grey_size(1);
  assert_black_size(1); // the last allocated object

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *test_TmHeap_allocate_and_grow_slowly()
{
  State *state = State_new();
  TmHeap *heap = new_heap(state, 3, 1);

  Object *immortal = Object_new(heap);
  Object_make_root(immortal, state);

  int times = 4;
  while(times--) {
    Object *obj = Object_new(heap);
    mu_assert(obj->health == 100, "Wrong object.");
  }

  TmHeap_destroy(heap);
  State_destroy(state);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_TmHeap_new);
  mu_run_test(test_TmHeap_grow);
  mu_run_test(test_TmHeap_allocate);
  mu_run_test(test_TmHeap_allocate_and_flip);
  mu_run_test(test_TmHeap_allocate_and_flip_twice);
  mu_run_test(test_TmHeap_allocate_and_grow_slowly);

  return NULL;
}

RUN_TESTS(all_tests);

