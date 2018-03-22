#ifndef LDIF2CSV_H
#define LDIF2CSV_H

#define ENC_UNDEFINED             0
#define ENC_TEXT                  1
#define ENC_BASE64                2
#define ENC_URL                   4

#define DEFAULT_DLM_1           ';'
#define DEFAULT_DLM_2           ','

typedef struct _CSVAtttribute {
   struct _CSVAttribute *next;
   char *name;
} CSVAttribute;

typedef struct _CSVKeyValue {
   struct _CSVKeyValue *next;
   char *key;
   char *value;
   int enc;
} CSVKeyValue;

typedef struct _CSVParseTree {
   int key_count;
   CSVKeyValue *keys;
   CSVKeyValue *keys_last;
} CSVParseTree;

typedef struct _CSVControl {
   char verbose_flag;
   char dlm1;
   char dlm2;
   char *filename;
   int attr_count;
   CSVAttribute *attrs;
   CSVAttribute *attrs_last;
} CSVControl;

void addAttribute(CSVControl *ctrl, char *attr);
int parseLDIFEntry(CSVKeyValue *kv_entry, char *line);
void addKeyValuePair(CSVParseTree *ldif, char *key, char *value, int enc);
void concatValue(CSVKeyValue *kv_entry, char *value);
void releaseAttributes(CSVControl *ctrl);
void releaseKeyValuePairs(CSVParseTree *ldif);
void printAttrList(CSVControl *ctrl, CSVParseTree *ldif);
int parse_options(CSVControl *ctrl, int argc, char **argv);
void print_usage();

#endif

