#ifndef LIST_H
#define LIST_H

#include "../../libs/logging/logging.h"
#include "../../libs/types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

typedef int list_elem_t;

typedef struct
{
    list_elem_t* data;
    size_t*      next;
    size_t*      prev;

    size_t       list_capacity;
    size_t       list_size;

    size_t       free_index;
} list_t;

#define DEFAULT_LIST_SIZE 4
#define LIST_FREE ((size_t)-1)

#define CREATE_LIST(list_name) \
    list_t list_name = { 0 };  \
    list_ctor(&(list_name))

err_t list_ctor(list_t * const list);
err_t list_dtor(list_t * const list);

err_t list_verify(const list_t * const list);

err_t get_elem       (const list_t * const list, const size_t index, list_elem_t * const elem);
err_t ins_elem_before(      list_t * const list, const size_t index, const list_elem_t elem);
err_t ins_elem_after (      list_t * const list, const size_t index, const list_elem_t elem);
err_t del_elem       (      list_t * const list, const size_t index);

err_t get_next(const list_t * const list, const size_t index, size_t * const elem);
err_t get_prev(const list_t * const list, const size_t index, size_t * const elem);

err_t get_head(const list_t * const list, size_t * const elem);
err_t get_tail(const list_t * const list, size_t * const elem);

err_t push_front(list_t * const list, list_elem_t elem, size_t * const real_index);
err_t push_back (list_t * const list, list_elem_t elem, size_t * const real_index);

err_t list_linearize(list_t * const list);

#endif
