struct Client {
    int sockfd;
    char *name;
};

struct node {
    struct node *next;
    struct node *prev;
    struct Client *client;
};

struct list {
    struct node *head;
    struct node *tail;
}; 

struct list* initL(void);
void append(struct list *l, struct Client *cl);
void deleteI(struct list *l, struct Client *cl);
void destroyL(struct list *l);