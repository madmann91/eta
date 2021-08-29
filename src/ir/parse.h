#ifndef FU_IR_PARSE_H
#define FU_IR_PARSE_H

#include <stddef.h>

struct ir_node;
struct ir_module;
struct log;
struct mem_pool;

struct ir_node* parse_ir(
    struct log* log,
    struct ir_module* module,
    struct mem_pool* mem_pool,
    const char* data_ptr,
    size_t data_size,
    const char* file_name);

#endif
