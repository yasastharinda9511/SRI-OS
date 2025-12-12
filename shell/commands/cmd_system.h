#ifndef CMD_SYSTEM_H
#define CMD_SYSTEM_H

// Command handlers
void cmd_info(const char* args);
void cmd_help(const char* args);
void cmd_uptime(const char* args);
void cmd_clear(const char* args);
void cmd_reboot(const char* args);

// Register all system commands
void cmd_system_init(void);

#endif
