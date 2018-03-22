#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if LONG_OPTS
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include "csv2nv.h"

int main(int argc, char **argv) {
    int        ix;
    int        rc;
    char       buffer[4096];
    int        buflen = sizeof(buffer);
    CSVControl ctrl   = { .verbose_flag = 0, .dlm1 = DEFAULT_DLM_1, .dlm2 = DEFAULT_DLM_2, 
        .format = FORMAT_LDIF, .num_entries = 0, .list_id = NULL, .obj_id = NULL,
        .filename = NULL, .attr_count = 0, .attrs = NULL, .attrs_last = NULL };
    char       *sep;
    FILE       *fp;

    rc = parse_options(&ctrl, argc, argv);
    if (rc == -1) {
        print_usage();
        return 1;
    }
    if (ctrl.attr_count == 0) {
        fprintf(stderr, "Attributes list missing\n");
        exit(1);
    }
    if (ctrl.dlm1 == ctrl.dlm2) {
        fprintf(stderr, "Delimiter fur multivalued fields must be different from field delimiter\n");
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
    printHeader(&ctrl);
    while (fgets(buffer, buflen, fp) != NULL) {
        sep = strchr(buffer, '\n');
        if (sep) *sep = '\0';
        parseValues(&ctrl, buffer);
        printValues(&ctrl);
        releaseValues(&ctrl);
    }
    fclose(fp);
    printFooter(&ctrl);
    releaseAttributes(&ctrl);
}

void parseValues(CSVControl *ctrl, char *buffer) {
    char tokens[8];
    char *p;
    char *end_val;
    CSVAttribute *attr;

    attr = ctrl->attrs;
    sprintf(tokens, "%c", ctrl->dlm1);
    p = (char *)strtok_r(buffer, tokens, &end_val);
    while (p != NULL) {
        if ((attr != NULL)) {
            parseMultiValues(ctrl, attr, p);
            attr = (struct _CSVAttribute *)attr->next;
        }
        p = (char *)strtok_r(NULL, tokens, &end_val);
    };
}

void parseMultiValues(CSVControl *ctrl, CSVAttribute *attr, char *buffer) {
    char tokens[8];
    char *p;
    char *end_mv;

    if (ctrl->dlm2 == 0) {
        addValue(attr, buffer);
    } else {
        sprintf(tokens, "%c", ctrl->dlm2);
        p = (char *)strtok_r(buffer, tokens, &end_mv);
        while (p != NULL) {
            addValue(attr, p);
            p = (char *)strtok_r(NULL, tokens, &end_mv);
        }
    }
}

void printHeader(CSVControl *ctrl) {
    switch (ctrl->format) {
    case FORMAT_LDIF:
        printf("# LDIF\n");
        if (ctrl->list_id != NULL) {
            printf("# List ID: %s\n", ctrl->list_id);
        }
        if (ctrl->obj_id != NULL) {
            printf("# Object ID: %s\n", ctrl->obj_id);
        }
        printf("\n");
        break;
    case FORMAT_XML:
        if (ctrl->list_id != NULL) {
            printf("<%s>\n", ctrl->list_id);
        }
        break;
    case FORMAT_JSON:
        if (ctrl->list_id != NULL) {
            printf("{\n\"%s\": [\n", ctrl->list_id);
        }
        break;
    }
}

void printFooter(CSVControl *ctrl) {
    switch (ctrl->format) {
    case FORMAT_LDIF:
        printf("# numEntries: %d\n", ctrl->num_entries);
        break;
    case FORMAT_XML:
        if (ctrl->list_id != NULL) {
            printf("</%s>\n", ctrl->list_id);
        }
        break;
    case FORMAT_JSON:
        if (ctrl->list_id != NULL) {
            printf("\n]\n}");
        }
        printf("\n");
        break;
    }
}

void printValues(CSVControl *ctrl) {
    switch (ctrl->format) {
    case FORMAT_LDIF:
        printValuesLDIF(ctrl);
        break;
    case FORMAT_XML:
        printValuesXML(ctrl);
        break;
    case FORMAT_JSON:
        printValuesJSON(ctrl);
        break;
    }
    ctrl->num_entries += 1;
}

void printValuesLDIF(CSVControl *ctrl) {
    CSVAttribute *attr;
    CSVValue *val;

    attr = ctrl->attrs;
    while (attr != NULL) {
        val = attr->val;
        while (val != NULL) {
            printf("%s: %s\n", attr->name, val->str);
            val = (struct _CSVValue *)val->next;
        }
        attr = (struct _CSVAttribute *)attr->next;
    }
    printf("\n");
}

void printValuesXML(CSVControl *ctrl) {
    CSVAttribute *attr;
    CSVValue *val;

    attr = ctrl->attrs;
    if (ctrl->obj_id != NULL) {
        printf("<%s>", ctrl->obj_id);
    }
    while (attr != NULL) {
        val = attr->val;
        while (val != NULL) {
            printf("<%s>%s</%s>", attr->name, val->str, attr->name);
            val = (struct _CSVValue *)val->next;
        }
        attr = (struct _CSVAttribute *)attr->next;
    }
    if (ctrl->obj_id != NULL) {
        printf("</%s>", ctrl->obj_id);
    }
    printf("\n");
}

void printValuesJSON(CSVControl *ctrl) {
    CSVAttribute *attr;
    CSVValue *val;
    char bstr[4];
    char cstr[4];
    char mv_flag;

    *cstr = '\0';
    attr = ctrl->attrs;
    if (ctrl->num_entries > 0) {
        if (ctrl->list_id != NULL) {
            printf(",");
        }
        printf("\n");
    }
    printf("{ ");
    while (attr != NULL) {
        val = attr->val;
        if (val) {
            printf("%s\"%s\": ", cstr, attr->name);
            mv_flag = (val->next) ? 1 : 0;
            if (mv_flag) {
                printf("[");
            }
            *bstr = '\0';
            while (val != NULL) {
                printf("%s \"%s\"", bstr, val->str);
                sprintf(bstr, "%s", ", ");
                sprintf(cstr, "%s", ", ");
                val = (struct _CSVValue *)val->next;
            }
            if (mv_flag) {
                printf(" ]");
            }
        }
        attr = (struct _CSVAttribute *)attr->next;
    }
    printf(" }");
}

void addValue(CSVAttribute *attr, char *str) {
    CSVValue *entry;
    entry = (CSVValue *)malloc(sizeof(CSVValue));
    entry->str = (char *)strdup(str);
    entry->next = NULL;
    if (attr->val == NULL) {
        attr->val = entry;
    } else {
        attr->val_last->next = (struct _CSVValue *)entry;
    }
    attr->val_last = entry;
    attr->val_count += 1;
}

void addAttribute(CSVControl *ctrl, char *attr) {
    CSVAttribute *entry;
    entry = (CSVAttribute *)malloc(sizeof(CSVAttribute));
    entry->name = (char *)strdup(attr);
    entry->next = NULL;
    entry->val_count = 0;
    entry->val = NULL;
    entry->val_last = NULL;
    if (ctrl->attrs == NULL) {
        ctrl->attrs = entry;
    } else {
        ctrl->attrs_last->next = (struct _CSVAttribute *)entry;
    }
    ctrl->attrs_last = entry;
    ctrl->attr_count += 1;
}

void releaseAttributes(CSVControl *ctrl) {
    CSVValue *val;
    CSVAttribute *entry;
    while (ctrl->attrs != NULL) {
        entry = ctrl->attrs;
        releaseAttrValues(entry);
        free(entry->name);
        ctrl->attrs = (CSVAttribute *)entry->next;
        free(entry);
    }
    ctrl->attrs_last = NULL;
    ctrl->attr_count = 0;
}

void releaseValues(CSVControl *ctrl) {
    CSVValue *val;
    CSVAttribute *entry;
    entry = ctrl->attrs;
    while (entry != NULL) {
        releaseAttrValues(entry);
        entry = (CSVAttribute *)entry->next;
    }
}

void releaseAttrValues(CSVAttribute *attr) {
    CSVValue *entry;
    while (attr->val != NULL) {
        entry = attr->val;
        free(entry->str);
        attr->val = entry->next;
        free(entry);
    }
    attr->val_last = NULL;
    attr->val_count = 0;
}


int parse_options(CSVControl *ctrl, int argc, char **argv) {
#if LONG_OPTS
    struct option long_options[] = {
        { "verbose", no_argument, &ctrl->verbose_flag, 1 },
        { "brief", no_argument, &ctrl->verbose_flag, 0 },
        { "help", no_argument, 0, 'h' },
        { "file", required_argument, 0, 'f' },
        { "delimiter", required_argument, 0, 'd' },
        { "multivalue-delimiter", required_argument, 0, 'm' },
        { "list-id", required_argument, 0, 'l' },
        { "object-id", required_argument, 0, 'o' },
        { "format", required_argument, 0, 't' },
        { "type", required_argument, 0, 't' },
        { 0, 0, 0, 0 }
    };
    int option_index = 0;
#endif
    int c;
    int rc;

    rc = 0;
#if LONG_OPTS
    while ((c = getopt_long(argc, argv, "hf:d:m:t:l:o:", long_options, &option_index)) != -1 && rc == 0) {
#else
    while ((c = getopt(argc, argv, "vhf:d:m:t:l:o:")) != -1 && rc == 0) {
#endif
        switch (c) {
        case 0:
            break;
        case 'f':
            if (*optarg != '-') {
                ctrl->filename = optarg;
            }
            break;
        case 'd':
            ctrl->dlm1 = *optarg;
            break;
        case 'm':
            ctrl->dlm2 = *optarg;
            break;
        case 'l':
            ctrl->list_id = (char *)optarg;
            break;
        case 'o':
            ctrl->obj_id = (char *)optarg;
            break;
        case 't':
            if (strcasecmp(optarg, "ldif") == 0) {
                ctrl->format = FORMAT_LDIF;
            } else if (strcasecmp(optarg, "xml") == 0) {
                ctrl->format = FORMAT_XML;
            } else if (strcasecmp(optarg, "json") == 0) {
                ctrl->format = FORMAT_JSON;
            } else {
                fprintf(stderr, "Unknown output format type: %s\n", optarg);
                rc = -1;
            }
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
        addAttribute(ctrl, argv[optind]);
        optind += 1;
    }

    return rc;
}

void print_usage() {
    printf("Convert csv file into various name=value formats.\n");
    printf("Usage: csv2nv [options] [[attr] ...]\n");
    printf("\n");
    printf("Attributes list can be wrapped in an object id.\n");
    printf("Objects can be wrapped in a list id.\n");
    printf("\n");
    printf("Options:\n");
    printf("--file, -f file\t\t\t File name ('-' for stdin).\n");
    printf("--delimiter, -d del\t\t Use <del> as delimiter char (default=';')\n");
    printf("--multivalue-delimiter, -m del\t Use <del> as delimiter char for multivalued fields (default=none)\n");
    printf("--format, --type, -t format\t Format type (ldif, xml, json)\n");
    printf("--object-id, -o id\t\t Object ID name\n");
    printf("--list-id, -l id\t\t List ID name\n");
    printf("--brief\t\t\t\t Brief mode\n");
    printf("--verbose\t\t\t Verbose mode\n");
}
