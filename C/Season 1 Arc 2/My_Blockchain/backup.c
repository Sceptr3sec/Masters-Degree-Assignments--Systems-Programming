#include "blockchain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int blockchain_save(Blockchain *bc, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    write(fd, &bc->node_count, sizeof(int));
    write(fd, &bc->is_synchronized, sizeof(int));
    
    Node *node = bc->nodes;
    while (node) {
        write(fd, &node->nid, sizeof(int));
        write(fd, &node->block_count, sizeof(int));
        
        Block *block = node->blocks;
        while (block) {
            int bid_len = strlen(block->bid);
            write(fd, &bid_len, sizeof(int));
            write(fd, block->bid, bid_len);
            block = block->next;
        }
        
        node = node->next;
    }
    
    close(fd);
    return 0;
}

Blockchain* blockchain_load(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }
    
    Blockchain *bc = blockchain_create();
    if (!bc) {
        close(fd);
        return NULL;
    }
    
    if (read(fd, &bc->node_count, sizeof(int)) != sizeof(int)) {
        blockchain_destroy(bc);
        close(fd);
        return NULL;
    }
    
    if (read(fd, &bc->is_synchronized, sizeof(int)) != sizeof(int)) {
        blockchain_destroy(bc);
        close(fd);
        return NULL;
    }
    
    Node *last_node = NULL;
    
    for (int i = 0; i < bc->node_count; i++) {
        Node *node = malloc(sizeof(Node));
        if (!node) {
            blockchain_destroy(bc);
            close(fd);
            return NULL;
        }
        
        if (read(fd, &node->nid, sizeof(int)) != sizeof(int)) {
            free(node);
            blockchain_destroy(bc);
            close(fd);
            return NULL;
        }
        
        if (read(fd, &node->block_count, sizeof(int)) != sizeof(int)) {
            free(node);
            blockchain_destroy(bc);
            close(fd);
            return NULL;
        }
        
        node->blocks = NULL;
        node->next = NULL;
        
        Block *last_block = NULL;
        
        for (int j = 0; j < node->block_count; j++) {
            int bid_len;
            if (read(fd, &bid_len, sizeof(int)) != sizeof(int)) {
                blockchain_destroy(bc);
                close(fd);
                return NULL;
            }
            
            char *bid = malloc(bid_len + 1);
            if (!bid) {
                blockchain_destroy(bc);
                close(fd);
                return NULL;
            }
            
            if (read(fd, bid, bid_len) != bid_len) {
                free(bid);
                blockchain_destroy(bc);
                close(fd);
                return NULL;
            }
            bid[bid_len] = '\0';
            
            Block *block = malloc(sizeof(Block));
            if (!block) {
                free(bid);
                blockchain_destroy(bc);
                close(fd);
                return NULL;
            }
            
            block->bid = bid;
            block->next = NULL;
            
            if (last_block) {
                last_block->next = block;
            } else {
                node->blocks = block;
            }
            last_block = block;
        }
        
        if (last_node) {
            last_node->next = node;
        } else {
            bc->nodes = node;
        }
        last_node = node;
    }
    
    close(fd);
    return bc;
}

int blockchain_save_text(Blockchain *bc, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    
    char buffer[1024];
    int len;
    
    len = sprintf(buffer, "=== Blockchain Text Backup ===\n");
    write(fd, buffer, len);
    
    len = sprintf(buffer, "Nodes: %d\n", bc->node_count);
    write(fd, buffer, len);
    
    len = sprintf(buffer, "Synchronized: %s\n\n", bc->is_synchronized ? "Yes" : "No");
    write(fd, buffer, len);
    
    Node *node = bc->nodes;
    while (node) {
        len = sprintf(buffer, "Node %d (%d blocks):\n", node->nid, node->block_count);
        write(fd, buffer, len);
        
        Block *block = node->blocks;
        while (block) {
            len = sprintf(buffer, "  - %s\n", block->bid);
            write(fd, buffer, len);
            block = block->next;
        }
        
        len = sprintf(buffer, "\n");
        write(fd, buffer, len);
        
        node = node->next;
    }
    
    close(fd);
    return 0;
}