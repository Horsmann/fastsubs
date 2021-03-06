#include <assert.h>
#include "heap.h"

/* With a one based binary heap parent is at i/2 and children are at
   2i, 2i+1. */

void heap_insert_min(Heap heap, Token key, float value) {
  uint32_t n = ++heap_size(heap);
  while (n > 1) {
    uint32_t p = n >> 1;
    if (heap[p].logp <= value) break; /*  */
    heap[n] = heap[p];
    n = p;
  }
  heap[n].token = key;
  heap[n].logp = value;
}

void heap_insert_max(Heap heap, Token key, float value) {
  uint32_t n = ++heap_size(heap);
  while (n > 1) {
    uint32_t p = n >> 1;
    if (heap[p].logp >= value) break; /*  */
    heap[n] = heap[p];
    n = p;
  }
  heap[n].token = key;
  heap[n].logp = value;
}

Hpair heap_delete_min(Heap heap) {
  assert(heap_size(heap) > 0);
  Hpair top = heap[1];
  Hpair bot = heap[heap_size(heap)];
  uint32_t end = --heap_size(heap);
  uint32_t p = 1;
  uint32_t n;
  while ((n = p << 1) <= end) {
    if ((n < end) && (heap[n+1].logp <= heap[n].logp)) n++; /*  */
    if (bot.logp <= heap[n].logp) break;		    /*  */
    heap[p] = heap[n];
    p = n;
  }
  heap[p] = bot;
  return top;
}

Hpair heap_delete_max(Heap heap) {
  assert(heap_size(heap) > 0);
  Hpair top = heap[1];
  Hpair bot = heap[heap_size(heap)];
  uint32_t end = --heap_size(heap);
  uint32_t p = 1;
  uint32_t n;
  while ((n = p << 1) <= end) {
    if ((n < end) && (heap[n+1].logp >= heap[n].logp)) n++; /*  */
    if (bot.logp >= heap[n].logp) break;		    /*  */
    heap[p] = heap[n];
    p = n;
  }
  heap[p] = bot;
  return top;
}

void heap_sort_max(Heap heap) {
  uint32_t size = heap_size(heap);
  for (uint32_t i = size; i > 1; i--) {
    heap[i] = heap_delete_min(heap);
  }
  heap_size(heap) = size;
}

void heap_sort_min(Heap heap) {
  uint32_t size = heap_size(heap);
  for (uint32_t i = size; i > 1; i--) {
    heap[i] = heap_delete_max(heap);
  }
  heap_size(heap) = size;
}
