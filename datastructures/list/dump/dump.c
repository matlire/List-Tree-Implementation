#include "dump.h"

static size_t s_img_counter = 0;

static inline int in_bounds   (size_t i, size_t n)        { return i < n; }
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
    for (size_t cur = l->free_index; cur && in_bounds(cur, cap) && steps++ < cap; cur = l->next[cur]) 
    {
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
    for (size_t steps = 0; ; ++steps) 
    {
        if (!in_bounds(cur, cap) || is_free_node(l, cur)) return 0;
        size_t nxt = l->next[cur];
        if (nxt == 0) return 1;
        if (!in_bounds(nxt, cap) || is_free_node(l, nxt)) return 0;
        if (nxt != cur + 1) return 0;
        if (steps > cap)    return 0;
        cur = nxt;
    }
}

void list_dump(const list_t *list, const char *title, const char *html_file)
{
    if (!list || !html_file) return;

    const size_t capacity = list->list_capacity;

    bool *on_main = (bool*)calloc(capacity, sizeof(bool));
    bool *on_free = (bool*)calloc(capacity, sizeof(bool));
    long *virtpos = (long*)calloc(capacity, sizeof(long));
    if (!on_main || !on_free || !virtpos) { free(on_main); free(on_free); free(virtpos); return; }

    mark_free_chain(list, capacity, on_free);

    on_main[0] = true;
    virtpos[0] = 0;

    size_t cur = list->next[0];
    size_t pos = 1;

    for (size_t steps = 0; steps < list->list_size && cur && cur < capacity; ++steps)
    {
        if (on_main[cur] || is_free_node(list, cur)) break;
        on_main[cur] = true;
        virtpos[cur] = (long)pos++;
        cur = list->next[cur];
    }

    for (size_t i = 1; i < capacity; ++i)
        if (!on_main[i]) virtpos[i] = LONG_MAX;

    char dot_path[512], svg_name[64], svg_path[512];
    snprintf(svg_name, sizeof(svg_name), "img%zu.svg", s_img_counter++);
    snprintf(dot_path, sizeof(dot_path), "temp/graph.dot");
    snprintf(svg_path, sizeof(svg_path), "temp/l%s", svg_name);

    FILE *dot = fopen(dot_path, "w");
    if (!dot) { free(on_main); free(on_free); free(virtpos); return; }

    const char *EDGE_NEXT = "#00E676";
    const char *EDGE_PREV = "#2962FF";
    const char *EDGE_FREE = "#D500F9";
    const char *EDGE_PHYS = "#B0BEC5";

    const char *CELL_BG   = "#FFFFFF";
    const char *CELL_DATA = "#F5F7FF";

    const char *OUT_USE   = "#10B981";
    const char *OUT_FREE  = "#8B5CF6";
    const char *OUT_SENT  = "#475467";
    const char *OUT_OTHER = "#94A3B8";

    const char *FILL_USE   = "#E8FFF3";
    const char *FILL_FREE  = "#F3E8FF";
    const char *FILL_SENT  = "#ECEFF1";
    const char *FILL_OTHER = "#EEF2FF";

    const char *BAD_OUT   = "#FF1744";
    const char *BAD_FILL  = "#FFE9EE";
    const char *EDGE_PREV_PATCHED = "#FF1744";

    fprintf(dot, "digraph G {\n");
    fprintf(dot, "rankdir=TB;\n");
    fprintf(dot, "graph [bgcolor=\"#F7F7F7\", pad=0.25, nodesep=0.55, ranksep=0.9, splines=ortho];\n");
    fprintf(dot, "node  [shape=box, style=\"rounded,filled\", color=\"#D0D5DD\", penwidth=1.4, fillcolor=\"%s\", fontname=\"monospace\", fontsize=10];\n", CELL_BG);
    fprintf(dot, "edge  [color=\"#98A2B3\", penwidth=1.5, arrowsize=0.8, arrowhead=vee];\n");

    for (size_t i = 0; i < capacity; ++i) 
    {
        const char *outline =
            (i == 0)      ? OUT_SENT :
            on_free[i]    ? OUT_FREE :
            on_main[i]    ? OUT_USE  :
                            OUT_OTHER;

        const char *fill =
            (i == 0)      ? FILL_SENT :
            on_free[i]    ? FILL_FREE :
            on_main[i]    ? FILL_USE  :
                            FILL_OTHER;

        long  n_show = (list->next[i] == (size_t)-1) ? -1L : (long)list->next[i];
        long  p_show = (list->prev[i] == (size_t)-1) ? -1L : (long)list->prev[i];
        int   val    = (int)list->data[i];
        void* addr   = (void*)&list->data[i];

        char vbuf[32] = { 0 };
        const char *vstr = "-";
        if (i == 0) vstr = "0";
        else if (virtpos[i] > 0 && virtpos[i] != LONG_MAX) { snprintf(vbuf, sizeof vbuf, "%ld", virtpos[i]); vstr = vbuf; }

        fprintf(dot,
          "label%zu [color=\"%s\", fillcolor=\"%s\", penwidth=2.1, label=<"
          "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" COLOR=\"#D0D5DD\">"
            "<TR><TD COLSPAN=\"6\" BGCOLOR=\"%s\"><B>addr:</B> %p</TD></TR>"
            "<TR>"
              "<TD BGCOLOR=\"%s\"><B>phys:</B> %zu</TD>"
              "<TD BGCOLOR=\"%s\"><B>virt:</B> %s</TD>"
              "<TD BGCOLOR=\"%s\"><B>val:</B> %d</TD>"
              "<TD BGCOLOR=\"%s\"><B>n:</B> %ld</TD>"
              "<TD BGCOLOR=\"%s\"><B>p:</B> %ld</TD>"
            "</TR>"
          "</TABLE>"
          ">];\n",
          i, outline, fill, fill, addr,
          CELL_DATA, i, CELL_DATA, vstr,
          CELL_DATA, val, CELL_DATA, n_show, CELL_DATA, p_show);
    }

    fprintf(dot, "{ rank=same; ");
    for (size_t i = 0; i < capacity; ++i) 
        fprintf(dot, "label%zu; ", i);
    fprintf(dot, "}\n");

    for (size_t i = 0; i + 1 < capacity; ++i)
        fprintf(dot, "label%zu -> label%zu [color=\"%s\", arrowhead=none, weight=8, minlen=1, constraint=true];\n", i, i + 1, EDGE_PHYS);

    const size_t head = list->next[0];
    const size_t tail = list->prev[0];
    const size_t freei = list->free_index;

    fprintf(dot, "{ rank=min; headN [shape=oval, style=filled, fillcolor=\"#F3F4F6\", color=\"%s\", penwidth=1.7, label=\"head\"]; tailN [shape=oval, style=filled, fillcolor=\"#F3F4F6\", color=\"%s\", penwidth=1.7, label=\"tail\"]; freeN [shape=oval, style=filled, fillcolor=\"#F3F4F6\", color=\"%s\", penwidth=1.7, label=\"free\"]; }\n", EDGE_NEXT, EDGE_PREV, OUT_FREE);

    if (in_bounds(head, capacity)) fprintf(dot, "headN -> label%zu [color=\"%s\", penwidth=2.2];\n", head, EDGE_NEXT);
    if (in_bounds(tail, capacity)) fprintf(dot, "tailN -> label%zu [color=\"%s\", penwidth=2.2];\n", tail, EDGE_PREV);
    if (freei && in_bounds(freei, capacity)) fprintf(dot, "freeN -> label%zu [color=\"%s\", penwidth=2.2, style=dashed];\n", freei, EDGE_FREE);

    bool *paired_from = (bool*)calloc(capacity, sizeof(bool));
    if (!paired_from) { free(on_main); free(on_free); free(virtpos); fclose(dot); return; }

    
    for (size_t i = 0; i < capacity; ++i)
    {
        if (i == 0) continue;
        if (is_free_node(list, i)) continue;
        size_t j = list->next[i];
        if (!j) continue;

        if (!in_bounds(j, capacity) || is_free_node(list, j))
        {
                fprintf(dot, 
                        "badn_%zu [shape=hexagon, fillcolor=\"%s\", style=\"filled\", color=\"%s\", penwidth=4, label=\"%zu\"];\n", i, BAD_FILL, BAD_OUT, (size_t)j);
                fprintf(dot, 
                        "label%zu -> badn_%zu [color=\"%s\", penwidth=2.5, style=bold];\n", i, i, EDGE_PREV_PATCHED);
            continue;
        }

        if (i == tail && j == head) continue;

        if (in_bounds(j, capacity) && list->prev[j] == i && !(j == head && i == tail))
        {
            if (!paired_from[i])
            {
                fprintf(dot, 
                        "label%zu -> label%zu [color=\"#D0D5DD\", penwidth=2.0, dir=both, arrowhead=vee, arrowtail=vee];\n", i, j);
                paired_from[i] = true;
            }
        }
        else
        {
            fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=1.9];\n", i, j, EDGE_NEXT);
        }
    }

    for (size_t i = 0; i < capacity; ++i)
    {
        if (i == 0) continue;
        if (is_free_node(list, i)) continue;
        size_t j = list->prev[i];
        if (!j) continue;

        if (!in_bounds(j, capacity) || is_free_node(list, j))
        {
            fprintf(dot, 
                    "badp_%zu [shape=hexagon, fillcolor=\"%s\", style=\"filled\", color=\"%s\", penwidth=4, label=\"%zu\"];\n", i, BAD_FILL, BAD_OUT, (size_t)j);
            fprintf(dot, "label%zu -> badp_%zu [color=\"%s\", penwidth=2.5, style=bold];\n", i, i, EDGE_PREV_PATCHED);
            continue;
        }

        if (i == head && j == tail) continue;

        if (in_bounds(j, capacity) && list->next[j] == i && !(i == head && j == tail))
        {
            continue;
        }

        fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=1.9];\n", i, j, EDGE_PREV);
    }

    if (list->free_index && in_bounds(list->free_index, capacity))
    {
        for (size_t i = list->free_index, steps = 0;
             i && in_bounds(i, capacity) && steps++ < capacity; i = list->next[i])
        {
            size_t j = list->next[i];
            if (j && in_bounds(j, capacity) && is_free_node(list, i) && is_free_node(list, j))
                fprintf(dot, "label%zu -> label%zu [color=\"%s\", penwidth=2.0, style=dashed];\n", i, j, EDGE_FREE);
            if (list->next[i] == i) break;
        }
    }

    fprintf(dot, "}\n");
    fclose(dot);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "dot -T svg \"%s\" -o \"%s\"", dot_path, svg_path);
    system(cmd);

    FILE *html = fopen(html_file, "a");
    if (!html) { free(on_main); free(on_free); free(virtpos); free(paired_from); return; }

    const int linear = is_linearized(list, capacity);

    fprintf(html, "<body style=\"background-color:#F7F7F7;\">");
    fprintf(html, "<hr>\n");
    fprintf(html, "<h2>%s</h2>\n", title ? title : "List dump");
    fprintf(html, "<h3>Size: %zu, capacity: %zu</h3>\n", list->list_size, list->list_capacity);
    fprintf(html, "<h3>Head: %zu, Tail: %zu, Free: %zu</h3>\n", list->next[0], list->prev[0], list->free_index);
    fprintf(html, "<h3>Linearized: %d, needLinear: 1</h3>\n", linear);
    fprintf(html, "<h3>List addr: 0x%p</h3>\n", (void*)list);
    fprintf(html, "<img src=\"temp/l%s\" />\n", svg_name);
    fprintf(html, "</hr>\n");
    fclose(html);

    free(on_main);
    free(on_free);
    free(virtpos);
    free(paired_from);
}

