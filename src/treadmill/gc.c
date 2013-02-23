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

/*
 * -(bottom)- ECRU -(top)- GREY -(scan)- BLACK -(free)- WHITE ...
 */

TmHeap*
TmHeap_new(TmStateHeader* state, int size, int growth_rate, size_t object_size, TmReleaseFn release_fn)
{
  TmHeap *heap = calloc(1, sizeof(TmHeap));

  heap->state  = state;
  heap->chunks = DArray_create(sizeof(TmCell*), 100);

  TmChunk chunk = TmChunk_new(size);
  TmCell *head = chunk.head;
  TmCell *tail = chunk.tail;

  // Save a reference to the chunk to deallocate it later
  DArray_push(heap->chunks, head);

  heap->growth_rate = growth_rate;
  heap->release     = release_fn;
  heap->object_size = object_size;
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
TmHeap_print_all(TmHeap *heap)
{
  TmCell *ptr = TOP;
  printf(
    "[HEAP] (%0.f) (ECRU %0.f | GREY %0.f | BLACK %0.f | WHITE %0.f)\n",
    TmHeap_size(heap),
    TmHeap_ecru_size(heap),
    TmHeap_grey_size(heap),
    TmHeap_black_size(heap),
    TmHeap_white_size(heap)
    );
  do {
    printf("* %p", ptr);
    if(ptr == TOP) printf(" (TOP)");
    if(ptr == BOTTOM) printf(" (BOTTOM)");
    if(ptr == FREE) printf(" (FREE)");
    if(ptr == SCAN) printf(" (SCAN)");
    printf("\n");
  } while((ptr = ptr->next) && ptr != TOP);
  printf("[END HEAP]\n");
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

  if(BOTTOM == FREE) BOTTOM = head;
  if(TOP    == FREE) TOP = head;
  if(SCAN   == FREE) SCAN = head;
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
    RELEASE(ptr->value);
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

static inline void
scan_cell(TmHeap *heap, TmCell *cell)
{
  debug("SCANNED %p\n", cell);
}

void
Tm_scan(TmHeap *heap)
{
  // If scan == top, the collection has finished
  if(SCAN == TOP) {
    debug("[GC]: No more grey objects to scan.");
    return;
  }

  // Move the scan pointer backwards, converting the scanned grey cell into a
  // black cell.
  SCAN = SCAN->prev;
  scan_cell(heap, SCAN);
}

static inline void
unsnap(TmHeap *heap, TmCell* me) {
  TmCell *my_prev = me->prev;
  TmCell *my_next = me->next;
  me->next = NULL;
  me->prev = NULL;

  if(BOTTOM == me) BOTTOM = my_next;
  if(TOP    == me) TOP    = my_next;
  if(SCAN   == me) SCAN   = my_next;
  if(FREE   == me) FREE   = my_next;

  my_prev->next = my_next;
  my_next->prev = my_prev;
}

static inline void
insert_in(TmHeap *heap, TmCell* me, TmCell* him) {
  if(me == him) return; // we do nothing

  unsnap(heap, me);

  TmCell *his_prev = him->prev;

  his_prev->next = me;
  him->prev      = me;

  me->prev  = his_prev;
  me->next  = him;

  if(him == TOP)    TOP = me;
  if(him == BOTTOM) BOTTOM = me;
  if(him == SCAN)   SCAN = me;
  if(him == FREE)   FREE = me;
}

static inline void
make_ecru(TmHeap *heap, TmCell *self)
{
  if(self == BOTTOM) {
    if (self == TOP)  TOP  = self->next;
    if (self == SCAN) SCAN = self->next;
    if (self == FREE) FREE = self->next;
  } else {
    insert_in(heap, self, BOTTOM);
  }
  self->ecru = 1;
}

void
Tm_flip(TmHeap *heap)
{
  // Scan all the grey cells before flipping.
  while(SCAN != TOP) Tm_scan(heap);

  TmCell *ptr = NULL;

  // Make all the ecru into white and release them
  ITERATE(BOTTOM, TOP, ptr) {
    ptr->ecru = 0;
    RELEASE(ptr->value);
    ptr = ptr->next;
  }
  BOTTOM = TOP;

  TmHeap_grow(heap, heap->growth_rate);

  // Make all black into ecru.
  ITERATE(SCAN, FREE, ptr) {
    TmCell *next = ptr->next;
    make_ecru(heap, ptr);
    ptr = next;
  }
}

TmObjectHeader*
Tm_allocate(TmHeap *heap)
{
  /*
   * If there are no slots in the white list,
   * force a collection.
   */
  if(FREE->next == BOTTOM) Tm_flip(heap);

  TmObjectHeader *header = calloc(1, heap->object_size);
  check(header, "Out of memory.");

  TmCell *free = FREE;
  header->cell = free;
  header->cell->value = header;

  FREE = FREE->next;

  heap->allocs++;

  return header;
error:
  exit(EXIT_FAILURE);
  return NULL;
}
