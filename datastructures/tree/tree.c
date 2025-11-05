#include "tree.h"

err_t node_ctor(node_t * const node)
{
    node->data  = 0;
    node->left  = NULL;
    node->right = NULL;
}

err_t node_dtor(node_t * const node)
{
    if (node) free(node);
    return OK;
}

err_t tree_ctor(tree_t * const tree)
{
    tree->nodes_amount = 0;
    tree->root = NULL;
    return OK;
}

err_t tree_dtor(tree_t * const tree)
{
    //tree_delete_node(tree->root);
    tree->root = NULL;
    tree->nodes_amount = 0;
    return OK;
}

err_t tree_verify(const tree_t * const tree)
{
    if (tree == NULL) return ERR_BAD_ARG;
    if (tree->nodes_amount > 0 && tree->root == NULL) return ERR_CORRUPT;
    return OK;
}

err_t tree_print_node(const node_t * const node)
{
    if (node == NULL) return ERR_BAD_ARG;

    printf("(");
    if (node->left != NULL)
        tree_print_node(node->left);
    printf("%d", node->data);
    if (node->right != NULL)
        tree_print_node(node->right);
    printf(")");

    return OK;
}

err_t tree_print(const tree_t * const tree)
{
    return tree_print_node(tree->root);
}

err_t tree_delete_node(const node_t * node);

err_t tree_clear(const tree_t * const tree)
{
    if (tree == NULL) return ERR_BAD_ARG;

    
    return OK;
}

err_t tree_insert(tree_t * const tree)
{
    if (tree == NULL) return ERR_BAD_ARG;

    node_t* node = (node_t*
    )calloc();

    return OK;
}
