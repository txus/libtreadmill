#include "minunit.h"
#include <treadmill/gc.h>

#define assert_heap_size(A) mu_assert(TmHeap_size(heap) == (A), "Wrong heap size. Expected " #A)

void test_release(void *value)
{
}

char *test_TmHeap_new()
{
  TmHeap *heap = TmHeap_new(10, 10, test_release);

  assert_heap_size(10);

  TmHeap_destroy(heap);
  return NULL;
}

char *test_TmHeap_grow()
{
  TmHeap *heap = TmHeap_new(10, 10, test_release);
  TmHeap_grow(heap, 10);

  assert_heap_size(20);

  TmHeap_destroy(heap);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_TmHeap_new);
  mu_run_test(test_TmHeap_grow);

  return NULL;
}

RUN_TESTS(all_tests);

