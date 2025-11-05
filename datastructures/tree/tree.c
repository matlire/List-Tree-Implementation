#include "tree.h"

err_t node_ctor(node_t * const node)
{
    node->data  = 0;
    node->left  = NULL;
    node->right = NULL;
}

err_t node_dtor(node_t * node)
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
    tree_clear(tree);
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

err_t tree_delete_node(node_t * node)
{
    if (node == NULL) return ERR_BAD_ARG;

    if (node->left != NULL)
        tree_delete_node(node->left);
    if (node->right != NULL)
        tree_delete_node(node->right);
    node_dtor(node);

    return OK;
}

err_t tree_clear(const tree_t * const tree)
{
    if (tree == NULL) return ERR_BAD_ARG;

    tree_delete_node(tree->root);
    
    return OK;
}

err_t tree_insert(tree_t * const tree, const tree_elem_t data)
{
    if (tree == NULL) return ERR_BAD_ARG;

    node_t* node = (node_t*)calloc(1, sizeof(node_t));
    if (node == NULL) return ERR_ALLOC;
    node_ctor(node);
    node->data = data;
    
    tree->nodes_amount += 1;
    if (tree->root == NULL)
    {
        tree->root = node;
        return OK;
    }

    node_t* cur = tree->root;
    while (true)
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

    return OK;
}
