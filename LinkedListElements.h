typedef struct list_node
{
   struct list_node *next; //8 byte because pointer
   
} list_node;


typedef struct string_item
{
    struct list_node node;  //8 byte
    const char *string;     //8 byte because pointer
}string_item;


//DOUBLE LINKED LIST ELEMENTS 
typedef struct list_double_node
{
    struct list_double_node *prev;
    struct list_double_node *next;
}list_double_node;

typedef struct string_item_double
{
    struct list_double_node node;  //8 byte
    const char *string;     //8 byte because pointer
}string_item_double;

void invert_LinkedList(list_node** head)
{
    if (!head)
        return;

    list_node* curr = *head;
    list_node* prev = NULL;
    list_node* next = NULL;

    
   
    while (curr)
    {
        //printf("INVERTING %p \n",curr);
        // Store next
        next = curr->next;
        // Reverse current node's pointer
        curr->next = prev;
        // Move pointers one position ahead.
        prev = curr;
        curr = next;
        //printf("INVERTED %p \n",curr);

    }
    //printf("NEW HEAD %p \n",prev);
    *head = prev;
    //printf("NEXT  %p    \n",prev->next);
    // printf("INVERTED \n");
    // curr = *head;
    // while(curr)
    // {
    //     printf(" %p \n",curr);
    //     curr = curr->next;
    // }
    // printf("\n");
    //invert(next, curr, head);
    return;
}
struct list_node *list_pop(struct list_node **head)
{
    struct list_node *current_head = *head;
    if (!current_head)
    {
        return NULL;
    }
    *head = (*head)->next;
    current_head->next = NULL;
    return current_head;
}

struct list_node *list_get_tail(struct list_node **head)
{
    struct list_node *curr_node = *head;
    struct list_node *last_node = NULL;
    while (curr_node)
    {
        last_node = curr_node;
        curr_node = curr_node->next;
    }
    return last_node;
}

struct list_node *list_append(struct list_node **head, struct list_node *item)
{
    struct list_node *tail = list_get_tail(head);
    if (!tail)
    {
        *head = item;
    }
    else
    {
        tail->next = item;
    }
    item->next = NULL;
    return item;
}


void PrintList(string_item* my_linked_list)
{
    string_item* string_item = my_linked_list;
    while (string_item)
    {
        printf("%s\n", string_item->string);
        string_item = (struct string_item *)string_item->node.next;
    }
}