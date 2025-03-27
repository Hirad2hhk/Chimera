file(GLOB_RECURSE app_sources "${CMAKE_SOURCE_DIR}/main/*.c")
file(GLOB_RECURSE axs15231b_sources "${CMAKE_SOURCE_DIR}/components/axs15231b/*.c")


list(APPEND app_sources ${axs15231b_sources})


set(required_components bt nvs_flash driver esp_http_server mdns json esp_http_client mbedtls fatfs sdmmc wpa_supplicant lvgl lvgl_esp32_drivers freertos esp_lcd)


set(sources ${app_sources})


idf_component_register(SRCS ${sources}
                       INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include"
                       REQUIRES ${required_components})
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-incompatible-pointer-types)
