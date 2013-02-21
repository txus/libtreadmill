#include <stdlib.h>
#include <treadmill/gc.h>

#define BOTTOM  heap->bottom
#define TOP     heap->top
#define SCAN    heap->scan
#define FREE    heap->free
#define RELEASE heap->release

#define ITERATE(A, B, N) \
  (N) = (A);             \
  while((N) != (B))

TmHeap*
TmHeap_new(int size, int growth_rate, TmReleaseFn release_fn)
{
  TmHeap *heap = calloc(1, sizeof(TmHeap));

  heap->chunks = DArray_create(sizeof(TmCell*), 100);

  TmChunk chunk = TmChunk_new(size);
  TmCell *head = chunk.head;
  TmCell *tail = chunk.tail;

  // Save a reference to the chunk to deallocate it later
  DArray_push(heap->chunks, head);

  heap->growth_rate = growth_rate;
  heap->release     = release_fn;
  heap->allocs      = 0;
  heap->scan_every  = 5;

  FREE   = head;
  BOTTOM = head;
  TOP    = head;
  SCAN   = head;

  // Close the circle.
  tail->next = head;
  head->prev = tail;

  return heap;
}

void
TmHeap_grow(TmHeap *heap, int size)
{
  TmChunk chunk = TmChunk_new(size);
  TmCell *head = chunk.head;
  TmCell *tail = chunk.tail;

  // Save a reference to the chunk to deallocate it later
  DArray_push(heap->chunks, head);

  // Put the new chunk before the current free.
  TmCell *oldfree  = FREE;
  TmCell *previous = oldfree->prev;

  // Attach tail
  oldfree->prev = tail;
  tail->next    = oldfree;

  // Attach head
  previous->next = head;
  head->prev     = previous;

  FREE = head;
}

static inline int
TmHeap_distance_between(TmCell *a, TmCell *b)
{
  int count = 1;
  TmCell *ptr = a;
  while((ptr = ptr->next) && ptr != b) count++;

  return count;
}


double
TmHeap_size(TmHeap *heap)
{
  return TmHeap_distance_between(TOP, TOP);
}

double
TmHeap_white_size(TmHeap *heap)
{
  if(FREE == BOTTOM &&
     FREE == TOP &&
     FREE == SCAN) {
    return TmHeap_size(heap);
  }

  return TmHeap_distance_between(FREE, BOTTOM);
}

double
TmHeap_ecru_size(TmHeap *heap)
{
  if(BOTTOM == TOP) return 0;
  return TmHeap_distance_between(BOTTOM, TOP);
}

double
TmHeap_grey_size(TmHeap *heap)
{
  if(TOP == SCAN) return 0;
  return TmHeap_distance_between(TOP, SCAN);
}

double
TmHeap_black_size(TmHeap *heap)
{
  if(SCAN == FREE) return 0;
  return TmHeap_distance_between(SCAN, FREE);
}

void
TmHeap_destroy(TmHeap* heap)
{
  TmCell *ptr = NULL;

  ITERATE(BOTTOM, FREE, ptr) {
    RELEASE(ptr);
    ptr = ptr->next;
  }

  for(int i=0; i < DArray_count(heap->chunks); i++) {
    TmCell *chunk = (TmCell*)DArray_at(heap->chunks, i);
    free(chunk);
  }

  DArray_destroy(heap->chunks);

  free(heap);
}

TmChunk
TmChunk_new(int size)
{
  TmCell *memory = calloc(size, sizeof(TmCell));
  TmCell *ptr    = memory;

  for(int i=0; i < size; i++) {
    if(i>0) {
      TmCell *prev = ptr; prev--;
      ptr->prev = prev;
    }
    if(i<size) {
      TmCell *next = ptr; next++;
      ptr->next = next;
    }
    ptr++;
  }

  TmCell *tail = --ptr;

  memory->prev = NULL;
  tail->next   = NULL;

  TmChunk chunk = { .head = memory, .tail = tail };
  return chunk;
}
