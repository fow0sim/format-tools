#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if LONG_OPTS
    #include <getopt.h>
#else
    #include <unistd.h>
#endif

#include "ldif2csv.h"

int main (int argc, char **argv)
{
   int rc;
   CSVControl ctrl = { .verbose_flag = 0, .dlm1 = DEFAULT_DLM_1, .dlm2 = DEFAULT_DLM_2,
       .filename = NULL, .attr_count = 0, .attrs = NULL, .attrs_last = NULL };
   CSVParseTree ldif = { .key_count = 0, .keys = NULL, .keys_last = NULL };
   char buffer[4096];
   int buflen = sizeof(buffer);
   char *sep;
   char parse_mode = 0;
   CSVKeyValue ldifEntry = { .next = NULL, .key = NULL, .value = NULL, .enc = ENC_UNDEFINED };
   FILE *fp;

   rc = parse_options(&ctrl, argc, argv);
   if (rc == -1) {
       print_usage();
       return 0;
   }
   if (ctrl.attr_count == 0) {
      fprintf(stderr, "Attributes list missing\n");
      exit(1);
   }

   if (ctrl.filename == NULL) {
      fp = stdin;
   } else {
      if ((fp = fopen(ctrl.filename, "rt")) == NULL) {
         fprintf(stderr, "Could not open file %s\n", ctrl.filename);
         exit(1);
      }
   }

   while (fgets(buffer, buflen, fp) != NULL) {
      sep = strchr(buffer, '\n');
      if (sep) *sep = '\0';
      if (*buffer == '#') continue;

      if (parse_mode) {
         if (*buffer == ' ') {
            concatValue(ldif.keys_last, buffer + 1);
            continue;
         }
         if (strlen(buffer) == 0) {
            printAttrList(&ctrl, &ldif);
            releaseKeyValuePairs(&ldif);
            parse_mode = 0;
            continue;
         }
      }

      if (parseLDIFEntry(&ldifEntry, buffer) != 0) continue;

   if (strcasecmp(ldifEntry.key, "dn") == 0) parse_mode = 1;
      if (parse_mode) {
         addKeyValuePair(&ldif, ldifEntry.key, ldifEntry.value, ldifEntry.enc);
      }
   }

   if (parse_mode) printAttrList(&ctrl, &ldif);
   releaseKeyValuePairs(&ldif);
   releaseAttributes(&ctrl);
   fclose(fp);
}

void addAttribute(CSVControl *ctrl, char *attr)
{
   CSVAttribute *entry;
   entry = (CSVAttribute *) malloc(sizeof(CSVAttribute));
   entry->name = (char *) strdup(attr);
   entry->next = NULL;
   if (ctrl->attrs == NULL) {
      ctrl->attrs = entry;
   } else {
      ctrl->attrs_last->next = (struct _CSVAttribute *) entry;
   }
   ctrl->attrs_last = entry;
   ctrl->attr_count++;
   return;
}

int parseLDIFEntry(CSVKeyValue *kv_entry, char *line)
{
   char *sep = strchr(line, ':');
   if (!sep) return 1;

   *sep = '\0';
   kv_entry->key = line;
   kv_entry->value = sep + 1;

   switch(*kv_entry->value) {
      case ':':
         kv_entry->enc = ENC_BASE64;
         kv_entry->value++;
         break;
      case '>':
         kv_entry->enc = ENC_URL;
         kv_entry->value++;
         break;
      default:
         kv_entry->enc = ENC_TEXT;
   }
   kv_entry->value++;
   return 0;
}

void addKeyValuePair(CSVParseTree *ldif, char *key, char *value, int enc)
{
   CSVKeyValue *entry;
   entry = (CSVKeyValue *) malloc(sizeof(CSVKeyValue));
   entry->key = (char *) strdup(key);
   entry->value = (char *) strdup(value);
   entry->next = NULL;
   entry->enc = enc;
   if (ldif->keys == NULL) {
      ldif->keys = entry;
   } else {
      ldif->keys_last->next = (struct _CSVKeyValue *) entry;
   }
   ldif->keys_last = entry;
   ldif->key_count++;
   return;
}

void releaseAttributes(CSVControl *ctrl)
{
   CSVAttribute *entry;
   while (ctrl->attrs != NULL) {
      entry = ctrl->attrs;
      free(entry->name);
      ctrl->attrs = (CSVAttribute *) entry->next;
      free(entry);
   }
   ctrl->attrs_last = NULL;
   ctrl->attr_count = 0;
}

void concatValue(CSVKeyValue *kv_entry, char *value) {
   char *tmp = malloc(strlen(kv_entry->value) + strlen(value) + 1);
   strcpy(tmp, kv_entry->value);
   strcat(tmp, value);
   free(kv_entry->value);
   kv_entry->value = tmp;
}

void releaseKeyValuePairs(CSVParseTree *ldif)
{
   CSVKeyValue *entry;
   while (ldif->keys != NULL) {
      entry = ldif->keys;
      free(entry->key);
      free(entry->value);
      ldif->keys = (CSVKeyValue *) entry->next;
      free(entry);
   }
   ldif->keys_last = NULL;
   ldif->key_count = 0;
}

void printAttrList(CSVControl *ctrl, CSVParseTree *ldif)
{
   int mv_flag = 0;
   CSVAttribute *entry = ctrl->attrs;
   CSVKeyValue *kv_entry;
   while (entry != NULL) {
      if (entry != ctrl->attrs) printf("%c", ctrl->dlm1);
      mv_flag = 0;
      kv_entry = ldif->keys;
      while (kv_entry != NULL) {
         if (strcasecmp(entry->name, kv_entry->key) == 0) {
            if (mv_flag) printf("%c", ctrl->dlm2); else mv_flag = 1;
            printf("%s", kv_entry->value);
         }
         kv_entry = (CSVKeyValue *) kv_entry->next;
      }
      entry = (CSVAttribute *) entry->next;
   }
   printf("\n");
}

int parse_options(CSVControl *ctrl, int argc, char **argv) {
#if LONG_OPTS
    struct option long_options[] = {
        { "verbose", no_argument, 0, 'v' },
        { "help", no_argument, 0, 'h' },
        { "delimiter", required_argument, 0, 'd' },
        { "multivalue-delimiter", required_argument, 0, 'm' },
        { "file", required_argument, 0, 'f' },
        { 0, 0, 0, 0 }
    };
    int option_index = 0;
#endif
    int c;
    int rc;

    rc = 0;
#if LONG_OPTS
    while ((c = getopt_long(argc, argv, "vhd:m:f:", long_options, &option_index)) != -1 && rc == 0) {
#else
    while ((c = getopt(argc, argv, "vhd:m:f:")) != -1 && rc == 0) {
#endif
        switch (c) {
        case 0:
            break;
        case 'v':
            ctrl->verbose_flag = 1;
            break;
        case 'h':
            rc = -1;
            break;
        case 'd':
           ctrl->dlm1 = *optarg;
           break;
        case 'm':
           ctrl->dlm2 = *optarg;
           break;
        case 'f':
            if (*optarg != '-') {
                ctrl->filename = optarg;
            }
            break;
        case '?':
            rc = -1;
            break;
        default:
            abort();
        }
    }

    while (rc == 0 && optind < argc) {
        addAttribute(ctrl, argv[optind]);
        optind += 1;
    }

    return rc;
}

void print_usage() {
    printf("Convert attributes from LDIF format to csv\n");
    printf("Usage: ldif2csv [options] attr [...]\n\n");
    printf("Options:\n");
    printf(" --help, -h\t\t\t\t display this help page\n");
    printf(" --delimiter, -d delim\t\t\t delimiter char (default ';')\n");
    printf(" --multivalue-delimiter, -m delim\t delimiter char for multivalued fields (default ',')\n");
    printf(" --file, -f filename\t\t\t file name ('-' for stdin)\n");
    printf(" --verbose, -v\t\t\t\t be verbose\n");
}
