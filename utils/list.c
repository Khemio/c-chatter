#include "list.h"
#include "includes.h"

struct list* initL(void) {
    struct list *l;
    l = malloc(sizeof(struct list));

    l->head = NULL;
    l->tail = NULL;

    return l;
}

void append(struct list *l, struct Client *cl) {
    struct node *n;
    n = malloc(sizeof(struct node));

    n->client = cl;
    n->next = NULL;
    n->prev = NULL;

    if (l->head == NULL) {
        l->head = l->tail = n;
    } else {
        n->prev = l->tail;
        l->tail->next = n;
        l->tail = n;
    }
}

void deleteI(struct list *l, struct Client *cl) {
    // Do error handling and messaging
    struct node *curr = l->head;
    while (curr && curr->client->sockfd != cl->sockfd) {
        // if (curr->next == NULL) return;

        curr = curr->next;     
    }

    if (curr == NULL) return;

    if (l->head == curr) {
        l->head = curr->next;
        curr->next->prev = NULL;
    }

    if (l->tail == curr) {
        l->tail = curr->prev;
        curr->prev->next = NULL;
    }

    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    free(curr);
}

void destroyL(struct list l) {

}