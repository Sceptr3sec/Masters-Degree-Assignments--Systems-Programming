#include "blockchain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* str_tolower(char *str) {
    for (char *p = str; *p; p++) {
        *p = tolower(*p);
    }
    return str;
}

char* trim_whitespace(char *str) {
    while (isspace(*str)) str++;
    
    if (*str == 0) return str;
    
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    
    *(end + 1) = '\0';
    return str;
}

void display_prompt(Blockchain *bc) {
    char sync_char = bc->is_synchronized ? 's' : '-';
    printf("[%c%d]>", sync_char, bc->node_count);
    fflush(stdout);
}

static void print_error(int error_code) {
    switch (error_code) {
        case ERR_NO_RESOURCES:
            printf("NOK: 1: no more resources available on the computer\n");
            break;
        case ERR_NODE_EXISTS:
            printf("NOK: 2: this node already exists\n");
            break;
        case ERR_BLOCK_EXISTS:
            printf("NOK: 3: this block already exists\n");
            break;
        case ERR_NODE_NOT_FOUND:
            printf("NOK: 4: node doesn't exist\n");
            break;
        case ERR_BLOCK_NOT_FOUND:
            printf("NOK: 5: block doesn't exist\n");
            break;
        case ERR_CMD_NOT_FOUND:
            printf("NOK: 6: command not found\n");
            break;
        default:
            printf("NOK: unknown error\n");
            break;
    }
}

int process_command(Blockchain *bc, const char *cmd) {
    char *cmd_copy = malloc(strlen(cmd) + 1);
    if (!cmd_copy) {
        print_error(ERR_NO_RESOURCES);
        return 1;
    }
    strcpy(cmd_copy, cmd);
    
    char *trimmed = trim_whitespace(cmd_copy);
    if (strlen(trimmed) == 0) {
        free(cmd_copy);
        return 1;
    }
    
    char *tokens[100];
    int token_count = 0;
    
    char *token = strtok(trimmed, " \t\n");
    while (token && token_count < 100) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    if (token_count == 0) {
        free(cmd_copy);
        return 1;
    }
    
    str_tolower(tokens[0]);
    
    if (strcmp(tokens[0], "quit") == 0) {
        free(cmd_copy);
        return 0;
    }
    
    if (strcmp(tokens[0], "ls") == 0) {
        int verbose = 0;
        if (token_count > 1 && strcmp(tokens[1], "-l") == 0) {
            verbose = 1;
        }
        blockchain_list(bc, verbose);
        free(cmd_copy);
        return 1;
    }
    
    if (strcmp(tokens[0], "sync") == 0) {
        int result = blockchain_sync(bc);
        if (result == 0) {
            printf("OK\n");
        } else {
            print_error(result);
        }
        free(cmd_copy);
        return 1;
    }
    
    if (strcmp(tokens[0], "add") == 0) {
        if (token_count < 3) {
            print_error(ERR_CMD_NOT_FOUND);
            free(cmd_copy);
            return 1;
        }
        
        str_tolower(tokens[1]);
        
        if (strcmp(tokens[1], "node") == 0) {
            int nid = atoi(tokens[2]);
            int result = blockchain_add_node(bc, nid);
            if (result == 0) {
                printf("OK\n");
            } else {
                print_error(result);
            }
        } else if (strcmp(tokens[1], "block") == 0) {
            if (token_count < 4) {
                print_error(ERR_CMD_NOT_FOUND);
                free(cmd_copy);
                return 1;
            }
            
            char *bid = tokens[2];
            int all = 0;
            int nids[100];
            int nid_count = 0;
            
            for (int i = 3; i < token_count; i++) {
                if (strcmp(tokens[i], "*") == 0) {
                    all = 1;
                    break;
                }
                nids[nid_count++] = atoi(tokens[i]);
            }
            
            int result = blockchain_add_block(bc, bid, nids, nid_count, all);
            if (result == 0) {
                printf("OK\n");
            } else {
                print_error(result);
            }
        } else {
            print_error(ERR_CMD_NOT_FOUND);
        }
        
        free(cmd_copy);
        return 1;
    }
    
    if (strcmp(tokens[0], "rm") == 0) {
        if (token_count < 3) {
            print_error(ERR_CMD_NOT_FOUND);
            free(cmd_copy);
            return 1;
        }
        
        str_tolower(tokens[1]);
        
        if (strcmp(tokens[1], "node") == 0) {
            int all = 0;
            int nids[100];
            int nid_count = 0;
            
            for (int i = 2; i < token_count; i++) {
                if (strcmp(tokens[i], "*") == 0) {
                    all = 1;
                    break;
                }
                nids[nid_count++] = atoi(tokens[i]);
            }
            
            int result = blockchain_remove_nodes(bc, nids, nid_count, all);
            if (result == 0) {
                printf("OK\n");
            } else {
                print_error(result);
            }
        } else if (strcmp(tokens[1], "block") == 0) {
            if (token_count < 4) {
                print_error(ERR_CMD_NOT_FOUND);
                free(cmd_copy);
                return 1;
            }
            
            char *bid = tokens[2];
            int all = 0;
            int nids[100];
            int nid_count = 0;
            
            for (int i = 3; i < token_count; i++) {
                if (strcmp(tokens[i], "*") == 0) {
                    all = 1;
                    break;
                }
                nids[nid_count++] = atoi(tokens[i]);
            }
            
            int result = blockchain_remove_block(bc, bid, nids, nid_count, all);
            if (result == 0) {
                printf("OK\n");
            } else {
                print_error(result);
            }
        } else {
            print_error(ERR_CMD_NOT_FOUND);
        }
        
        free(cmd_copy);
        return 1;
    }
    
    print_error(ERR_CMD_NOT_FOUND);
    free(cmd_copy);
    return 1;
}