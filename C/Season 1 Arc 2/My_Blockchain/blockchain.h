#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <stddef.h>

/* Structures */
typedef struct Block {
    char *bid;
    struct Block *next;
} Block;

typedef struct Node {
    int nid;
    Block *blocks;
    int block_count;
    struct Node *next;
} Node;

typedef struct Blockchain {
    Node *nodes;
    int node_count;
    int is_synchronized;
} Blockchain;

/* Error codes */
#define ERR_NO_RESOURCES 1
#define ERR_NODE_EXISTS 2
#define ERR_BLOCK_EXISTS 3
#define ERR_NODE_NOT_FOUND 4
#define ERR_BLOCK_NOT_FOUND 5
#define ERR_CMD_NOT_FOUND 6

/* Function declarations - blockchain operations */
Blockchain* blockchain_create(void);
void blockchain_destroy(Blockchain *bc);
int blockchain_add_node(Blockchain *bc, int nid);
int blockchain_remove_nodes(Blockchain *bc, int *nids, int count, int all);
int blockchain_add_block(Blockchain *bc, const char *bid, int *nids, int count, int all);
int blockchain_remove_block(Blockchain *bc, const char *bid, int *nids, int count, int all);
int blockchain_sync(Blockchain *bc);
void blockchain_list(Blockchain *bc, int verbose);
Node* blockchain_find_node(Blockchain *bc, int nid);
int blockchain_block_exists_globally(Blockchain *bc, const char *bid);
int node_has_block(Node *node, const char *bid);
int node_add_block(Node *node, const char *bid);
void node_remove_block(Node *node, const char *bid);

/* Function declarations - backup/restore */
int blockchain_save(Blockchain *bc, const char *filename);
Blockchain* blockchain_load(const char *filename);
int blockchain_save_text(Blockchain *bc, const char *filename);

/* Function declarations - command processing */
int process_command(Blockchain *bc, const char *cmd);
void display_prompt(Blockchain *bc);
char* str_tolower(char *str);
char* trim_whitespace(char *str);

#endif