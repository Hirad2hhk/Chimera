extern "C"
{
#include "core/commandline.h"
#include "core/serial_manager.h"
#include "core/system_manager.h"
#include "managers/ap_manager.h"
#include "managers/display_manager.h"
#include "managers/rgb_manager.h"
#include "managers/sd_card_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ble_manager.h"
}
#include <esp_log.h>
#include <RadioLib.h>
#include "core/EspHal.h"

static const char *TAG = "main";
// create a new instance of the HAL class

// now we can create the radio module
// NSS pin:   18
// DIO0 pin:  26
// NRST pin:  14
// DIO1 pin:  33

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { return 0; }

extern "C" void app_main(void)
{
    initialize_console();
    wifi_manager_init();
    command_init();
    register_commands();
    xTaskCreate(console_task, "console_task", 4096, NULL, 5, NULL);

    #ifdef CONFIG_WITH_SCREEN

    #ifdef CONFIG_USE_JOYSTICK

    #define L_BTN 13
    #define C_BTN 34
    #define U_BTN 36
    #define R_BTN 39
    #define D_BTN 35

        joystick_init(&joysticks[0], L_BTN, HOLD_LIMIT, true);
        joystick_init(&joysticks[1], C_BTN, HOLD_LIMIT, true);
        joystick_init(&joysticks[2], U_BTN, HOLD_LIMIT, true);
        joystick_init(&joysticks[3], R_BTN, HOLD_LIMIT, true);
        joystick_init(&joysticks[4], D_BTN, HOLD_LIMIT, true);

        printf("Joystick GPIO Setup Successfully...\n");
    #endif

        display_manager_init();
        display_manager_switch_view(&splash_view);
    #endif

    #ifdef CONFIG_LED_DATA_PIN
        rgb_manager_init(&rgb_manager, CONFIG_LED_DATA_PIN, CONFIG_NUM_LEDS, LED_ORDER,
                         LED_MODEL_WS2812, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC);

        if (settings_get_rgb_mode(&G_Settings) == 1) {
            xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
        }
    #endif
    #ifdef CONFIG_RED_RGB_PIN &&CONFIG_GREEN_RGB_PIN &&CONFIG_BLUE_RGB_PIN
        rgb_manager_init(&rgb_manager, GPIO_NUM_NC, 1, LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812,
                         static_cast<gpio_num_t>(CONFIG_RED_RGB_PIN),
                         static_cast<gpio_num_t>(CONFIG_GREEN_RGB_PIN),
                         static_cast<gpio_num_t>(CONFIG_BLUE_RGB_PIN));
        if (settings_get_rgb_mode(&G_Settings) == 1) {
            xTaskCreate(rainbow_task, "Rainbow Task", 8192, &rgb_manager, 1, &rgb_effect_task_handle);
        }
    #endif

    printf("Ghost ESP Ready ;)\n");
   
}

// create a new instance of the HAL class
