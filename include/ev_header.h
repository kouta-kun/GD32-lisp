#ifndef EVALUATION_HEADER
#define EVALUATION_HEADER
#include "se_header.h"
typedef struct se_node (*se_function)(struct se_list *argument_list);
extern struct se_node eval_object(struct se_node *node);
extern void log_object(struct se_node *rootw, char *logBuffer);
extern void delete_object(struct se_node *rootw);
extern void init_map();

struct map_entry
{
    uint32_t key;
    se_function function;
    struct map_entry *left;
    struct map_entry *right;
};

struct function_table
{
    struct map_entry *entries;
    int entry_count;
    int entry_size;
};
#endif