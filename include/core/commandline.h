// command.h

#ifndef COMMAND_H
#define COMMAND_H

#include "driver/gpio.h"
#include"freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef void (*CommandFunction)(int argc, char **argv);

typedef struct CommandNode{
  char *name;
  CommandFunction function;
  struct CommandNode *next;
} CommandNode;

// Functions to manage commands
void command_init();
CommandFunction find_command(const char *name);

extern TaskHandle_t VisualizerHandle;

void register_commands();

#endif // COMMAND_H