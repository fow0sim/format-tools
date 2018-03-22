#ifndef CSV2NV_H
#define CSV2NV_H

#define DEFAULT_DLM_1           ';'
#define DEFAULT_DLM_2           '\0'

#define FORMAT_LDIF             1
#define FORMAT_XML              2
#define FORMAT_JSON             3

typedef struct _CSVValue {
   struct _CSVValue *next;
   char *str;
} CSVValue;

typedef struct _CSVAttribute {
   struct _CSVAttribute *next;
   char *name;
   int val_count;
   CSVValue *val;
   CSVValue *val_last;
} CSVAttribute;

typedef struct _CSVControl {
   int verbose_flag;
   char dlm1;
   char dlm2;
   char format;
   int num_entries;
   char *list_id;
   char *obj_id;
   char *filename;
   int attr_count;
   CSVAttribute *attrs;
   CSVAttribute *attrs_last;
} CSVControl;

void parseValues(CSVControl *ctrl, char *buf);
void parseMultiValues(CSVControl *ctrl, CSVAttribute *attr, char *buffer);
void printHeader(CSVControl *ctrl);
void printFooter(CSVControl *ctrl);
void printValues(CSVControl *ctrl);
void printValuesLDIF(CSVControl *ctrl);
void printValuesXML(CSVControl *ctrl);
void printValuesJSON(CSVControl *ctrl);
void addAttribute(CSVControl *ctrl, char *attr);
void addValue(CSVAttribute *attr, char *str);
void releaseAttributes(CSVControl *ctrl);
void releaseValues(CSVControl *ctrl);
void releaseAttrValues(CSVAttribute *attr);
int parse_options(CSVControl *ctrl, int argc, char **argv);
void print_usage();

#endif

