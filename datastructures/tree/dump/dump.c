#include "dump.h"

static size_t s_img_counter = 0;

typedef struct NodeInfo
{
    const node_t *node;
    size_t        id;
    size_t        depth;
    size_t        xpos;
} NodeInfo;

void tree_dump_reset(const char *html_file)
{
    if (!html_file) return;
    FILE *f = fopen(html_file, "w");
    if (f) fclose(f);
    s_img_counter = 0;
}

static size_t find_index_by_ptr(NodeInfo *arr, size_t n, const node_t *p)
{
    for (size_t i = 0; i < n; ++i)
        if (arr[i].node == p) return i;
    return (size_t)-1;
}

static void assign_inorder_xpos(const node_t *node, NodeInfo *arr, size_t n, size_t *counter)
{
    if (!node) return;
    assign_inorder_xpos(node->left,  arr, n, counter);
    size_t idx = find_index_by_ptr(arr, n, node);
    if (idx != (size_t)-1) arr[idx].xpos = (*counter)++;
    assign_inorder_xpos(node->right, arr, n, counter);
}

void tree_dump(const tree_t *tree, const char *title, const char *html_file)
{
    if (!tree || !html_file) return;

    ensure_temp_dir();

    const node_t *root = tree->root;
    char dot_path[512], svg_name[64], svg_path[512];
    snprintf(svg_name, sizeof(svg_name), "img%zu.svg", s_img_counter++);
    snprintf(dot_path, sizeof(dot_path), "temp/graph.dot");
    snprintf(svg_path, sizeof(svg_path), "temp/%s", svg_name);

    FILE *dot = fopen(dot_path, "w");
    if (!dot) return;

    const char *EDGE_LEFT  = "#98A2B3";
    const char *EDGE_RIGHT = "#98A2B3";
    const char *OUT_ROOT   = "#16A34A";
    const char *OUT_NODE   = "#475467";
    const char *FILL_NODE  = "#F9FAFB";
    const char *FILL_ROOT  = "#E6F4EA";
    const char *CELL_BG    = "#FFFFFF";
    const char *TABLE_BRD  = "#D0D5DD";
    const char *TXT_COLOR  = "#111827";

    fprintf(dot, "digraph G {\n");
    fprintf(dot, "rankdir=TB;\n");
    fprintf(dot, "bgcolor=\"white\";\n");
    fprintf(dot, "labelloc=t;\n");
    fprintf(dot, "labeljust=l;\n");
    fprintf(dot, "fontname=\"monospace\";\n");
    fprintf(dot, "fontsize=18;\n");
    fprintf(dot, "label=\"%s\";\n", (title && *title) ? title : "tree");

    fprintf(dot, "node  [shape=box, style=\"rounded,filled\", color=\"%s\", fillcolor=\"%s\", fontname=\"monospace\", fontsize=10];\n", OUT_NODE, FILL_NODE);
    fprintf(dot, "edge  [color=\"#98A2B3\", penwidth=1.7, arrowsize=0.8, arrowhead=vee];\n");

    if (!root) {
        fprintf(dot, "empty [label=\"<empty tree>\", color=\"#9CA3AF\", fontcolor=\"#9CA3AF\", fillcolor=\"#F3F4F6\"];\n");
        fprintf(dot, "}\n");
        fclose(dot);

        char cmd_empty[4096];
        snprintf(cmd_empty, sizeof(cmd_empty), "dot -T svg \"%s\" -o \"%s\"", dot_path, svg_path);
        system(cmd_empty);

        FILE *html = fopen(html_file, "a");
        if (html) {
            fprintf(html, "<h2>%s</h2>\n", title ? title : "Tree");
            fprintf(html, "<h3>Nodes: 0</h3>\n");
            fprintf(html, "<h3>Root: (null)</h3>\n");
            fprintf(html, "<img src=\"temp/%s\" />\n", svg_name);
            fclose(html);
        }
        return;
    }

    size_t cap  = 64;
    size_t n    = 0;
    size_t head = 0;
    size_t tail = 0;
    NodeInfo *nodes = (NodeInfo*)calloc(cap, sizeof(NodeInfo));
    const node_t **q_nodes = (const node_t**)calloc(cap, sizeof(node_t*));
    size_t *q_depths = (size_t*)calloc(cap, sizeof(size_t));
    if (!nodes || !q_nodes || !q_depths) 
        { free(nodes); free(q_nodes); free(q_depths); fclose(dot); return; }

    q_nodes[tail]  = root;
    q_depths[tail] = 0;
    tail = 1;

    while (head < tail) {
        const node_t *cur = q_nodes[head];
        size_t depth      = q_depths[head];
        head++;

        if (n == cap) {
            cap *= 2;
            nodes    = (NodeInfo*)realloc(nodes, cap * sizeof(NodeInfo));
            q_nodes  = (const node_t**)realloc(q_nodes, cap * sizeof(node_t*));
            q_depths = (size_t*)realloc(q_depths, cap * sizeof(size_t));
            if (!nodes || !q_nodes || !q_depths) 
                { free(nodes); free(q_nodes); free(q_depths); fclose(dot); return; }
        }

        nodes[n].node  = cur;
        nodes[n].id    = n;
        nodes[n].depth = depth;
        nodes[n].xpos  = 0;
        n++;

        if (cur->left)  { q_nodes[tail] = cur->left;  q_depths[tail] = depth + 1; tail++; }
        if (cur->right) { q_nodes[tail] = cur->right; q_depths[tail] = depth + 1; tail++; }
    }

    free(q_nodes);
    free(q_depths);

    size_t counter = 0;
    assign_inorder_xpos(root, nodes, n, &counter);

    for (size_t i = 0; i < n; ++i) {
        const node_t *p = nodes[i].node;
        const char *outline = (p == root) ? OUT_ROOT : OUT_NODE;
        const char *fill    = (p == root) ? FILL_ROOT : FILL_NODE;

        fprintf(dot,
          "n%zu [color=\"%s\", fillcolor=\"%s\", penwidth=2.0, label=<"
          "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" COLOR=\"%s\">"
          "<TR><TD COLSPAN=\"2\" BGCOLOR=\"%s\"><B><FONT COLOR=\"%s\">node</FONT></B></TD></TR>"
          "<TR><TD ALIGN=\"LEFT\">addr</TD><TD ALIGN=\"LEFT\">0x%p</TD></TR>"
          "<TR><TD ALIGN=\"LEFT\">data</TD><TD ALIGN=\"LEFT\">%d</TD></TR>"
          "<TR><TD ALIGN=\"LEFT\">left</TD><TD ALIGN=\"LEFT\">0x%p</TD></TR>"
          "<TR><TD ALIGN=\"LEFT\">right</TD><TD ALIGN=\"LEFT\">0x%p</TD></TR>"
          "<TR><TD ALIGN=\"LEFT\">depth</TD><TD ALIGN=\"LEFT\">%zu</TD></TR>"
          "</TABLE>"
          ">];\n",
          nodes[i].id, outline, fill, TABLE_BRD, CELL_BG, TXT_COLOR,
          (void*)p, (int)p->data, (void*)p->left, (void*)p->right, nodes[i].depth
        );
    }

    for (size_t i = 0; i < n; ++i) {
        const node_t *p = nodes[i].node;
        if (p->left) {
            size_t j = find_index_by_ptr(nodes, n, p->left);
            if (j != (size_t)-1)
                fprintf(dot, "n%zu -> n%zu [color=\"%s\", penwidth=1.9];\n", 
                        nodes[i].id, nodes[j].id, EDGE_LEFT);
        }
        if (p->right) {
            size_t j = find_index_by_ptr(nodes, n, p->right);
            if (j != (size_t)-1)
                fprintf(dot, "n%zu -> n%zu [color=\"%s\", penwidth=1.9];\n", 
                        nodes[i].id, nodes[j].id, EDGE_RIGHT);
        }
    }

    size_t max_depth = 0;
    for (size_t i = 0; i < n; ++i) if (nodes[i].depth > max_depth) max_depth = nodes[i].depth;

    for (size_t d = 0; d <= max_depth; ++d) {
        size_t cnt = 0;
        for (size_t i = 0; i < n; ++i) if (nodes[i].depth == d) cnt++;
        if (cnt == 0) continue;

        size_t *ids = (size_t*)malloc(cnt * sizeof(size_t));
        size_t k = 0;
        for (size_t i = 0; i < n; ++i) if (nodes[i].depth == d) ids[k++] = i;

        for (size_t a = 1; a < cnt; ++a) {
            size_t key = ids[a];
            size_t b = a;
            while (b > 0 && nodes[ids[b-1]].xpos > nodes[key].xpos) {
                ids[b] = ids[b-1];
                --b;
            }
            ids[b] = key;
        }

        fprintf(dot, "{ rank=same; ");
        for (size_t i = 0; i < cnt; ++i) fprintf(dot, "n%zu; ", nodes[ids[i]].id);
        fprintf(dot, "}\n");

        for (size_t i = 1; i < cnt; ++i)
            fprintf(dot, "n%zu -> n%zu [style=invis, weight=10, constraint=true];\n",
                    nodes[ids[i-1]].id, nodes[ids[i]].id);

        free(ids);
    }

    fprintf(dot, "}\n");
    fclose(dot);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "dot -T svg \"%s\" -o \"%s\"", dot_path, svg_path);
    system(cmd);

    FILE *html = fopen(html_file, "a");
    if (html) {
        fprintf(html, "<h2>%s</h2>\n", title ? title : "Tree");
        fprintf(html, "<h3>Nodes: %zu</h3>\n", n);
        fprintf(html, "<h3>Root: 0x%p</h3>\n", (void*)root);
        fprintf(html, "<img src=\"temp/%s\" />\n", svg_name);
        fprintf(html, "<hr/>\n");
        fclose(html);
    }

    free(nodes);
}
