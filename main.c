#include "libs/types.h"

#include "datastructures/list/dump/dump.h"
#include "datastructures/list/list.h"

#include "datastructures/tree/dump/dump.h"
#include "datastructures/tree/tree.h"

#include <stdlib.h>

#define LDUMP(list_ptr, title_str) \
    list_dump((list_ptr), (title_str), "gdump.html")

#define TDUMP(tree_ptr, title_str) \
    tree_dump((tree_ptr), (title_str), "tgdump.html")

void test_list()
{
    list_dump_reset("gdump.html");

    CREATE_LIST(l1);
    LDUMP(&l1, "after ctor");
    list_verify(&l1);

    size_t real_index = 0;
    push_front(&l1, 10, &real_index); LDUMP(&l1, "push front 10 (after 0)");
    push_back (&l1, 20, &real_index); LDUMP(&l1, "push back 20 (after 1)");
    ins_elem_after(&l1, 2, 30);       LDUMP(&l1, "after insert 30 (after 2)");
    
    /*
    l1.prev[2] = 5;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    ins_elem_after(&l1, 3, 40);  LDUMP(&l1, "after insert 40 (after 3)");
    ins_elem_after(&l1, 4, 50);  LDUMP(&l1, "after insert 50 (after 4)");

    /*
    l1.next[7] = 6;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    ins_elem_after(&l1, 5, 60);  LDUMP(&l1, "after insert 60 (after 5)");

    /*
    l1.prev[4] = 2;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    /*
    l1.next[4] = 10;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    /*
    l1.free_index = 4;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */ 

    /*
    l1.free_index = 10;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    /*    
    l1.list_size = 3;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    /*
    l1.list_size = 10;
    LDUMP(&l1, "after spoiling");
    list_verify(&l1);
    */

    ins_elem_after(&l1, 3, 35);  LDUMP(&l1, "after insert 35 (after 3)");
    ins_elem_after(&l1, 7, 36);  LDUMP(&l1, "after insert 36 (after 4)");
    ins_elem_after(&l1, 8, 37);  LDUMP(&l1, "after insert 37 (after 5)");
    ins_elem_after(&l1, 9, 38);  LDUMP(&l1, "after insert 38 (after 6)");
    ins_elem_after(&l1, 10, 39); LDUMP(&l1, "after insert 39 (after 7)");

    del_elem(&l1, 9);             LDUMP(&l1, "after delete index 9");
    ins_elem_before(&l1, 10, 37); LDUMP(&l1, "before insert 37 (before 10)");

    del_elem(&l1, 1);             LDUMP(&l1, "after delete index 1");
    del_elem(&l1, 6);             LDUMP(&l1, "after delete index 6");

    ins_elem_before(&l1, 2, 10);  LDUMP(&l1, "before insert 10 (before 2)");
    ins_elem_after (&l1, 5, 60);  LDUMP(&l1, "after insert 60 (after 5)");

    list_linearize(&l1); LDUMP(&l1, "after linearaization");

    list_dtor(&l1);
}

#define SET_NODE_VALUES(node, idata, ileft, iright) \
    (node)->data  = (idata);  \
    (node)->left  = (ileft);  \
    (node)->right = (iright); \

void test_tree()
{
    tree_dump_reset("tgdump.html");

    CREATE_TREE(t1);

    tree_insert(&t1, 10);
    tree_insert(&t1, 5);
    tree_insert(&t1, 20); 
    tree_insert(&t1, 3);
    tree_insert(&t1, 7); 
    tree_insert(&t1, 15);
    tree_insert(&t1, 13);
    tree_insert(&t1, 16);

    tree_print(&t1);
    TDUMP(&t1, "test");

    tree_insert(&t1, 6);
    tree_print(&t1);
    TDUMP(&t1, "test"); 

    tree_dtor(&t1);
}

int main(const int argc, char* const argv[])
{
    unused argc;
    unused argv;

    init_logging("log.log", DEBUG);

    test_list();
    test_tree();
    
    close_log_file();
    return 0;
}

