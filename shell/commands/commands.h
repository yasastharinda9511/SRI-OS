#ifndef COMMANDS_H
#define COMMANDS_H


#define MAX_COMMANDS 32

// Command handler function type
typedef void (*CommandHandler)(const char* args);

// Command structure
typedef struct {
    const char* name;  // Command name (e.g., "help")
    const char* prefix;         
    const char* description;    // Short description
    CommandHandler handler;     // Function to call
} Command;

extern Command command_table[MAX_COMMANDS];
extern int command_count;

// Register a new command
void register_command(const char* name, const char* prefix, 
                      const char* description, CommandHandler handler);

// Process user input and execute matching command
void process_command(const char* input);

#endif