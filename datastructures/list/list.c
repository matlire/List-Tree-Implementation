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

void list_dump(const list_t *list, const char *title, const char *html_file);

static void dump_with_title(const list_t *list, const char *title_str)
{
    char html_file[512];
    snprintf(html_file, sizeof(html_file), "gdump%s.html", title_str ? title_str : "");
    list_dump(list, title_str, html_file);
}

#define CHECKD(condition, title_str)                                              \
    ((condition) ? 1                                                              \
        : (                                                                       \
            dump_with_title(list, (title_str)),                                   \
            log_printf(ERROR,                                                     \
                "[File %s at line %d at %s] %s",                                  \
                       __FILE__, __LINE__, __PRETTY_FUNCTION__, (title_str)), 0))

err_t list_verify(const list_t * const list)
{
    if (!CHECKD(list != NULL, "verify: list is null")) return ERR_BAD_ARG;
    const size_t cap = list->list_capacity;
    if (!CHECKD(cap > 0, "verify: capacity is zero")) return ERR_CORRUPT;

    const size_t head = list->next[0];
    const size_t tail = list->prev[0];

    if (list->list_size == 0)
        return CHECKD(head == 0 && tail == 0, 
                      "verify: empty but head/tail not zero") ? OK : ERR_CORRUPT;

    if (!CHECKD(idx_valid(list, head) && idx_valid(list, tail), 
                "verify: head/tail OOB")) return ERR_CORRUPT;
    if (!CHECKD(!idx_is_free(list, head) && !idx_is_free(list, tail), 
                "verify: head/tail marked free")) return ERR_CORRUPT;
    if (!CHECKD(list->prev[head] == tail, 
                "verify: head->prev != tail")) return ERR_CORRUPT;
    if (!CHECKD(list->next[tail] == head, 
                "verify: tail->next != head")) return ERR_CORRUPT;

    unsigned char *used = (unsigned char*)calloc(cap, 1);
    if (!CHECKD(used != NULL, 
                "verify: alloc used failed")) return ERR_ALLOC;

    size_t counted = 0, cur = head;
    for (size_t steps = 0; steps < cap; ++steps) {
        if (!CHECKD(idx_valid(list, cur), 
                    "verify: cur OOB")) 
                        { free(used); return ERR_CORRUPT; }
        if (!CHECKD(!idx_is_free(list, cur), 
                    "verify: used node marked free")) 
                        { free(used); return ERR_CORRUPT; }
        if (!CHECKD(!used[cur], 
                    "verify: revisit used node")) 
                        { free(used); return ERR_CORRUPT; }
        used[cur] = 1;
        counted++;

        const size_t nxt = list->next[cur];
        const size_t prv = list->prev[cur];

        if (!CHECKD(idx_valid(list, prv) && !idx_is_free(list, prv), 
                    "verify: prev invalid")) 
                        { free(used); return ERR_CORRUPT; }
        if (!CHECKD(idx_valid(list, nxt) && !idx_is_free(list, nxt), 
                    "verify: next invalid")) 
                        { free(used); return ERR_CORRUPT; }
        if (!CHECKD(list->next[prv] == cur, 
                    "verify: prev->next mismatch")) 
                        { free(used); return ERR_CORRUPT; }
        if (!CHECKD(list->prev[nxt] == cur, 
                    "verify: next->prev mismatch")) 
                        { free(used); return ERR_CORRUPT; }

        if (cur == tail) {
            if (!CHECKD(nxt == head, 
                        "verify: tail doesn't link to head")) { free(used); return ERR_CORRUPT; }
            break;
        }

        cur = nxt;
    }

    if (!CHECKD(counted == list->list_size, 
                "verify: size mismatch")) 
                    { free(used); return ERR_CORRUPT; }

    unsigned char *seen_free = (unsigned char*)calloc(cap, 1);
    if (!CHECKD(seen_free != NULL, 
                "verify: alloc free-set failed")) 
                    { free(used); return ERR_ALLOC; }

    size_t free_cnt = 0, f = list->free_index;
    for (size_t steps = 0; f != 0; ++steps) {
        if (!CHECKD(steps < cap, 
                    "verify: cycle in free chain")) 
                        { free(used); free(seen_free); return ERR_CORRUPT; }
        if (!CHECKD(idx_valid(list, f), 
                    "verify: free OOB")) 
                        { free(used); free(seen_free); return ERR_CORRUPT; }
        if (!CHECKD(idx_is_free(list, f), 
                    "verify: node in free chain not free")) 
                        { free(used); free(seen_free); return ERR_CORRUPT; }
        if (!CHECKD(!seen_free[f], 
                    "verify: revisit free node")) 
                        { free(used); free(seen_free); return ERR_CORRUPT; }
        if (!CHECKD(!used[f], 
                    "verify: free overlaps used")) 
                        { free(used); free(seen_free); return ERR_CORRUPT; }
        seen_free[f] = 1;
        free_cnt++;
        f = list->next[f];
    }

    if (!CHECKD(counted + free_cnt == (cap - 1), 
                "verify: partition mismatch")) 
                    { free(used); free(seen_free); return ERR_CORRUPT; }

    free(used); free(seen_free);
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

err_t get_next(const list_t * const list, const size_t index, size_t * const elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range"))  return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "free"))return ERR_BAD_ARG;
    *elem = list->next[index];
    return OK;
}

err_t get_prev(const list_t * const list, const size_t index, size_t * const elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    if (!CHECK(ERROR, idx_valid(list, index), "range"))  return ERR_BAD_ARG;
    if (!CHECK(ERROR, !idx_is_free(list, index), "free"))return ERR_BAD_ARG;
    *elem = list->prev[index];
    return OK;
}

err_t get_head(const list_t * const list, size_t * const elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    *elem = list->next[0];
    return OK; 
}

err_t get_tail(const list_t * const list, size_t * const elem)
{
    if (!CHECK(ERROR, list && elem, "bad args"))         return ERR_BAD_ARG;
    *elem = list->prev[0];
    return OK; 
}

static inline void link_between(list_t* L, size_t left, size_t mid, size_t right) 
{
    L->next[left] = mid;
    L->prev[mid]  = left;
    L->next[mid]  = right;
    L->prev[right]= mid;
}

static inline void unlink_node(list_t* L, size_t i) 
{
    size_t p = L->prev[i];
    size_t n = L->next[i];
    L->next[p] = n;
    L->prev[n] = p;
    if (i == L->next[0]) L->next[0] = n;
    if (i == L->prev[0]) L->prev[0] = p;
}

#define INS_MACROS                                                                              \
    if (!CHECK(ERROR, list, "null")) return ERR_BAD_ARG;                                        \
    if (!CHECK(ERROR, idx_valid(list, index), "range")) return ERR_BAD_ARG;                     \
    if (index != 0 && !CHECK(ERROR, !idx_is_free(list, index), "free idx")) return ERR_BAD_ARG; \
    if (ensure_slot(list) != OK) return ERR_ALLOC;                                              \
 
err_t ins_elem_after(list_t * const list, const size_t index, const list_elem_t elem)
{
    INS_MACROS;
    size_t n      = pop_free(list);
    list->data[n] = elem;

    
    if (list->list_size == 1)
    {
        list->next[0] = list->prev[0] = n;
        list->next[n] = list->prev[n] = n;
        return OK;
    }
    size_t left  = (index == 0) ? list->prev[0] : index;
    size_t right = (index == 0) ? list->next[0] : list->next[index];
    link_between(list, left, n, right);

    if (index == 0) list->next[0] = n;
    if (index == list->prev[0]) list->prev[0] = n;

    return OK;
}

err_t ins_elem_before(list_t * const list, const size_t index, const list_elem_t elem)
{
    INS_MACROS;

    size_t before = (index == 0) ? list->prev[0] : list->prev[index];
    return ins_elem_after(list, before, elem);
}

#undef INS_MACROS

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

err_t push_front(list_t * const list, list_elem_t elem, size_t * const real_index)
{
    if (!CHECK(ERROR, list && real_index, "bad args")) return ERR_BAD_ARG;
    const err_t rc = ins_elem_after(list, 0, elem);
    if (rc != OK) return rc;
    *real_index = list->next[0];
    return OK;
}

err_t push_back(list_t * const list, list_elem_t elem, size_t * const real_index)
{
    if (!CHECK(ERROR, list && real_index, "bad args")) return ERR_BAD_ARG;
    const err_t rc = ins_elem_before(list, 0, elem);
    if (rc != OK) return rc;
    *real_index = list->prev[0];
    return OK;
}

#define LIST_LIN_FREE_MACROS \
    free(list->data);        \
    free(list->next);        \
    free(list->prev);        \

#define LIST_LIN_MACROS   \
    LIST_LIN_FREE_MACROS; \
    list->data = ndata;   \
    list->next = nnext;   \
    list->prev = nprev;   \
    list->list_capacity = newc;

err_t list_linearize(list_t * const list)
{
    if (!list) return ERR_BAD_ARG;

    const size_t size = list->list_size;
    const size_t minc = DEFAULT_LIST_SIZE;
    const size_t newc = (size + 1 < minc) ? minc : (size + 1);

    list_elem_t *ndata = (list_elem_t*)calloc(newc, sizeof(*ndata));
    size_t      *nnext = (size_t*)     calloc(newc, sizeof(*nnext));
    size_t      *nprev = (size_t*)     calloc(newc, sizeof(*nprev));

    if (!ndata || !nnext || !nprev) { LIST_LIN_FREE_MACROS; return ERR_ALLOC; }

    if (size > 0) 
    {
        size_t cur = list->next[0];

        for (size_t pos = 1; pos <= size; ++pos) 
        {
            ndata[pos] = list->data[cur];
            cur = list->next[cur];
        }

        for (size_t pos = 1; pos <= size; ++pos) 
        {
            nnext[pos] = (pos == size) ? 1 : (pos + 1);
            nprev[pos] = (pos == 1)    ? size : (pos - 1);
        }

        nnext[0] = 1;
        nprev[0] = size;
    } else {
        nnext[0] = 0;
        nprev[0] = 0;
    }

    if (newc > size + 1) 
    {
        const size_t start = size + 1;
        for (size_t i = start; i + 1 < newc; ++i) 
        {
            nnext[i] = i + 1;
            nprev[i] = LIST_FREE;
        }
        nnext[newc - 1] = 0;
        nprev[newc - 1] = LIST_FREE;
        list->free_index = start;
    } else {
        list->free_index = 0;
    }

    LIST_LIN_MACROS;

    return OK;
}

#undef LIST_LIN_MACROS
#undef LIST_LIN_FREE_MACROS
