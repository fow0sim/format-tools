#ifndef FDUMP_H
#define FDUMP_H

typedef struct _str_chain_t {
    struct _str_chain_t *next;
    char * name;
} str_chain_t;

typedef struct _opt_t {
    int verbose_flag;
    str_chain_t *flist;
} opt_t;

void dump_file(const char* filename);
void dump_line(char *line, int linect, int charct);
void add_string_node(str_chain_t **llist, const char *str);
void free_string_nodes(str_chain_t **llist);
int parse_options(opt_t *opts, int argc, char **argv);
void print_usage();

#endif
