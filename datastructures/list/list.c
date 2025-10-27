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

    const size_t old_cap = list->list_size ? list->list_size : DEFAULT_LIST_SIZE;
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

    list->free_index = old_cap;
    list->list_size  = new_cap;
    return OK;
}


static inline int idx_valid(const list_t* list, size_t i)
{
    return list && i < list->list_size;
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
    list->next[i] = 0;
    list->prev[i] = 0;
    return i;
}

static void push_free(list_t* list, size_t i)
{
    list->data[i] = 0;
    list->prev[i] = LIST_FREE;
    list->next[i] = list->free_index;
    list->free_index = i;
}

err_t list_ctor(list_t * const list)
{
    if (!CHECK(ERROR, list, "list is null")) return ERR_BAD_ARG;

    ALLOC(list_elem_t, calloc, DEFAULT_LIST_SIZE, sizeof(list_elem_t), list->data);
    ALLOC(size_t,      calloc, DEFAULT_LIST_SIZE, sizeof(size_t),      list->next);
    ALLOC(size_t,      calloc, DEFAULT_LIST_SIZE, sizeof(size_t),      list->prev);

    list->list_size  = DEFAULT_LIST_SIZE;

    list->next[0] = 0; // head
    list->prev[0] = 0; // tail

    // Build free-list: 1 -> 2 -> ... -> N-1 -> 0
    for (size_t i = 1; i + 1 < list->list_size; i++)
    {
        list->next[i] = i + 1;
        list->prev[i] = LIST_FREE;
    }

    list->next[list->list_size - 1] = 0;
    list->prev[list->list_size - 1] = LIST_FREE;

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
        size_t old_head = list->next[0];
        list->next[0]   = n;

        list->prev[n]   = 0;
        list->next[n]   = old_head;

        if (old_head) list->prev[old_head] = n;
        else          list->prev[0] = n; // list was empty -> new tail too
    } else {
        size_t nxt = list->next[index];

        list->next[index] = n;
        list->prev[n]     = index;
        list->next[n]     = nxt;

        if (nxt) list->prev[nxt] = n;
        else     list->prev[0]   = n; // appended to tail
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
        // push back
        size_t tail = list->prev[0];

        list->next[n] = 0;
        list->prev[n] = tail;

        if (tail)  list->next[tail] = n;
        else       list->next[0]    = n; // list was empty -> new head
        list->prev[0] = n;
    } else {
        size_t prv = list->prev[index];

        list->prev[index] = n;
        list->prev[n] = prv;
        list->next[n] = index;

        if (prv)  list->next[prv] = n;
        else      list->next[0]   = n; // new head
    }
    return OK;
}

err_t del_elem(list_t * const list, const size_t index)
{
    if (!CHECK(ERROR, list, "null")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index) && index != 0, "range")) return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "already free")) return ERR_BAD_ARG;

    size_t prv = list->prev[index];
    size_t nxt = list->next[index];

    if (prv) list->next[prv] = nxt; else list->next[0] = nxt; // removed head?
    if (nxt) list->prev[nxt] = prv; else list->prev[0] = prv; // removed tail?

    push_free(list, index);
    return OK;
}

