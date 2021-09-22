

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ll_node {
    int32_t id;
    int32_t data;
    struct ll_node* next;
} ll_node_t;

typedef struct {
    void (*print)(ll_node_t*);
    ll_node_t* (*new)(int32_t, int32_t);
    int (*append)(ll_node_t**, ll_node_t*);
    size_t (*length)(ll_node_t*);
    int (*insert)(ll_node_t**, ll_node_t*, size_t);
    int (*remove)(ll_node_t** head, size_t idx);
} ll_func_t;

void ll_print(ll_node_t* head) {
    ll_node_t* n = head;
    while(n != NULL) {
        printf("Node (%p) %d with data %d\n", n, n->id, n->data);
        n = n->next;
    }
}
void ll_print_a(ll_node_t* head);

void* malloc_a(size_t nbytes);
ll_node_t* ll_new(int32_t id, int32_t data) {
    ll_node_t* n = (ll_node_t*)malloc(sizeof(ll_node_t));
    n->id = id;
    n->data = data;
    n->next = NULL;
    return n;
}
ll_node_t* ll_new_a(int32_t id, int32_t data);

int ll_append(ll_node_t** head, ll_node_t* newNode) {
    if(head == NULL)
        return 0;

    if(*head == NULL) {
        *head = newNode;
        return 1;
    }

    ll_node_t* last = *head;
    while(last->next != NULL) {
        last = last->next;
    }
    last->next = newNode;
    return 1;
}
int ll_append_a(ll_node_t** head, ll_node_t* newNode);

size_t ll_length(ll_node_t* head) {
    size_t len = 0;
    while(head != NULL) {
        len++;
        head = head->next;
    }
    return len;
}
size_t ll_length_a(ll_node_t* head);

int ll_insert(ll_node_t** head, ll_node_t* newNode, size_t idx) {
    if(head == NULL)
        return 0;
    if(idx == 0) {
        ll_node_t* oldHead = *head;
        *head = newNode;
        if(oldHead != NULL) {
            newNode->next = oldHead;
        }
        return 1;
    }

    ll_node_t* insertPnt = *head;
    size_t i = 1;
    while(insertPnt->next != NULL && i < idx) {
        insertPnt = insertPnt->next;
        i++;
    }
    if(i < idx)
        return 0;
    newNode->next = insertPnt->next;
    insertPnt->next = newNode;

    return 1;
}
int ll_insert_a(ll_node_t** head, ll_node_t* newNode, size_t idx);

// note this is deleting without freeing memory
// THIS IS A MEMORY LEAK
// to keep things simpler, ignoring for now
int ll_remove(ll_node_t** head, size_t idx) {

    if(head == NULL)
        return 0;
    if(idx == 0) {
        if(*head == NULL)
            return 0;
        *head = (*head)->next;
        return 1;
    }

    ll_node_t* beforeDelete = *head;
    size_t i = 1;
    while(beforeDelete->next != NULL && i < idx) {
        beforeDelete = beforeDelete->next;
        i++;
    }
    if(i < idx)
        return 0;
    // deletining deletePnt->next
    beforeDelete->next = beforeDelete->next->next;

    return 1;
}
int ll_remove_a(ll_node_t** head, size_t idx);

void runTestCase(char* filename, ll_func_t* functions, ll_node_t** head) {
    if(head == NULL)
        return;
    FILE* fp = fopen(filename, "r");
    if(!fp)
        return;

    char func, c;
    char buf[40];
    int32_t id, data;
    size_t len, idx;
    while(fscanf(fp, "%c", &func) == 1) {
        switch(func) {
            case 'p': {
                fscanf(fp, "%39[^\n]\n", buf);
                if(strlen(buf) > 1)
                    printf("%s\n", buf + 1); // skip first char
                functions->print(*head);
                break;
            }
            case 'a': {
                fscanf(fp, "%d %d\n", &id, &data);
                functions->append(head, functions->new(id, data));
                break;
            }
            case 'l': {
                fscanf(fp, "\n");
                len = functions->length(*head);
                printf("Length: %zu\n", len);
                break;
            }
            case 'i': {
                fscanf(fp, "%d %d %zu\n", &id, &data, &idx);
                functions->insert(head, functions->new(id, data), idx);
                break;
            }
            case 'r': {
                fscanf(fp, "%zu\n", &idx);
                functions->remove(head, idx);
                break;
            }
            default: {
                // consume
                while((c = fgetc(fp)) != '\n')
                    ;
            }
        }
    }
    fclose(fp);
}

int main() {
    ll_node_t* c_head = NULL;
    ll_node_t* a_head = NULL;
    ll_func_t c_funcs = {.print = ll_print,
                         .new = ll_new,
                         .append = ll_append,
                         .length = ll_length,
                         .insert = ll_insert,
                         .remove = ll_remove};
    ll_func_t a_funcs = {.print = ll_print_a,
                         .new = ll_new_a,
                         .append = ll_append_a,
                         .length = ll_length_a,
                         .insert = ll_insert_a,
                         .remove = ll_remove_a};
    printf("C\n");
    runTestCase("test.txt", &c_funcs, &c_head);
    printf("A\n");
    runTestCase("test.txt", &a_funcs, &a_head);
}
