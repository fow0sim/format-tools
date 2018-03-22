#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if LONG_OPTS
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include "fix2csv.h"


int main(int argc, char **argv) {
    int ix;
    int rc;
    opt_t opts = { .flags = 0, .dlm = DEFAULT_DELIMITER,
        .fields = NULL, .filename = NULL };
    char *s;
    FIELD * fields,*fld;

    if (((s = getenv("LC_ALL")) && *s) ||
        ((s = getenv("LC_CTYPE")) && *s) ||
        ((s = getenv("LANG")) && *s)) {
        if (strstr(s, "UTF-8")) opts.flags |= MODE_UTF8;
    }

    rc = parse_options(&opts, argc, argv);
    if (rc == -1) {
        print_usage();
        return 0;
    }

    parse_fields(&fields, opts.fields);
    copy_fields(fields, &opts);
    dealloc_fields(&fields);
}

void copy_fields(FIELD *fields, opt_t *opts) {
    FIELD *fld;
    FILE *fp;
    char buffer[4096];
    char outrec[4096];
    int buflen = sizeof(buffer);
    char *p, *q;
    int pos, len;

    if (opts->filename == NULL) {
        fp = stdin;
    } else {
        if ((fp = fopen(opts->filename, "rt")) == NULL) {
            fprintf(stderr, "Could not open input file %s\n", opts->filename);
            exit(1);
        }
    }

    while (fgets(buffer, buflen, fp) != NULL) {
        if ((p = strchr(buffer, '\n'))) {
            *p = '\0';
        }

        if (fields == NULL) {
            strcpy(outrec, buffer);
        } else {
            p = outrec;
            *p = '\0';
            fld = fields;
            while (fld != NULL) {
                if (p > outrec) p += sprintf(p, "%c", opts->dlm);
                if (fld->pos > 0 && fld->pos <= strlen(buffer)) {
                    if (opts->flags & MODE_UTF8) {
                        q = utf8index(buffer, fld->pos - 1);
                    } else {
                        q = buffer + fld->pos - 1;
                    }
                    if (opts->flags & MODE_UTF8) {
                        len = utf8len(q, fld->len);
                    } else {
                        len = (fld->len < 0 || fld->len > strlen(q)) ? strlen(q) : fld->len;
                    }
                    sprintf(p, "%*.*s", len, len, q);
                    if (!(opts->flags & MODE_PRESERVE)) trim_whitespace(p);
                    p += strlen(p);
                }
                fld = fld->next;
            }
        }

        printf("%s\n", outrec);
    }

    fclose(fp);

    return;
}

char* utf8index(char *s, int pos) {
    int ix = pos + 1;

    for (; *s; ++s) {
        if ((*s & 0xC0) != 0x80) --ix;
        if (ix == 0) {
            return s;
        }
    }
    return NULL;
}

int utf8len(char *s, int len) {
    int len8;
    char *q = utf8index(s, len);
    len8 = (q) ? q - s : strlen(s);
    return len8;
}

void parse_fields(FIELD **fields, char const *fieldstr) {
    FIELD * *current,*fld;
    char str[1024];
    char *token;
    char *p;
    int start, end, len;

    *fields = NULL;
    current = fields;
    if (fieldstr == NULL || strlen(fieldstr) == 0) {
        return;
    }

    strcpy(str, fieldstr);
    token = strtok(str, " ,;");
    do {
        start = 0;
        len = 1;
        end = 0;

        if ((p = strchr(token, '-'))) {
            *p++ = '\0';
        }
        if (strlen(token) > 0) {
            sscanf(token, "%d", &start);
        }
        if (p) {
            if (strlen(p) > 0) {
                sscanf(p, "%d", &end);
                len = end - start + 1;
            } else {
                len = -1;
            }
        } else {
            len = 1;
        }
        if (start <= 0 || len < -1) {
            fprintf(stderr, "Invalid field description\n");
            exit(1);
        }

        fld = malloc(sizeof(FIELD));
        fld->next = NULL;
        fld->pos = start;
        fld->len = len;
        *current = fld;
        current = &(*current)->next;

    } while ((token = strtok(NULL, " ,;")));


    return;
}

void dealloc_fields(FIELD **fields) {
    FIELD * current,*next;

    current = *fields;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }

    return;
}

static char const *WHITESPACE = " \t\n\r";

void trim_whitespace(char *s) {
    char *firstWord, *lastWord, *trailingSpace;
    size_t newLength;

    firstWord = lastWord = s + strspn(s, WHITESPACE);
    do {
        trailingSpace = lastWord + strcspn(lastWord, WHITESPACE);
        lastWord = trailingSpace + strspn(trailingSpace, WHITESPACE);
    } while (*lastWord != '\0');

    newLength = trailingSpace - firstWord;
    memmove(s, firstWord, newLength);
    s[newLength] = '\0';
}

int parse_options(opt_t *opts, int argc, char **argv) {
#if LONG_OPTS
    struct option long_options[] = {
        { "help", no_argument, 0, 'h' },
        { "preserve", no_argument, 0, 'p' },
        { "fields", required_argument, 0, 'f' },
        { "delimiter", required_argument, 0, 'd' },
        { "ascii", no_argument, 0, 'a' },
        { "utf8", no_argument, 0, 'u' },
        { 0, 0, 0, 0 }
    };
    int option_index = 0;
#endif
    int c;
    int rc;

    rc = 0;

#if LONG_OPTS
    while ((c = getopt_long(argc, argv, "pf:d:auh", long_options, &option_index)) != -1 && rc == 0) {
#else
    while ((c = getopt(argc, argv, "pf:d:auh")) != -1 && rc == 0) {
#endif
        switch (c) {
        case 0:
            break;
        case 'a':
            opts->flags &= ~MODE_UTF8;
            break;
        case 'u':
            opts->flags |= MODE_UTF8;
            break;
        case 'p':
            opts->flags |= MODE_PRESERVE;
            break;
        case 'd':
            opts->dlm = *optarg;
            break;
        case 'f':
            opts->fields = optarg;
            break;
        case 'h':
        case '?':
            rc = -1;
            break;
        default:
            abort();
        }
    }

    if (rc == 0 && optind < argc) {
        opts->filename = argv[optind];
        optind += 1;
    }

    if (optind < argc) {
        fprintf(stderr, "Invalid argument after filename: %s\n", argv[optind]);
        rc = -1;
    }

    return rc;
}

void print_usage() {
    printf("Convert file with fixed columns into csv.\n");
    printf("Usage: fix2csv [options] file\n");
    printf("\n");
    printf("Options:\n\n");
    printf("--delimiter, -d del\t Use <del> as delimiter char (default=';') \n");
    printf("--preserve, -p\t\t Preserve blanks (default: strip blanks)\n");
    printf("--ascii, -a\t\t ASCII format\n");
    printf("--utf8, -u\t\t UTF-8 format\n");
    printf("--fields, -f\t\t Field list separated by commas.\n");
    printf("\t\t\t Field number starts with 1. Field description can be single column (3), range (5-8) or suffix(4-)\n");
}
