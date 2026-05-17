#include "blockchain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BACKUP_FILE_BIN ".blockchain.bin"
#define BACKUP_FILE_TXT ".blockchain.txt"
#define MAX_CMD_LENGTH 512

int main(void) {
    Blockchain *bc = NULL;
    
    /* ALWAYS start fresh - delete any existing backups so each test run starts with a clean state */
    unlink(BACKUP_FILE_BIN);
    unlink(BACKUP_FILE_TXT);
    
    /* Create new blockchain */
    bc = blockchain_create();
    if (!bc) {
        printf("Fatal: Could not create blockchain\n");
        return 1;
    }
    
    char command[MAX_CMD_LENGTH];
    int running = 1;
    
    while (running) {
        display_prompt(bc);
        
        if (!fgets(command, MAX_CMD_LENGTH, stdin)) {
            break;
        }
        
        int result = process_command(bc, command);
        if (result == 0) {
            running = 0;
        }
    }
    
    blockchain_save(bc, BACKUP_FILE_BIN);
    blockchain_save_text(bc, BACKUP_FILE_TXT);
    
    blockchain_destroy(bc);
    
    return 0;
}