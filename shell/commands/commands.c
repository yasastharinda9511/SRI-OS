#include "commands.h"
#include "../../drivers/uart/uart.h"
#include "../../utils/string_utils.h"

// Initialize the command table
Command command_table[MAX_COMMANDS];
int command_count = 0;

// Register a command
void register_command(const char* name, const char* prefix, 
                      const char* description, CommandHandler handler) {
    if (command_count >= MAX_COMMANDS) {
        uart_puts("Error: Command table full!\n");
        return;
    }
    
    command_table[command_count].name = name;
    command_table[command_count].prefix = prefix;
    command_table[command_count].description = description;
    command_table[command_count].handler = handler;
    command_count++;
}

// Process input and execute command
void process_command(const char* input) {
    // Skip empty input
    input = str_skip_spaces(input);
    if (input[0] == '\0') {
        return;
    }
    
    // Try to match a command
    for (int i = 0; i < command_count; i++) {
        const char* prefix = command_table[i].prefix;
        int prefix_len = str_len(prefix);
        
        // Check if input starts with this command's prefix
        if (str_startswith(input, prefix)) {
            char next_char = input[prefix_len];
            
            // Match if end of input or space after prefix
            if (next_char == '\0' || next_char == ' ') {
                // Get arguments (skip prefix and space)
                const char* args = input + prefix_len;
                args = str_skip_spaces(args);
                
                // Call handler
                command_table[i].handler(args);
                return;
            }
        }
    }
    
    // No match found
    uart_puts("Unknown command: ");
    uart_puts(input);
    uart_puts("\nType 'help' for available commands.\n");
}