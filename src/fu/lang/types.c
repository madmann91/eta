#include "fu/lang/types.h"
#include "fu/core/mem_pool.h"
#include "fu/core/hash.h"
#include "fu/core/utils.h"

#include <assert.h>
#include <string.h>

#define DEFAULT_TYPE_TABLE_CAPACITY 16

bool is_prim_type(TypeTag tag) {
    switch (tag) {
#define f(name, ...) case TYPE_##name:
        AST_PRIM_TYPE_LIST(f)
#undef f
            return true;
        default:
            return false;
    }
}

void set_member_name(TypeTable* type_table, Type* type, size_t i, const char* name) {
    assert(i < type->struct_type.member_count);
    type->struct_type.member_names[i] = make_str(&type_table->str_pool, name);
}

TypeTable new_type_table(MemPool* mem_pool) {
    return (TypeTable) {
        .types = new_hash_table(DEFAULT_TYPE_TABLE_CAPACITY, sizeof(Type*)),
        .str_pool = new_str_pool(mem_pool),
        .mem_pool = mem_pool
    };
}

void free_type_table(TypeTable* type_table) {
    free_hash_table(&type_table->types);
    free_str_pool(&type_table->str_pool); 
}

static Type* make_struct_or_enum_type(TypeTable* type_table, TypeTag tag, const char* name, size_t member_count) {
    Type* type = alloc_from_mem_pool(type_table->mem_pool, sizeof(Type));
    type->tag = tag;
    type->struct_type.name = make_str(&type_table->str_pool, name);
    type->struct_type.members = alloc_from_mem_pool(type_table->mem_pool, sizeof(Type*) * member_count);
    type->struct_type.member_names = alloc_from_mem_pool(type_table->mem_pool, sizeof(char*) * member_count);
    type->struct_type.member_count = member_count;
    type->struct_type.child_types = NULL;
    type->id = type_table->type_count++;
    return type;
}

Type* make_struct_type(TypeTable* type_table, const char* name, size_t field_count) {
    return make_struct_or_enum_type(type_table, TYPE_STRUCT, name, field_count);
}

Type* make_enum_type(TypeTable* type_table, const char* name, size_t option_count) {
    return make_struct_or_enum_type(type_table, TYPE_ENUM, name, option_count);
}

static uint32_t hash_type(uint32_t hash, const Type* type) {
    hash = hash_uint32(hash, type->tag);
    switch (type->tag) {
        case TYPE_TUPLE:
            for (size_t i = 0; i < type->tuple_type.arg_count; ++i)
                hash = hash_uint64(hash, type->tuple_type.arg_types[i]->id);
            break;
        case TYPE_FUN:
            hash = hash_uint64(hash, type->fun_type.dom_type->id);
            hash = hash_uint64(hash, type->fun_type.codom_type->id);
            break;
        case TYPE_ARRAY:
            hash = hash_uint64(hash, type->array_type.elem_type->id);
            break;
        case TYPE_PARAM:
            hash = hash_str(hash, type->type_param.name);
            break;
        default:
            break;
    }
    return hash;
}

static bool compare_types(const void* left, const void* right) {
    const Type* type_left = *(const Type**)left;
    const Type* type_right = *(const Type**)right;
    if (type_left->tag != type_right->tag)
        return false;
    switch (type_left->tag) {
        case TYPE_TUPLE:
            for (size_t i = 0; i < type_left->tuple_type.arg_count; ++i) {
                if (type_left->tuple_type.arg_types[i] != type_right->tuple_type.arg_types[i])
                    return false;
            }
            break;
        case TYPE_FUN:
            return
                type_left->fun_type.dom_type == type_right->fun_type.dom_type &&
                type_left->fun_type.codom_type == type_right->fun_type.codom_type;
        case TYPE_ARRAY:
            return type_left->array_type.elem_type == type_right->array_type.elem_type;
        case TYPE_PARAM:
            return type_left->type_param.name == type_right->type_param.name;
        default:
            break;
    }
    return true;
}

static const Type* get_or_insert_type(TypeTable* type_table, const Type* type) {
    uint32_t hash = hash_type(hash_init(), type);
    const Type** type_ptr = find_in_hash_table(&type_table->types, &type, hash, sizeof(Type*), compare_types);
    if (type_ptr)
        return *type_ptr;
    Type* new_type = alloc_from_mem_pool(type_table->mem_pool, sizeof(Type));
    memcpy(new_type, type, sizeof(Type));
    new_type->id = type_table->type_count++;
    if (type->tag == TYPE_TUPLE) {
        size_t args_size = sizeof(Type*) * type->tuple_type.arg_count;
        new_type->tuple_type.arg_types = alloc_from_mem_pool(type_table->mem_pool, args_size);
        memcpy(new_type->tuple_type.arg_types, type->tuple_type.arg_types, args_size);
    }
    must_succeed(insert_in_hash_table(&type_table->types, &new_type, hash, sizeof(Type*), compare_types));
    return new_type;
}

const Type* make_prim_type(TypeTable* type_table, TypeTag tag) {
    assert(is_prim_type(tag));
    return get_or_insert_type(type_table, &(Type) { .tag = tag });
}

const Type* make_unknown_type(TypeTable* type_table) {
    return get_or_insert_type(type_table, &(Type) { .tag = TYPE_UNKNOWN });
}

const Type* make_type_param(TypeTable* type_table, const char* name) {
    name = make_str(&type_table->str_pool, name);
    return get_or_insert_type(type_table, &(Type) { .tag = TYPE_PARAM, .type_param.name = name });
}

const Type* make_tuple_type(TypeTable* type_table, const Type** arg_types, size_t arg_count) {
    return get_or_insert_type(type_table, &(Type) {
        .tag = TYPE_TUPLE,
        .tuple_type = { .arg_types = arg_types, .arg_count = arg_count }
    });
}

const Type* make_fun_type(TypeTable* type_table, const Type* dom_type, const Type* codom_type) {
    return get_or_insert_type(type_table, &(Type) {
        .tag = TYPE_FUN,
        .fun_type = { .dom_type = dom_type, .codom_type = codom_type }
    });
}

const Type* make_array_type(TypeTable* type_table, const Type* elem_type) {
    return get_or_insert_type(type_table, &(Type) {
        .tag = TYPE_ARRAY,
        .array_type = { .elem_type = elem_type }
    });
}

void print_type(FormatState* state, const Type* type) {
    switch (type->tag) {
#define f(name, str) case TYPE_##name: print_keyword(state, str); break;
        AST_PRIM_TYPE_LIST(f);
        case TYPE_UNKNOWN:
            format(state, "?", NULL);
            break;
        case TYPE_TUPLE:
            format(state, "(", NULL);
            for (size_t i = 0, n = type->tuple_type.arg_count; i < n; ++i) {
                print_type(state, type->tuple_type.arg_types[i]);
                if (i != n - 1)
                    format(state, ", ", NULL);
            }
            format(state, ")", NULL);
            break;
        case TYPE_ARRAY:
            format(state, "[", NULL);
            print_type(state, type->array_type.elem_type);
            format(state, "]", NULL);
            break;
        case TYPE_PARAM:
            format(state, "{s}", (FormatArg[]) { { .s = type->type_param.name } });
            break;
        case TYPE_FUN:
            print_keyword(state, "fun");
            format(state, " ",  NULL);
            if (type->fun_type.dom_type->tag == TYPE_TUPLE)
                print_type(state, type->fun_type.dom_type);
            else {
                format(state, "(", NULL);
                print_type(state, type->fun_type.dom_type);
                format(state, ")", NULL);
            }
            format(state, " -> ", NULL);
            print_type(state, type->fun_type.codom_type);
            break;
        case TYPE_ENUM:
            print_keyword(state, "enum");
            format(state, " {s}", (FormatArg[]) { { .s = type->enum_type.name } });
            break;
        case TYPE_STRUCT:
            print_keyword(state, "struct");
            format(state, " {s}", (FormatArg[]) { { .s = type->struct_type.name } });
            break;
        default:
            assert(false && "invalid type");
            break;
    }
}

void dump_type(const Type* type) {
    FormatState state = new_format_state("    ", !is_color_supported(stdout));
    print_type(&state, type);
    write_format_state(&state, stdout);
    free_format_state(&state);
    printf("\n");
}
