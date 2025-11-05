#include "tree.h"

err_t node_ctor(node_t * const node)
{
    if (!CHECK(ERROR, node != NULL, "node_ctor: node is NULL"))
        return ERR_BAD_ARG;

    node->data  = 0;
    node->left  = NULL;
    node->right = NULL;
    return OK;
}

err_t node_dtor(node_t * node)
{
    if (node) free(node);
    return OK;
}

err_t tree_ctor(tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_ctor: tree is NULL"))
        return ERR_BAD_ARG;

    tree->nodes_amount = 0;
    tree->root = NULL;
    return OK;
}

err_t tree_dtor(tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_dtor: tree is NULL"))
        return ERR_BAD_ARG;

    (void)tree_clear(tree);
    tree->root = NULL;
    tree->nodes_amount = 0;
    return OK;
}

err_t tree_verify(const tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_verify: tree is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR,
               !(tree->nodes_amount > 0 && tree->root == NULL),
               "tree_verify: nodes_amount=%zu but root is NULL",
               (size_t)tree->nodes_amount))
        return ERR_CORRUPT;

    return OK;
}

err_t tree_print_node(const node_t * const node, size_t iter)
{
    if (!CHECK(ERROR, node != NULL, "tree_print_node: node is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, iter <= MAX_RECURSION_LIMIT,
               "tree_print_node: recursion limit exceeded (%zu > %zu)",
               iter, (size_t)MAX_RECURSION_LIMIT))
        return ERR_CORRUPT;

    iter += 1;

    printf("(");
    if (node->left != NULL)
        (void)tree_print_node(node->left, iter);
    printf("%d", node->data);
    if (node->right != NULL)
        (void)tree_print_node(node->right, iter);
    printf(")");

    return OK;
}

err_t tree_print(const tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_print: tree is NULL"))
        return ERR_BAD_ARG;

    return tree_print_node(tree->root, 0);
}

err_t tree_delete_node(node_t * node, size_t iter)
{
    if (!CHECK(ERROR, node != NULL, "tree_delete_node: node is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, iter <= MAX_RECURSION_LIMIT,
               "tree_delete_node: recursion limit exceeded (%zu > %zu)",
               iter, (size_t)MAX_RECURSION_LIMIT))
        return ERR_CORRUPT;

    iter += 1;

    if (node->left  != NULL)
        if (!CHECK(ERROR, tree_delete_node(node->left,  iter) == OK,
                   "tree_delete_node: failed to delete left subtree"))
            return ERR_CORRUPT;

    if (node->right != NULL)
        if (!CHECK(ERROR, tree_delete_node(node->right, iter) == OK,
                   "tree_delete_node: failed to delete right subtree"))
            return ERR_CORRUPT;

    (void)node_dtor(node);
    return OK;
}

err_t tree_clear(const tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_clear: tree is NULL"))
        return ERR_BAD_ARG;

    if (tree->root == NULL)
        return OK;

    if (!CHECK(ERROR, tree_delete_node(tree->root, 0) == OK,
               "tree_clear: failed to delete nodes"))
        return ERR_CORRUPT;

    return OK;
}

err_t tree_insert(tree_t * const tree, const tree_elem_t data)
{
    if (!CHECK(ERROR, tree != NULL, "tree_insert: tree is NULL"))
        return ERR_BAD_ARG;

    node_t* node = (node_t*)calloc(1, sizeof(node_t));
    if (!CHECK(ERROR, node != NULL, "tree_insert: alloc failed"))
        return ERR_ALLOC;

    if (!CHECK(ERROR, node_ctor(node) == OK, "tree_insert: node_ctor failed"))
    {
        free(node);
        return ERR_CORRUPT;
    }

    node->data = data;

    tree->nodes_amount += 1;

    if (tree->root == NULL)
    {
        tree->root = node;
        return OK;
    }

    node_t* cur = tree->root;
    size_t   i  = 0;

    for (; i < MAX_RECURSION_LIMIT; ++i)
    {
        if (cur->left == NULL)
        {
            cur->left = node;
            break;
        }

        if (cur->right == NULL)
        {
            cur->right = node;
            break;
        }

        if (cur->data >= data)
            cur = cur->left;
        else
            cur = cur->right;
    }

    if (!CHECK(ERROR, i < MAX_RECURSION_LIMIT,
               "tree_insert: descent exceeded limit"))
        return ERR_CORRUPT;

    return OK;
}
