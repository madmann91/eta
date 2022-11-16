#include "fu/ir/node_map.h"
#include "fu/ir/node.h"

typedef struct {
    const Node* from;
    void* to;
} NodeMapPair;

NodeMap new_node_map() {
    return (NodeMap) { .hash_table = new_hash_table(sizeof(NodeMapPair)) };
}

void free_node_map(NodeMap* node_map) {
    free_hash_table(&node_map->hash_table);
}

static bool compare_node_map_pairs(const void* left, const void* right) {
    return ((const NodeMapPair*)left)->from == ((const NodeMapPair*)right)->from;
}

bool insert_in_node_map(NodeMap* node_map, const Node* from, void* to) {
    return insert_in_hash_table(&node_map->hash_table,
        &(NodeMapPair) { .from = from, .to = to }, from->id,
        sizeof(NodeMapPair), compare_node_map_pairs);
}

bool replace_in_node_map(NodeMap* node_map, const Node* from, void* to) {
    return replace_in_hash_table(&node_map->hash_table,
        &(NodeMapPair) { .from = from, .to = to }, from->id,
        sizeof(NodeMapPair), compare_node_map_pairs);
}

void* find_in_node_map(const NodeMap* node_map, const Node* node) {
    NodeMapPair* elem = find_in_hash_table(&node_map->hash_table, &(NodeMapPair) { .from = node },
        node->id, sizeof(NodeMapPair), compare_node_map_pairs);
    return elem ? elem->to : NULL;
}
