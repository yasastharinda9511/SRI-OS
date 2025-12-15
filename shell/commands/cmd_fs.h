#ifndef CMD_FS_H
#define CMD_FS_H

// Command handlers
void cmd_ls(const char* args);
void cmd_cat(const char* args);
void cmd_touch(const char* args);
void cmd_write(const char* args);
void cmd_rm(const char* args);
void cmd_mkdir(const char* args) ;

// Register all system commands
void cmd_fs_init(void);

#endif