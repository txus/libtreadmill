#ifndef _treadmill_gc_h
#define _treadmill_gc_h

#include <treadmill/darray.h>

typedef void (*TmReleaseFn)(void *value);

struct tm_cell_s;

typedef struct tm_cell_s {
  struct tm_cell_s *next;
  struct tm_cell_s *prev;
  void *value;
  char ecru;
} TmCell;

typedef struct tm_heap_s {
  TmCell *bottom;
  TmCell *top;
  TmCell *free;
  TmCell *scan;
  int growth_rate;
  int allocs;
  int scan_every;
  TmReleaseFn release;
  DArray *chunks;
} TmHeap;

typedef struct tm_chunk_s {
  TmCell *head;
  TmCell *tail;
} TmChunk;

TmHeap* TmHeap_new(int size, int growth_rate, TmReleaseFn release_fn);
void TmHeap_destroy(TmHeap* heap);

TmChunk TmChunk_new(int size);

#endif
