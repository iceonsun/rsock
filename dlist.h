//
// Created on 10/30/17.
//

#ifndef SOCKNM_DLIST_H
#define SOCKNM_DLIST_H

#include "ktype.h"

#ifdef __cplusplus
extern "C" {
#endif

// a_dlist should always be head of dlist
#define FOR_EACH(it, nxt, list_head) \
    for ((it) = (list_head)->next, (nxt) = (it)->next; (it) != (list_head); (it) = (nxt), (nxt) = (nxt)->next)

typedef struct dlist_s {
    struct dlist_s *prev;
    struct dlist_s *next;
} dlist, dlnode;

// que operations
void dlist_init(dlist *list);

void dlist_add_tail(dlist *list, dlnode *node);

void dlist_add_after(dlnode *node1, dlnode *node2);

void dlist_remove_node(dlnode *node);

IINT8 list_not_empty(dlist *list);

#ifdef __cplusplus
}
#endif
#endif //SOCKNM_DLIST_H
