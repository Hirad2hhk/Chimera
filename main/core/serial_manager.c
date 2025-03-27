#include "core/serial_manager.h"
#include "core/system_manager.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "managers/gps_manager.h"
#include <core/commandline.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) ||                                      \
    defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define JTAG_SUPPORTED 1
#else
#define JTAG_SUPPORTED 0
#endif

#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define SERIAL_BUFFER_SIZE 528

char serial_buffer[SERIAL_BUFFER_SIZE];

// Forward declaration of command handler
int handle_serial_command(const char *command);

void console_task(void *pvParameter)
{
  char *line;
  while (1)
  {
    line = linenoise("esp> ");

    if (line == NULL)
    {
      printf("Error: linenoise returned NULL.\n");
      continue; // Prevents crash
    }

      linenoiseHistoryAdd(line);


      printf("console_task ,Received command: %s", line);
      char output_buf[256] = {0}; // Create a small buffer for output
      esp_err_t ret = esp_console_run(line, output_buf);
      ESP_LOGI("console_task", "Received command: %s", line);
      if (ret != ESP_OK)
      {
        printf("Error executing command: %s\n", esp_err_to_name(ret));
      }

    linenoiseFree(line); // Safe to free since we checked for NULL
  }
}

// Initialize the SerialManager
void initialize_console()
{
  // Configure UART for console input/output
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  // Set up VFS to use UART0 for console
  esp_vfs_dev_uart_use_driver(UART_NUM_0);

  // Initialize console
  esp_console_config_t console_config = ESP_CONSOLE_CONFIG_DEFAULT();
  esp_console_init(&console_config);

  // Enable command history and auto-completion
  linenoiseSetMultiLine(1);
  linenoiseHistorySetMaxLen(10);
}

void simulateCommand(const char *commandString) {
  SerialCommand command;
  strncpy(command.command, commandString, sizeof(command.command) - 1);
  command.command[sizeof(command.command) - 1] = '\0';
  xQueueSend(commandQueue, &command, portMAX_DELAY);
}