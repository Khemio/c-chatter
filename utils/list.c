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
    // Do error handling and messaging, chek for memory leaks
    struct node *curr = l->head;
    while (curr && curr->client->sockfd != cl->sockfd) {
        curr = curr->next;     
    }

    if (curr == NULL) return;

    if (l->head == curr) {
        l->head = curr->next;
        if (curr->next != NULL) curr->next->prev = NULL;

        free(curr);
        return;
    }

    if (l->tail == curr) {
        l->tail = curr->prev;
        if (curr->prev != NULL) curr->prev->next = NULL;;
        
        free(curr);
        return;
    }

    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    free(curr);
}

void destroyL(struct list *l) {
    struct node *curr;
    while (l->head) {
        curr = l->head; 
        l->head = l->head->next;
        free(curr);    
    }

    l->head = l->tail = NULL;
    free(l);
}