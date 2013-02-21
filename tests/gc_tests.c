#include "minunit.h"
#include <treadmill/gc.h>

void test_release(void *value)
{
}

char *test_TmHeap_new()
{
  TmHeap *heap = TmHeap_new(10, 10, test_release);
  TmHeap_destroy(heap);

  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_TmHeap_new);

  return NULL;
}

RUN_TESTS(all_tests);

