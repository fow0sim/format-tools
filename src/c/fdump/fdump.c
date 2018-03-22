#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if LONG_OPTS
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include "fdump.h"

int main(int argc, char **argv) {
    int         rc;
    opt_t       opts   = { .verbose_flag = 0, .flist = NULL };
    str_chain_t *flist;

    rc = parse_options(&opts, argc, argv);
    if (rc == -1) {
        print_usage();
        return 0;
    }

    if (opts.flist == NULL) {
        dump_file(NULL);
    } else {
        flist = opts.flist;
        while (flist != NULL) {
            dump_file(flist->name);
            flist = flist->next;
        }
    }
    free_string_nodes(&opts.flist);
}

void dump_file(const char *filename) {
    FILE *fp;
    char dump[32];
    int  num;
    int  lx;

    if (filename == NULL) {
        fp = stdin;
    } else {
        fp = fopen(filename, "rb");
    }
    if (fp == NULL) {
        fprintf(stderr, "File %s not found\n", filename);
    } else {
        if (filename != NULL) {
            printf("%s\n", filename);
        }
        lx = 0;
        do {
            num = fread(dump, 1, sizeof(dump), fp);
            if (num > 0) {
                dump_line(dump, lx, num);
                lx++;
            }
        } while (num > 0);
        fclose(fp);
    }
}

void dump_line(char *line, int linect, int charct) {
    char dump[256];
    char buf[64];
    char dspl[35];
    int  ix;

    sprintf(dump, "%8.8X: ", linect * 32);
    memset(dspl, '\0', sizeof(dspl));
    for (ix = 0; ix < 32; ix++) {
        if (ix < charct) {
            sprintf(buf, "%2.2X", (unsigned char)line[ix]);
            if (!isprint(line[ix])) {
                dspl[ix] = '.';
            } else {
                dspl[ix] = line[ix];
            }
        } else {
            strcpy(buf, "  ");
        }
        if (ix % 4 == 3) {
            strcat(buf, " ");
        }
        if (ix == 15) {
            strcat(buf, " ");
        }
        strcat(dump, buf);
    }
    sprintf(buf, "<%s>", dspl);
    strcat(dump, buf);
    printf("%s\n", dump);
}

int parse_options(opt_t *opts, int argc, char **argv) {
#if LONG_OPTS
    struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };
    int option_index = 0;
#endif
    int c;
    int rc;

    rc = 0;
#if LONG_OPTS
    while ((c = getopt_long(argc, argv, "vh", long_options, &option_index)) != -1 && rc == 0) {
#else
    while ((c = getopt(argc, argv, "vh")) != -1 && rc == 0) {
#endif
        switch (c) {
        case 0:
            break;
        case 'v':
            opts->verbose_flag = 1;
            break;
        case 'h':
        case '?':
            rc = -1;
            break;
        default:
            abort();
        }
    }

    while (rc == 0 && optind < argc) {
        add_string_node(&opts->flist, argv[optind]);
        optind += 1;
    }

    return rc;
}

void add_string_node(str_chain_t * *llist, const char *str) {
    str_chain_t **p;
    p = llist;
    while (*p != NULL) {
        p = &((*p)->next);
    }
    *p = malloc(sizeof(str_chain_t));
    (*p)->next = NULL;
    (*p)->name = (char *)strdup(str);
}

void free_string_nodes(str_chain_t * *llist) {
    str_chain_t * p,*q;
    p = *llist;
    while (p != NULL) {
        q = p->next;
        free(p->name);
        free(p);
        p = q;
    }
    *llist = NULL;
}

void print_usage() {
    printf("Display a file in z/OS dump format\n");
    printf("Usage: fdump [options] [[file] ..]\n\n");
    printf("Options:\n");
    printf(" --help, -h\t display this help page\n");
    printf(" --verbose, -v\t be verbose\n");
    printf(" file\t\t file name (default is stdin)\n");
}

