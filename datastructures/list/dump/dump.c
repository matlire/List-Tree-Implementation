#include "dump.h"

static size_t s_img_counter = 0;

static inline int in_bounds   (size_t i,        size_t n) { return i < n; }
static inline int is_free_node(const list_t* l, size_t i) { return (i != 0) && (l->prev[i] == LIST_FREE); }

void list_dump_reset(const char *html_file)
{
    if (!html_file) return;
    FILE *f = fopen(html_file, "w");
    if (f) fclose(f);
    s_img_counter = 0;
}

static void mark_free_chain(const list_t* l, size_t cap, bool* on_free)
{
    size_t steps = 0;
    for (size_t cur = l->free_index; cur && in_bounds(cur, cap) && steps++ < cap; cur = l->next[cur]) {
        if (!is_free_node(l, cur)) break;
        if (on_free[cur]) break;
        on_free[cur] = true;
        if (l->next[cur] == cur) break;
    }
}

static int is_linearized(const list_t* l, size_t cap)
{
    size_t cur = l->next[0];
    if (!cur) return 1;
    for (size_t steps = 0; ; ++steps) {
        if (!in_bounds(cur, cap) || is_free_node(l, cur)) return 0;
        size_t nxt = l->next[cur];
        if (nxt == 0) return 1;
        if (!in_bounds(nxt, cap) || is_free_node(l, nxt)) return 0;
        if (nxt != cur + 1) return 0;
        if (steps > cap)     return 0;
        cur = nxt;
    }
}

void list_dump(const list_t *list,  size_t capacity,
               const char   *title, const char *html_file)
{
    if (!list || !html_file || capacity == 0) return;

    bool *on_main = (bool*)calloc(capacity, sizeof(bool));
    bool *on_free = (bool*)calloc(capacity, sizeof(bool));
    long *virtpos = (long*)calloc(capacity, sizeof(long));
    if (!on_main || !on_free || !virtpos) { free(on_main); free(on_free); free(virtpos); return; }

    mark_free_chain(list, capacity, on_free);

    char dot_path[512], svg_name[64], svg_path[512];
    snprintf(svg_name, sizeof(svg_name), "img%zu.svg", s_img_counter++);
    snprintf(dot_path, sizeof(dot_path), "temp/graph.dot");
    snprintf(svg_path, sizeof(svg_path), "temp/%s", svg_name);

    FILE *dot = fopen(dot_path, "w");
    if (!dot) { free(on_main); free(on_free); free(virtpos); return; }

    const char *EDGE_NEXT   = "#22C55E";
    const char *EDGE_PREV   = "#EF4444";
    const char *EDGE_FREE   = "#A855F7";
    const char *EDGE_PHYS   = "#DCE6E3";

    const char *CELL_BG     = "#FFFFFF";
    const char *CELL_DATA   = "#EEF4FF";

    const char *OUT_USE     = "#16A34A";
    const char *OUT_FREE    = "#7F56D9";
    const char *OUT_SENT    = "#7F56D9";
    const char *OUT_OTHER   = "#EF4444";

    fprintf(dot, "digraph G {");
    fprintf(dot, "rankdir=LR;\n");
    fprintf(dot, "graph [bgcolor=\"#FCFCFD\", pad=0.25, nodesep=0.55, ranksep=0.9, splines=true];\n");
    fprintf(dot, "node  [shape=box, style=\"rounded,filled\", color=\"#D0D5DD\", penwidth=1.4, "
                 "fillcolor=\"%s\", fontname=\"monospace\", fontsize=10];\n", CELL_BG);
    fprintf(dot, "edge  [color=\"#98A2B3\", penwidth=1.5, arrowsize=0.8, arrowhead=vee];\n");

    for (size_t i = 0; i < capacity; ++i) {
        const char *outline =
            (i == 0)      ? OUT_SENT :
            on_free[i]    ? OUT_FREE :
            on_main[i]    ? OUT_USE  :
                            OUT_OTHER;

        long  n_show = (list->next[i] == (size_t)-1) ? -1L : (long)list->next[i];
        long  p_show = (list->prev[i] == (size_t)-1) ? -1L : (long)list->prev[i];
        int   val    = (int)list->data[i];
        void* addr   = (void*)&list->data[i];

        fprintf(dot,
          "label%zu [color=\"%s\", penwidth=2.1, label=<"
          "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" COLOR=\"#D0D5DD\">"
            "<TR><TD COLSPAN=\"6\" BGCOLOR=\"%s\"><B>addr:</B> 0x%p</TD></TR>"
            "<TR>"
              "<TD BGCOLOR=\"%s\"><B>phys:</B> %zu</TD>"
              "<TD BGCOLOR=\"%s\"><B>virt:</B> %ld</TD>"
              "<TD BGCOLOR=\"%s\"><B>val:</B> %d</TD>"
              "<TD BGCOLOR=\"%s\"><B>n:</B> %ld</TD>"
              "<TD BGCOLOR=\"%s\"><B>p:</B> %ld</TD>"
            "</TR>"
          "</TABLE>"
          ">];\n",
          i, outline, CELL_DATA, addr,
          CELL_DATA, i, CELL_DATA, virtpos[i],
          CELL_DATA, val, CELL_DATA, n_show, CELL_DATA, p_show);
    }

    for (size_t i = 0; i + 1 < capacity; ++i) {
        fprintf(dot, "label%zu -> label%zu [color=\"%s\", style=\"dashed\", arrowhead=\"none\", weight=6, minlen=1];\n", i, i + 1, EDGE_PHYS);
    }

    const size_t head = list->next[0];
    const size_t tail = list->prev[0];

    for (size_t i = 0; i < capacity; ++i) {
        size_t j = list->next[i];
        if (!j || !in_bounds(j, capacity)) continue;
        if (is_free_node(list, j)) continue;
        if (i != 0 && is_free_node(list, i)) continue;
        if (i == tail && j == head) continue;
        fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=1.9];\n", i, j, EDGE_NEXT);
    }    

    for (size_t i = 0; i < capacity; ++i) {
        size_t j = list->prev[i];
        if (!j || !in_bounds(j, capacity)) continue;
        if (is_free_node(list, i) || is_free_node(list, j)) continue;
        if (i == head && j == tail) continue;
        fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=1.9];\n", i, j, EDGE_PREV);
    }

    if (list->free_index && in_bounds(list->free_index, capacity)) {
        for (size_t i = list->free_index, steps = 0;
             i && in_bounds(i, capacity) && steps++ < capacity; i = list->next[i])
        {
            size_t j = list->next[i];
            if (j && in_bounds(j, capacity) && is_free_node(list, i) && is_free_node(list, j))
                fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=2.0];\n", i, j, EDGE_FREE);
            if (list->next[i] == i) break;
        }
    }
    fprintf(dot, "}");
    fclose(dot);

    free(on_main);
    free(on_free);
    free(virtpos);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "dot -T svg '%s' -o '%s'", dot_path, svg_path);
    system(cmd);

    FILE *html = fopen(html_file, "a");
    if (!html) return;

    const int linear = is_linearized(list, capacity);

    fprintf(html, "<hr>\n");
    fprintf(html, "<h2>%s</h2>\n", title ? title : "List dump");
    fprintf(html, "<h3>Size: %zu, capacity: %zu</h3>\n", list->list_size, list->list_capacity);
    fprintf(html, "<h3>Head: %zu, Tail: %zu, Free: %zu</h3>\n", list->next[0], list->prev[0], list->free_index);
    fprintf(html, "<h3>Linearized: %d, needLinear: 1</h3>\n", linear);
    fprintf(html, "<h3>List addr: 0x%p</h3>\n", (void*)list);
    fprintf(html, "<img src=\"temp/%s\" />\n", svg_name);
    fprintf(html, "</hr>\n");
    fclose(html);
}

