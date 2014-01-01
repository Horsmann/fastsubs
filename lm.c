#include <stdlib.h>
#include <assert.h>
#include "dlib.h"
#include "ngram.h"
#include "heap.h"
#include "lm.h"

struct _LMS {
  uint32_t order;
  uint32_t nvocab;
  darr_t phash;
  darr_t whash;
  darr_t pheap;
  darr_t wheap;
};

/* Define ngram-logp/logb hash */
typedef struct NFS { Ngram key; float val; } *NF; // elements are ngram-float pairs
#define _nf_init(ng) ((struct NFS) { (ng), 0 }) // ng must be already allocated
D_HASH(_nf_, struct NFS, Ngram, ngram_equal, ngram_hash, d_keyof, _nf_init, d_keyisnull, d_keymknull)
#define _logP(lm, ng) (_nf_get(((lm)->phash),(ng),1)->val)
#define _logB(lm, ng) (_nf_get(((lm)->whash),(ng),1)->val)

/* Define ngram-tokenHeap hash */
typedef struct NHS { Ngram key; Heap val; } *NH; // elements are ngram-heap pairs
#define _nh_init(ng) ((struct NHS) { ngram_dup(ng), NULL }) // no need for this dup, fix using ptr-hole
D_HASH(_nh_, struct NHS, Ngram, ngram_equal, ngram_hash, d_keyof, _nh_init, d_keyisnull, d_keymknull)

static void lm_read(LM lm, const char *lmfile);
static void lm_heap_init(LM lm);

LM lm_init(const char *lmfile) {
  LM lm = _d_malloc(sizeof(struct _LMS));
  lm->phash = _nf_new(0);
  lm->whash = _nf_new(0);
  lm->pheap = _nh_new(0);
  lm->wheap = _nh_new(0);
  lm->order = 0;
  lm->nvocab = 0;
  msg("lm_init: reading %s...", lmfile);
  lm_read(lm, lmfile);
  msg("lm_init: initializing heaps...");
  lm_heap_init(lm);
  msg("lm_init done: logP=%zux(%zu/%zu) logB=%zux(%zu/%zu) toks=%zu", 
      sizeof(struct NFS), len(lm->phash), cap(lm->phash), 
      sizeof(struct NFS), len(lm->whash), cap(lm->whash));
  return lm;
}

void lm_free(LM lm) {
  _nf_free(lm->phash);
  _nf_free(lm->whash);
  _nh_free(lm->pheap);
  _nh_free(lm->wheap);
  _d_free(lm);
}

float lm_logP(LM lm, Ngram ng) {
  NF p = _nf_get(lm->phash, ng, false);
  return (p == NULL ? SRILM_LOG0 : p->val);
}

float lm_logB(LM lm, Ngram ng) {
  NF p = _nf_get(lm->whash, ng, false);
  return (p == NULL ? 0 : p->val);
}

Heap lm_logP_heap(LM lm, Ngram ng) {
  NH p = _nh_get(lm->pheap, ng, false);
  return (p == NULL ? NULL : p->val);
}

Heap lm_logB_heap(LM lm, Ngram ng) {
  NH p = _nh_get(lm->wheap, ng, false);
  return (p == NULL ? NULL : p->val);
}

uint32_t lm_order(LM lm) {
  return lm->order;
}

uint32_t lm_nvocab(LM lm) {
  return lm->nvocab;
}

static void lm_read(LM lm, const char *lmfile) {
  forline(str, lmfile) {
    errno = 0;
    if (*str == '\n' || *str == '\\' || *str == 'n') continue;
    char *tok[] = { NULL, NULL, NULL };
    size_t ntok = split(str, '\t', tok, 3);
    assert(ntok >= 2);
    float f = strtof(tok[0], NULL);
    assert(errno == 0);
    if (ntok == 2) {
      size_t len = strlen(tok[1]);
      if (tok[1][len-1] == '\n') tok[1][len-1] = '\0';
    }
    Ngram ng = ngram_from_string(tok[1]);
    if (ngram_size(ng) > lm->order) 
      lm->order = ngram_size(ng);
    for (int i = ngram_size(ng); i > 0; i--) {
      if (ng[i] > lm->nvocab) lm->nvocab = ng[i];
    }
    _logP(lm, ng) = f;
    if (ntok == 3) {
      f = (float) strtof(tok[2], NULL);
      assert(errno == 0);
      _logB(lm, ng) = f;
    }
  }
}

static void lm_heap_init(LM lm) {
  msg("count logP");
  forhash (NF, nf, lm->phash, d_keyisnull) {
    Ngram ng = nf->key;
    for (int i = ngram_size(ng); i > 0; i--) {
      Token ng_i = ng[i];
      ng[i] = NULLTOKEN;
      NH nh = _nh_get(lm->pheap, ng, true);
      nh->val = (Heap)(1 + ((size_t)(nh->val)));
      ng[i] = ng_i;
    }
  }

  msg("alloc logP_heap");
  forhash (NH, nh, lm->pheap, d_keyisnull) {
    size_t n = ((size_t) nh->val);
    Heap heap = dalloc(sizeof(Hpair) * (1 + n));
    heap_size(heap) = 0;
    nh->val = heap;
  }

  msg("insert logP");
  forhash (NF, nf, lm->phash, d_keyisnull) {
    Ngram ng = nf->key;
    float f = nf->val;
    for (int i = ngram_size(ng); i > 0; i--) {
      Token ng_i = ng[i];
      ng[i] = NULLTOKEN;
      NH nh = _nh_get(lm->pheap, ng, false);
      ng[i] = ng_i;
      heap_insert_min(nh->val, ng_i, f);
    }
  }

  size_t hpair_cnt = 0;
  msg("sort logP_heap");
  forhash (NH, nh, lm->pheap, d_keyisnull) {
    Heap heap = nh->val;
    assert(heap != NULL && heap_size(heap) > 0);
    heap_sort_max(heap);
    hpair_cnt += heap_size(heap);
  }

  msg("lm_heap_init done: NHS=%zu logPheap=%zu/%zu logWheap=%zu/%zu hpairs=%zu", 
      sizeof(struct NHS), len(lm->pheap), cap(lm->pheap), len(lm->wheap), cap(lm->wheap), hpair_cnt);
}

