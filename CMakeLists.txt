cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ZPHR-STM32-0003)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/inc)

target_sources(app  PRIVATE
               src/main.c
               src/setup_database.c
               src/slave_modbus.c
               src/app_ext_flash.c
               src/app_icon.c
               src/app_sdram.c
               src/app_version.c
               src/buzzer_lib.c
               src/eeprom_lib.c
               src/isotp_conn.c
               src/lcd_lib.c
               src/leds_lib.c
               src/rtc_lib.c
               src/eth_lib.c
               src/sdcard_lib.c
)

add_subdirectory(common/utils)
add_subdirectory(common/string_format)
add_subdirectory(common/mask_format)
add_subdirectory(libraries/alarm)
add_subdirectory(libraries/database)
add_subdirectory(libraries/leds)
add_subdirectory(libraries/ringtone)
add_subdirectory(libraries/modbus_slave)
add_subdirectory(libraries/digital_input)
add_subdirectory(libraries/digital_output)

if(CONFIG_CODE_DATA_RELOCATION)
zephyr_code_relocate(FILES src/app_ext_flash.c LOCATION EXTMEM NOCOPY)
endif()

zephyr_linker_sources(SECTIONS linker/dc_ram.ld)

# ========================
# LVGL Configuration - Simplified for v4.1.0
# ========================

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_BENCHMARK app PRIVATE
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_avatar.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_argb.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_rgb.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_12_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_16_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_28_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_STRESS app PRIVATE
    ${LVGL_DIR}/demos/stress/lv_demo_stress.c
)

# Widgets demo - only if explicitly enabled and assets exist
target_sources_ifdef(CONFIG_LV_USE_DEMO_WIDGETS app PRIVATE
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_needle.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c
)


target_sources_ifdef(CONFIG_LV_USE_DEMO_MUSIC app PRIVATE
    ${LVGL_DIR}/demos/music/lv_demo_music_list.c
    ${LVGL_DIR}/demos/music/lv_demo_music.c
    ${LVGL_DIR}/demos/music/lv_demo_music_main.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_next.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_top_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_4_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_4.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_right.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_1.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_slider_knob_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_3.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_pause.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_pause_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_bottom_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_2_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_play_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_top.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_play_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_1_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_bottom.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_left_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_play.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_list_border.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_2.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_next_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_play.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_list_border_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_rnd.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_3.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_prev_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_loop.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_1.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_slider_knob.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_right_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_corner_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_left.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_2_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_3_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_1_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_2.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_prev.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_rnd_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_3_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_pause.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_logo.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_loop_large.c
)