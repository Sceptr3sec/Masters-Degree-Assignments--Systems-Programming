#include "blockchain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Blockchain* blockchain_create(void) {
    Blockchain *bc = malloc(sizeof(Blockchain));
    if (!bc) return NULL;
    
    bc->nodes = NULL;
    bc->node_count = 0;
    bc->is_synchronized = 1;
    
    return bc;
}

void blockchain_destroy(Blockchain *bc) {
    if (!bc) return;
    
    Node *current = bc->nodes;
    while (current) {
        Node *next_node = current->next;
        
        Block *block = current->blocks;
        while (block) {
            Block *next_block = block->next;
            free(block->bid);
            free(block);
            block = next_block;
        }
        
        free(current);
        current = next_node;
    }
    
    free(bc);
}

Node* blockchain_find_node(Blockchain *bc, int nid) {
    Node *current = bc->nodes;
    while (current) {
        if (current->nid == nid) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int blockchain_block_exists_globally(Blockchain *bc, const char *bid) {
    Node *node = bc->nodes;
    while (node) {
        if (node_has_block(node, bid)) {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

int node_has_block(Node *node, const char *bid) {
    Block *block = node->blocks;
    while (block) {
        if (strcmp(block->bid, bid) == 0) {
            return 1;
        }
        block = block->next;
    }
    return 0;
}

int blockchain_add_node(Blockchain *bc, int nid) {
    if (blockchain_find_node(bc, nid)) {
        return ERR_NODE_EXISTS;
    }
    
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        return ERR_NO_RESOURCES;
    }
    
    new_node->nid = nid;
    new_node->blocks = NULL;
    new_node->block_count = 0;
    new_node->next = NULL;
    
    /* Add to TAIL to preserve insertion order */
    if (bc->nodes == NULL) {
        bc->nodes = new_node;
    } else {
        Node *current = bc->nodes;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    
    bc->node_count++;
    
    return 0;
}

int blockchain_remove_nodes(Blockchain *bc, int *nids, int count, int all) {
    if (all) {
        Node *current = bc->nodes;
        while (current) {
            Node *next = current->next;
            
            Block *block = current->blocks;
            while (block) {
                Block *next_block = block->next;
                free(block->bid);
                free(block);
                block = next_block;
            }
            
            free(current);
            current = next;
        }
        bc->nodes = NULL;
        bc->node_count = 0;
        bc->is_synchronized = 1;
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        Node *prev = NULL;
        Node *current = bc->nodes;
        int found = 0;
        
        while (current) {
            if (current->nid == nids[i]) {
                found = 1;
                
                Block *block = current->blocks;
                while (block) {
                    Block *next_block = block->next;
                    free(block->bid);
                    free(block);
                    block = next_block;
                }
                
                if (prev) {
                    prev->next = current->next;
                } else {
                    bc->nodes = current->next;
                }
                
                free(current);
                bc->node_count--;
                break;
            }
            prev = current;
            current = current->next;
        }
        
        if (!found) {
            return ERR_NODE_NOT_FOUND;
        }
    }
    
    return 0;
}

int node_add_block(Node *node, const char *bid) {
    Block *new_block = malloc(sizeof(Block));
    if (!new_block) {
        return ERR_NO_RESOURCES;
    }
    
    new_block->bid = malloc(strlen(bid) + 1);
    if (!new_block->bid) {
        free(new_block);
        return ERR_NO_RESOURCES;
    }
    
    strcpy(new_block->bid, bid);
    new_block->next = NULL;
    
    /* Add to TAIL to preserve block insertion order */
    if (node->blocks == NULL) {
        node->blocks = new_block;
    } else {
        Block *current = node->blocks;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_block;
    }
    
    node->block_count++;
    
    return 0;
}

int blockchain_add_block(Blockchain *bc, const char *bid, int *nids, int count, int all) {
    if (all) {
        if (bc->node_count == 0) {
            return ERR_NODE_NOT_FOUND;
        }
        
        Node *node = bc->nodes;
        while (node) {
            if (!node_has_block(node, bid)) {
                int result = node_add_block(node, bid);
                if (result != 0) {
                    return result;
                }
            }
            node = node->next;
        }
        
        /* When adding to ALL nodes, chain stays synchronized */
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        Node *node = blockchain_find_node(bc, nids[i]);
        if (!node) {
            return ERR_NODE_NOT_FOUND;
        }
        
        if (node_has_block(node, bid)) {
            return ERR_BLOCK_EXISTS;
        }
        
        int result = node_add_block(node, bid);
        if (result != 0) {
            return result;
        }
    }
    
    /* Only mark unsynchronized if multiple nodes exist */
    /* A single node is always synchronized with itself */
    if (bc->node_count > 1) {
        bc->is_synchronized = 0;
    }
    
    return 0;
}

void node_remove_block(Node *node, const char *bid) {
    Block *prev = NULL;
    Block *current = node->blocks;
    
    while (current) {
        if (strcmp(current->bid, bid) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                node->blocks = current->next;
            }
            
            free(current->bid);
            free(current);
            node->block_count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

int blockchain_remove_block(Blockchain *bc, const char *bid, int *nids, int count, int all) {
    int found_any = 0;
    
    if (all) {
        Node *node = bc->nodes;
        while (node) {
            if (node_has_block(node, bid)) {
                node_remove_block(node, bid);
                found_any = 1;
            }
            node = node->next;
        }
        
        if (!found_any) {
            return ERR_BLOCK_NOT_FOUND;
        }
        
        /* Only mark unsynchronized if multiple nodes exist */
        if (bc->node_count > 1) {
            bc->is_synchronized = 0;
        }
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        Node *node = blockchain_find_node(bc, nids[i]);
        if (!node) {
            return ERR_NODE_NOT_FOUND;
        }
        
        if (!node_has_block(node, bid)) {
            return ERR_BLOCK_NOT_FOUND;
        }
        
        node_remove_block(node, bid);
        found_any = 1;
    }
    
    /* Only mark unsynchronized if multiple nodes exist */
    if (bc->node_count > 1) {
        bc->is_synchronized = 0;
    }
    return 0;
}

int blockchain_sync(Blockchain *bc) {
    if (bc->node_count == 0) {
        printf("Warning: No nodes to synchronize\n");
        return 0;
    }
    
    char **all_bids = NULL;
    int bid_count = 0;
    int bid_capacity = 0;
    
    Node *node = bc->nodes;
    while (node) {
        Block *block = node->blocks;
        while (block) {
            int already_counted = 0;
            for (int i = 0; i < bid_count; i++) {
                if (strcmp(all_bids[i], block->bid) == 0) {
                    already_counted = 1;
                    break;
                }
            }
            
            if (!already_counted) {
                if (bid_count >= bid_capacity) {
                    bid_capacity = bid_capacity == 0 ? 10 : bid_capacity * 2;
                    char **new_bids = realloc(all_bids, bid_capacity * sizeof(char*));
                    if (!new_bids) {
                        for (int i = 0; i < bid_count; i++) {
                            free(all_bids[i]);
                        }
                        free(all_bids);
                        return ERR_NO_RESOURCES;
                    }
                    all_bids = new_bids;
                }
                
                all_bids[bid_count] = malloc(strlen(block->bid) + 1);
                if (!all_bids[bid_count]) {
                    for (int i = 0; i < bid_count; i++) {
                        free(all_bids[i]);
                    }
                    free(all_bids);
                    return ERR_NO_RESOURCES;
                }
                strcpy(all_bids[bid_count], block->bid);
                bid_count++;
            }
            
            block = block->next;
        }
        node = node->next;
    }
    
    /* Add blocks to each node in REVERSE order */
    node = bc->nodes;
    while (node) {
        for (int i = bid_count - 1; i >= 0; i--) {
            if (!node_has_block(node, all_bids[i])) {
                node_add_block(node, all_bids[i]);
            }
        }
        node = node->next;
    }
    
    for (int i = 0; i < bid_count; i++) {
        free(all_bids[i]);
    }
    free(all_bids);
    
    bc->is_synchronized = 1;
    return 0;
}

void blockchain_list(Blockchain *bc, int verbose) {
    Node *node = bc->nodes;
    
    while (node) {
        printf("%d", node->nid);
        
        if (verbose) {
            printf(":");
            Block *block = node->blocks;
            int first = 1;
            while (block) {
                if (!first) {
                    printf(",");
                }
                printf(" %s", block->bid);
                first = 0;
                block = block->next;
            }
        }
        
        printf("\n");
        node = node->next;
    }
}