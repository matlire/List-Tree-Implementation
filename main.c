
#include "libs/types.h"
#include "datastructures/list/dump/dump.h"
#include "datastructures/list/list.h"
#include <stdlib.h> // for system()

#define DUMP(list_ptr, title_str) \
    list_dump((list_ptr), (list_ptr)->list_size, (title_str), "gdump.html")

int main(const int argc, char* const argv[])
{
    unused argc;
    unused argv;

    init_logging("log.log", DEBUG);

    system("mkdir -p temp");
    list_dump_reset("gdump.html");

    CREATE_LIST(l1);
    DUMP(&l1, "after ctor");

    ins_elem_after(&l1, 0, 10);  DUMP(&l1, "after insert 10 (after 0)");
    ins_elem_after(&l1, 1, 20);  DUMP(&l1, "after insert 20 (after 1)");
    ins_elem_after(&l1, 2, 30);  DUMP(&l1, "after insert 30 (after 2)");
    ins_elem_after(&l1, 3, 40);  DUMP(&l1, "after insert 40 (after 3)");
    ins_elem_after(&l1, 4, 50);  DUMP(&l1, "after insert 50 (after 4)");
    ins_elem_after(&l1, 5, 60);  DUMP(&l1, "after insert 60 (after 5)");

    ins_elem_after(&l1, 3, 35);  DUMP(&l1, "after insert 35 (after 3)");
    ins_elem_after(&l1, 7, 36);  DUMP(&l1, "after insert 36 (after 4)");
    ins_elem_after(&l1, 8, 37);  DUMP(&l1, "after insert 37 (after 5)");
    ins_elem_after(&l1, 9, 38);  DUMP(&l1, "after insert 38 (after 6)");
    ins_elem_after(&l1, 10, 39); DUMP(&l1, "after insert 39 (after 7)");

    del_elem(&l1, 9);             DUMP(&l1, "after delete index 9");
    ins_elem_before(&l1, 10, 37); DUMP(&l1, "before insert 37 (before 10)");

    del_elem(&l1, 1);             DUMP(&l1, "after delete index 1");
    del_elem(&l1, 6);             DUMP(&l1, "after delete index 6");

    ins_elem_before(&l1, 2, 10); DUMP(&l1, "before insert 10 (before 2)");
    ins_elem_after (&l1, 5, 60);  DUMP(&l1, "after insert 60 (after 5)");

    list_dtor(&l1);

    close_log_file();
    return 0;
}

