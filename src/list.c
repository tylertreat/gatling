#include <stdio.h>
#include <stdlib.h>

#include "list.h"


// Allocates a new singly-linked list. Returns NULL if the list cannot be
// allocated.
list_t* list_init()
{
    list_t* list = malloc(sizeof(list_t));
    if (list == NULL)
    {
        perror("Failed to allocate list");
        return NULL;
    }

    list->head = NULL;
    return list;
}

// Frees the linked-list resources.
void list_dispose(list_t* list)
{
    node_t* curr = list->head;
    while (curr != NULL)
    {
        node_t* next = curr->next;
        free(curr);
        curr = next;
    }
    free(list);
    list = NULL;
}

// Adds a value to the list. Returns 0 on success, -1 if the node couldn't be
// allocated.
int list_add(list_t* list, int val)
{
    node_t* node = malloc(sizeof(node_t));
    if (node == NULL)
    {
        perror("Failed to allocate node");
        return -1;
    }

    node->val = val;
    node->next = NULL;

    if (list->head == NULL)
    {
        list->head = node;
    }
    else
    {
        node->next = list->head;
        list->head = node;
    }

    return 0;
}

// Removes the first occurrence of val from the list. Returns 0 if a value was
// removed, -1 if the value wasn't found in the list.
int list_remove(list_t* list, int val)
{
    node_t* curr = list->head;
    if (curr != NULL && curr->val == val)
    {
        list->head = list->head->next;
        free(curr);
        return 0;
    }

    while (curr != NULL)
    {
        if (curr->next->val == val)
        {
            node_t* remove = curr->next;
            curr->next = curr->next->next;
            free(remove);
            return 0;
        }

        curr = curr->next;
    }
    
    return -1;
}

