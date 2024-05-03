#include "includes.h"
#include "list.h"

int main(void) {
    struct list *l;
    printf("hi\n");
    l = initL();

    printf("hello\n");
    struct Client cln1 = {.sockfd = 1, .name = "khemio"};
    struct Client cln2 = {.sockfd = 2, .name = "inerizal"};
    struct Client cln3 = {.sockfd = 3, .name = "maya"};
    struct Client cln4 = {.sockfd = 4, .name = "testy"};

    printf("here we go\n");
    append(l, &cln1);
    // printf("1\n");
    append(l, &cln2);
    // printf("2\n");
    append(l, &cln3);
    append(l, &cln4);

    // deleteI(l, &cln1);
    deleteI(l, &cln2);
    printf("3\n");
    deleteI(l, &cln3);
    printf("4\n");
    deleteI(l, &cln4);

    return 0;

}