#ifndef FIX2CSV_H
#define FIX2CSV_H

#define MODE_PRESERVE               1
#define MODE_UTF8                   2

#define DEFAULT_DELIMITER          ';'

typedef struct _FIELD {
   struct _FIELD *next;
   int pos;
   int len;
} FIELD;

typedef struct _opt_t {
    int flags;
    char dlm;
    char *fields;
    char *filename;
} opt_t;

void copy_fields(FIELD *fields, opt_t *opts);
void parse_fields(FIELD **fields, const char *str);
void dealloc_fields(FIELD **fields);
char *utf8index(char *s, int pos);
int utf8len(char *s, int len);
void trim_whitespace(char *s);
int parse_options(opt_t *opts, int argc, char **argv);
void print_usage();

#endif

