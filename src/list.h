#ifndef list_h
#define list_h

// Singly-linked-list node.
typedef struct node_t
{
    int val;
    struct node_t *next;
} node_t;

// Singly-linked list.
typedef struct list_t
{
    node_t* head;
} list_t;

// Allocates a new singly-linked list. Returns NULL if the list cannot be
// allocated.
list_t* list_init();

// Frees the linked-list resources.
void list_dispose(list_t* list);

// Adds a value to the list. Returns 0 on success, -1 if the node couldn't be
// allocated.
int list_add(list_t* list, int val);

// Removes the first occurrence of val from the list. Returns 0 if a value was
// removed, -1 if the value wasn't found in the list.
int list_remove(list_t* list, int val);

#endif
