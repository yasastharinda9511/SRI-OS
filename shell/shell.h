#ifndef SHELL_H
#define SHELL_H

// Initialize shell and register all commands
void shell_init(void);

// Run shell loop
void shell_run(void);

// Shell as a task (for scheduler)
void shell_task(void);

#endif
