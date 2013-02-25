#include <treadmill/darray.h>
#include <assert.h>

Tm_DArray *Tm_DArray_create(size_t element_size, size_t initial_max)
{
  Tm_DArray *array = malloc(sizeof(Tm_DArray));
  check_mem(array);
  array->max = initial_max;
  check(array->max > 0, "You must set an initial_max > 0.");

  array->contents = calloc(initial_max, sizeof(void *));
  check_mem(array->contents);

  array->end = 0;
  array->element_size = element_size;
  array->expand_rate = DEFAULT_EXPAND_RATE;

  return array;

error:
  if(array) free(array);
  return NULL;
}

void Tm_DArray_clear(Tm_DArray *array)
{
  int i = 0;
  if(array->element_size > 0) {
    for(i = 0; i < array->max; i++) {
      if(array->contents[i] != NULL) {
        free(array->contents[i]);
      }
    }
  }
}

static inline int Tm_DArray_resize(Tm_DArray *array, size_t newsize)
{
  array->max = newsize;
  check(array->max > 0, "The newsize must be > 0.");

  void *contents = realloc(array->contents, array->max * sizeof(void *));
  // check contents and assume realloc doesn't harm the original on error

  check_mem(contents);

  array->contents = contents;

  return 0;
error:
  return -1;
}

int Tm_DArray_expand(Tm_DArray *array)
{
  size_t old_max = array->max;
  check(Tm_DArray_resize(array, array->max + array->expand_rate) == 0,
    "Failed to expand array to new size: %d",
    array->max + (int)array->expand_rate);

  memset(array->contents + old_max, 0, array->expand_rate + 1);
  return 0;

error:
  return -1;
}

int Tm_DArray_contract(Tm_DArray *array)
{
  int new_size = array->end < (int)array->expand_rate ? (int)array->expand_rate : array->end;

  return Tm_DArray_resize(array, new_size + 1);
}

void Tm_DArray_destroy(Tm_DArray *array)
{
  if(array) {
    if(array->contents) free(array->contents);
    free(array);
  }
}

void Tm_DArray_clear_destroy(Tm_DArray *array)
{
  Tm_DArray_clear(array);
  Tm_DArray_destroy(array);
}

int Tm_DArray_push(Tm_DArray *array, void *el)
{
  array->contents[array->end] = el;
  array->end++;

  if(Tm_DArray_end(array) >= Tm_DArray_max(array)) {
    return Tm_DArray_expand(array);
  } else {
    return 0;
  }
}

void *Tm_DArray_pop(Tm_DArray *array)
{
  check(array->end - 1 >= 0, "Attempt to pop from empty array.");

  void *el = Tm_DArray_remove(array, array->end - 1);
  array->end--;

  if (Tm_DArray_end(array) > (int)array->expand_rate && Tm_DArray_end(array) % array->expand_rate) {
    Tm_DArray_contract(array);
  }

  return el;
error:
  return NULL;
}

void Tm_DArray_set(Tm_DArray *array, int i, void *el)
{
  check(i < array->max, "darray attempt to set past max");
  array->contents[i] = el;
error:
  return;
}

