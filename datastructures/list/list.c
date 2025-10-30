#include "list.h"

#define ALLOC(type, action, extra_req, size, res)                             \
    begin                                                                     \
        type* alloced = action(extra_req, size);                              \
        if (!CHECK(ERROR, alloced != NULL, "alloc failed")) return ERR_ALLOC; \
        (res) = alloced;                                                      \
    end;

static err_t list_grow(list_t * const list)
{
    if (!CHECK(ERROR, list, "list is null")) return ERR_BAD_ARG;

    const size_t old_cap = list->list_capacity ? list->list_capacity : DEFAULT_LIST_SIZE;
    const size_t new_cap = old_cap * 2;

    ALLOC(list_elem_t, realloc, list->data, new_cap * sizeof(*list->data), list->data);
    ALLOC(size_t,      realloc, list->next, new_cap * sizeof(*list->next), list->next);
    ALLOC(size_t,      realloc, list->prev, new_cap * sizeof(*list->prev), list->prev);

    for (size_t i = old_cap; i + 1 < new_cap; ++i) {
        list->next[i] = i + 1;
        list->prev[i] = LIST_FREE;
        list->data[i] = 0;
    }
    list->next[new_cap - 1] = list->free_index;
    list->prev[new_cap - 1] = LIST_FREE;
    list->data[new_cap - 1] = 0;

    list->free_index     = old_cap;
    list->list_capacity  = new_cap;
    return OK;
}


static inline int idx_valid(const list_t* list, size_t i)
{
    return list && i < list->list_capacity;
}

static inline int idx_is_free(const list_t* list, size_t i)
{
    return i != 0 && list->prev[i] == LIST_FREE;
}

static err_t ensure_slot(list_t* list)
{
    if (list->free_index != 0) return OK;
    return list_grow(list);
}

static size_t pop_free(list_t* list)
{
    size_t i         = list->free_index;
    list->free_index = list->next[i];
    list->next[i]    = 0;
    list->prev[i]    = 0;
    list->list_size += 1;
    return i;
}

static void push_free(list_t* list, size_t i)
{
    list->data[i]    = 0;
    list->prev[i]    = LIST_FREE;
    list->next[i]    = list->free_index;
    list->free_index = i;
    list->list_size -= 1;
}

err_t list_ctor(list_t * const list)
{
    if (!CHECK(ERROR, list, "list is null")) return ERR_BAD_ARG;

    ALLOC(list_elem_t, calloc, DEFAULT_LIST_SIZE, sizeof(list_elem_t), list->data);
    ALLOC(size_t,      calloc, DEFAULT_LIST_SIZE, sizeof(size_t),      list->next);
    ALLOC(size_t,      calloc, DEFAULT_LIST_SIZE, sizeof(size_t),      list->prev);

    list->list_capacity  = DEFAULT_LIST_SIZE;

    list->next[0]   = 0; // head
    list->prev[0]   = 0; // tail
    list->list_size = 0;

    // Build free-list: 1 -> 2 -> ... -> N-1 -> 0
    for (size_t i = 1; i + 1 < list->list_capacity; i++)
    {
        list->next[i] = i + 1;
        list->prev[i] = LIST_FREE;
    }

    list->next[list->list_capacity - 1] = 0;
    list->prev[list->list_capacity - 1] = LIST_FREE;

    list->free_index = 1;
    return OK;
}

err_t list_dtor(list_t * const list)
{
    if (!list) return OK;
    free(list->data);
    free(list->next);
    free(list->prev);
    *list = (list_t){ 0 };
    return OK;
}

err_t get_elem(const list_t * const list, const size_t index, list_elem_t* elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range"))  return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "free"))return ERR_BAD_ARG;
    *elem = list->data[index];
    return OK;
}

err_t get_next(const list_t * const list, const size_t index, list_elem_t* elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range"))  return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "free"))return ERR_BAD_ARG;
    *elem = list->next[index];
    return OK;
}

err_t get_prev(const list_t * const list, const size_t index, list_elem_t* elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range"))  return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "free"))return ERR_BAD_ARG;
    *elem = list->prev[index];
    return OK;
}

err_t get_head(const list_t * const list, list_elem_t* elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    *elem = list->next[0];
    return OK; 
}

err_t get_tail(const list_t * const list, list_elem_t* elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    *elem = list->prev[0];
    return OK; 
}

err_t ins_elem_after(list_t * const list, const size_t index, list_elem_t elem)
{
    if (!CHECK(ERROR, list, "null")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range")) return ERR_BAD_ARG;
    if (index != 0 && !CHECK(ERROR, !idx_is_free(list, index), "free idx")) return ERR_BAD_ARG;

    if (ensure_slot(list) != OK) return ERR_ALLOC;
    size_t n      = pop_free(list);
    list->data[n] = elem;

    if (index == 0) {
        // push front
        const size_t head = list->next[0];
        const size_t tail = list->prev[0];
        if (head == 0) {
            list->next[n] = n;
            list->prev[n] = n;
            list->next[0] = n;
            list->prev[0] = n;
        } else {
            list->next[n]    = head;
            list->prev[n]    = tail;
            list->prev[head] = n;
            list->next[tail] = n;
            list->next[0]    = n;
        }
    } else {
        const size_t nxt  = list->next[index];
        list->next[index] = n;
        list->prev[n]     = index;
        list->next[n]     = nxt;
        list->prev[nxt]   = n;
        
        if (index == list->prev[0]) {
            list->prev[0] = n;
        }
    }

    return OK;
}

err_t ins_elem_before(list_t * const list, const size_t index, list_elem_t elem)
{
    if (!CHECK(ERROR, list, "null")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range")) return ERR_BAD_ARG;
    if (index != 0 && !CHECK(ERROR, !idx_is_free(list, index), "free idx")) return ERR_BAD_ARG;

    if (ensure_slot(list) != OK) return ERR_ALLOC;
    size_t n = pop_free(list);
    list->data[n] = elem;

    if (index == 0) {
        const size_t head = list->next[0];
        const size_t tail = list->prev[0];
        if (head == 0) {
            list->next[n] = n;
            list->prev[n] = n;
            list->next[0] = n;
            list->prev[0] = n;
        } else {
            list->next[n]   = head;
            list->prev[n]   = tail;
            list->next[tail]= n;
            list->prev[head]= n;
            list->prev[0]   = n;
        }    
    } else {
        const size_t prv = list->prev[index];
        list->prev[index] = n;
        list->prev[n]     = prv;
        list->next[n]     = index;
        list->next[prv]   = n;

        if (index == list->next[0]) {
            list->next[0] = n;
        }    
    }
    return OK;
}

err_t del_elem(list_t * const list, const size_t index)
{
    if (!CHECK(ERROR, list, "null")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index) && index != 0, "range")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "already free")) return ERR_BAD_ARG;

    const size_t was_size = list->list_size;
    const size_t prv = list->prev[index];
    const size_t nxt = list->next[index];

    if (was_size == 1) {
        list->next[0] = 0;
        list->prev[0] = 0;
    } else {
        list->next[prv] = nxt;
        list->prev[nxt] = prv;

        if (index == list->next[0]) list->next[0] = nxt;
        if (index == list->prev[0]) list->prev[0] = prv;
    }

    push_free(list, index);
    return OK;
}

err_t push_front(list_t * const list, list_elem_t elem, size_t * real_index)
{
    if (!CHECK(ERROR, list && real_index, "bad args")) return ERR_BAD_ARG;
    const err_t rc = ins_elem_after(list, 0, elem);
    if (rc != OK) return rc;
    *real_index = list->next[0];
    return OK;
}

err_t push_back(list_t * const list, list_elem_t elem, size_t * real_index)
{
    if (!CHECK(ERROR, list && real_index, "bad args")) return ERR_BAD_ARG;
    const err_t rc = ins_elem_before(list, 0, elem);
    if (rc != OK) return rc;
    *real_index = list->prev[0];
    return OK;
}
