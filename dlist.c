//
// Created on 10/30/17.

#include "dlist.h"

IINT8 list_not_empty(dlist *list) {
    return list != list->next;
}

void dlist_add_tail(dlist *list, dlnode *node) {
    if (!list || !node) {
        return;
    }

    dlist_add_after(list->prev, node);
}

void dlist_add_after(dlnode *node1, dlnode *node2) {
    if (!node1 || !node2) {
        return;
    }

    node2->prev = node1;
    node2->next = node1->next;
    node1->next->prev = node2;
    node1->next = node2;
}

void dlist_remove_node(dlnode *node) {
    if (!node) {
        return;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = 0;
    node->prev = 0;
}

void dlist_init(dlist *list) {
    list->prev = list;
    list->next = list;
}
