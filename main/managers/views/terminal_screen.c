#include "managers/views/terminal_screen.h"
#include "core/serial_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "managers/views/main_menu_screen.h"
#include "managers/wifi_manager.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "Terminal";
static lv_obj_t *terminal_textarea = NULL;
static SemaphoreHandle_t terminal_mutex = NULL;
static bool terminal_active = false;
static bool is_stopping = false;
#define MAX_TEXT_LENGTH 4096
#define CLEANUP_THRESHOLD (MAX_TEXT_LENGTH * 3 / 4)
#define CLEANUP_AMOUNT (MAX_TEXT_LENGTH / 2)
#define MAX_QUEUE_SIZE 10
#define MAX_MESSAGE_SIZE 256

// Message queue system
typedef struct {
  char messages[MAX_QUEUE_SIZE][MAX_MESSAGE_SIZE];
  int head;
  int tail;
  int count;
} MessageQueue;

static MessageQueue message_queue = {.head = 0, .tail = 0, .count = 0};

static void queue_message(const char *text) {
  if (message_queue.count >= MAX_QUEUE_SIZE) {
    // Remove oldest message by advancing head
    message_queue.head = (message_queue.head + 1) % MAX_QUEUE_SIZE;
    message_queue.count--;
  }

  strncpy(message_queue.messages[message_queue.tail], text,
          MAX_MESSAGE_SIZE - 1);
  message_queue.messages[message_queue.tail][MAX_MESSAGE_SIZE - 1] = '\0';

  message_queue.tail = (message_queue.tail + 1) % MAX_QUEUE_SIZE;
  message_queue.count++;
}

static void clear_message_queue(void) {
  message_queue.head = 0;
  message_queue.tail = 0;
  message_queue.count = 0;
}

static void process_queued_messages(void) {
  while (message_queue.count > 0) {
    const char *msg = message_queue.messages[message_queue.head];
    terminal_view_add_text(msg);

    message_queue.head = (message_queue.head + 1) % MAX_QUEUE_SIZE;
    message_queue.count--;
  }
}

int custom_log_vprintf(const char *fmt, va_list args);
static int (*default_log_vprintf)(const char *, va_list) = NULL;

static void scroll_terminal_up(void) {
  if (!terminal_textarea) return;

  lv_coord_t font_height = lv_font_get_line_height(
      lv_obj_get_style_text_font(terminal_textarea, LV_PART_MAIN));
  lv_coord_t scroll_pixels = lv_obj_get_height(terminal_textarea) / 2;
  lv_obj_scroll_by(terminal_textarea, 0, scroll_pixels, LV_ANIM_OFF);
  lv_obj_invalidate(terminal_textarea);
}

static void scroll_terminal_down(void) {
  if (!terminal_textarea) return;

  lv_coord_t font_height = lv_font_get_line_height(
      lv_obj_get_style_text_font(terminal_textarea, LV_PART_MAIN));
  lv_coord_t scroll_pixels = lv_obj_get_height(terminal_textarea) / 2;
  lv_obj_scroll_by(terminal_textarea, 0, -scroll_pixels, LV_ANIM_OFF);
  lv_obj_invalidate(terminal_textarea);
}

static void stop_all_operations(void) {
  terminal_active = false;
  is_stopping = true;
  clear_message_queue();
  simulateCommand("stop");             // Stops WiFi operations
  simulateCommand("stopspam");         // Stops beacon spam
  simulateCommand("stopdeauth");       // Stops deauth attacks
  simulateCommand("capture -stop");    // Stops packet capture
  simulateCommand("stopportal");       // Stops evil portal
  simulateCommand("gpsinfo -s");       // Stops GPS operations
  simulateCommand("blewardriving -s"); // Stops BLE operations
  simulateCommand("pineap -s");        // Stops PineAP detection

  display_manager_switch_view(&options_menu_view);
}

void terminal_view_create(void) {
  is_stopping = false;
  if (terminal_view.root != NULL) {
    return;
  }

  // Create mutex if it doesn't exist
  if (!terminal_mutex) {
    terminal_mutex = xSemaphoreCreateMutex();
    if (!terminal_mutex) {
      ESP_LOGE(TAG, "Failed to create terminal mutex");
      return;
    }
  }

  terminal_active = true;

  terminal_view.root = lv_obj_create(lv_scr_act());
  lv_obj_set_size(terminal_view.root, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_bg_color(terminal_view.root, lv_color_black(), 0);
  lv_obj_set_scrollbar_mode(terminal_view.root, LV_SCROLLBAR_MODE_OFF);

  terminal_textarea = lv_textarea_create(terminal_view.root);

  // Remove ALL default styles first
  lv_obj_remove_style(terminal_textarea, NULL, LV_PART_MAIN);
  lv_obj_remove_style(terminal_textarea, NULL, LV_PART_SCROLLBAR);

  // Text styling
  lv_obj_set_style_bg_opa(terminal_textarea, LV_OPA_TRANSP, 0);
  lv_textarea_set_one_line(terminal_textarea, false);
  lv_textarea_set_text(terminal_textarea, "");
  lv_obj_set_size(terminal_textarea, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_text_color(terminal_textarea, lv_color_hex(0x00FF00), 0);
  
  // Fixed font size (original working version)
  lv_obj_set_style_text_font(terminal_textarea, &lv_font_montserrat_10, 0);

  // Explicitly remove root container border
  lv_obj_set_style_border_width(terminal_view.root, 0, 0);
  lv_obj_set_style_radius(terminal_view.root, 0, 0);

  // scrolling
  lv_obj_set_scrollbar_mode(terminal_textarea, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_anim_time(terminal_textarea, 0, 0);
  lv_obj_set_style_clip_corner(terminal_textarea, false, 0);
  lv_obj_add_flag(terminal_textarea, LV_OBJ_FLAG_SCROLL_ONE);
  lv_obj_clear_flag(terminal_textarea, 
                  LV_OBJ_FLAG_SCROLL_CHAIN | 
                  LV_OBJ_FLAG_SCROLL_ELASTIC |
                  LV_OBJ_FLAG_SCROLL_MOMENTUM);

  lv_textarea_set_text_selection(terminal_textarea, false);
  lv_textarea_set_password_mode(terminal_textarea, false);
  lv_textarea_set_cursor_click_pos(terminal_textarea,
                                   false); // Disable cursor positioning

  display_manager_add_status_bar("Terminal");

  // Process any queued messages after terminal is ready
  process_queued_messages();
}

void terminal_view_destroy(void) {
  terminal_active = false;
  is_stopping = true;
  clear_message_queue(); // Clear any pending messages when destroying view

  // Wait a bit for pending operations
  vTaskDelay(pdMS_TO_TICKS(50));

  if (terminal_mutex) {
    if (xSemaphoreTake(terminal_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      vSemaphoreDelete(terminal_mutex);
      terminal_mutex = NULL;
    }
  }

  if (terminal_view.root != NULL) {
    lv_obj_del(terminal_view.root);
    terminal_view.root = NULL;
    terminal_textarea = NULL;
  }

  is_stopping = false;
}

void terminal_view_add_text(const char *text) {
  if (!text || is_stopping) return;
  if (text[0] == '\0') return;

  if (!terminal_active || !terminal_textarea) {
    queue_message(text);
    return;
  }

  if (xSemaphoreTake(terminal_mutex, pdMS_TO_TICKS(500)) != pdTRUE) {
    ESP_LOGW(TAG, "Failed to acquire terminal mutex");
    queue_message(text);
    return;
  }

  // Use LVGL's API instead of direct buffer manipulation
  const char *current_text = lv_textarea_get_text(terminal_textarea);
  size_t current_len = strlen(current_text);
  size_t new_len = strlen(text);

  if (current_len + new_len > CLEANUP_THRESHOLD) {
    // Find first newline after cleanup threshold
    const char *start = current_text + CLEANUP_AMOUNT;
    while (*start && *start != '\n') start++;
    if (*start == '\n') start++;

    // Create new buffer using LVGL-safe operations
    size_t keep_len = strlen(start);
    char *safe_buffer = malloc(keep_len + new_len + 1);
    if (safe_buffer) {
      memcpy(safe_buffer, start, keep_len);
      memcpy(safe_buffer + keep_len, text, new_len);
      safe_buffer[keep_len + new_len] = '\0';
      lv_textarea_set_text(terminal_textarea, safe_buffer);
      free(safe_buffer);
    }
  } else {
    // Use official API for appending text
    lv_textarea_add_text(terminal_textarea, text);
  }

  // Simplified cursor management
  lv_textarea_set_cursor_pos(terminal_textarea, LV_TEXTAREA_CURSOR_LAST);
  xSemaphoreGive(terminal_mutex);
}

void terminal_view_hardwareinput_callback(InputEvent *event) {
  if (event->type == INPUT_TYPE_TOUCH) {
    int touch_y = event->data.touch_data.point.y;

    if (touch_y < LV_VER_RES / 3) {
      scroll_terminal_up();
    } else if (touch_y > (LV_VER_RES * 2) / 3) {
      scroll_terminal_down();
    } else {
      stop_all_operations();
      return;
    }
  } else if (event->type == INPUT_TYPE_JOYSTICK) {
    int button = event->data.joystick_index;
    if (button == 1) {
      stop_all_operations();
      return;
    }

    if (button == 2) {
      scroll_terminal_up();
    }

    if (button == 4) {
      scroll_terminal_down();
    }
  }
}

void terminal_view_get_hardwareinput_callback(void **callback) {
  if (callback != NULL) {
    *callback = (void *)terminal_view_hardwareinput_callback;
  }
}

int custom_log_vprintf(const char *fmt, va_list args) {
  char buf[256];
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  if (len < 0) {
    return len;
  }

  terminal_view_add_text(buf);

  return len;
}

View terminal_view = {.root = NULL,
                      .create = terminal_view_create,
                      .destroy = terminal_view_destroy,
                      .input_callback = terminal_view_hardwareinput_callback,
                      .name = "TerminalView",
                      .get_hardwareinput_callback =
                          terminal_view_get_hardwareinput_callback};