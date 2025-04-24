/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

// This demo UI is adapted from LVGL official example: https://docs.lvgl.io/master/examples.html#loader-with-arc

#include "lvgl.h"
#include "esp_log.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Access global variable from main.c
extern _lock_t lvgl_api_lock;
extern float battery_percentage;
extern int battery_is_charging;
extern int e_stop;
extern int handbrake;
extern int direct_status;      // 0 or 1 for autonomous
extern int safety_mode;
extern int robot_mode;      // 1-Idle, 2-Coverage, 3-Litter Picking, 4-Switching


// styles
static lv_style_t style_normal;
static lv_style_t style_unknown;
static lv_style_t style_warning;
static lv_style_t style_blue;
static bool styles_initialized = false;     //battery


// objects
static lv_obj_t * btn_e_stop = NULL;
static lv_obj_t * lbl_e_stop = NULL;
static lv_obj_t * btn_handbrake = NULL;
static lv_obj_t * lbl_handbrake = NULL;
static lv_obj_t * btn_autonomous = NULL;
static lv_obj_t * lbl_autonomous = NULL;
static lv_obj_t * btn_safety_mode = NULL;
static lv_obj_t * lbl_safety_mode = NULL;
static lv_obj_t * btn_robot_mode = NULL;
static lv_obj_t * lbl_robot_mode = NULL;
static lv_obj_t * btn_battery = NULL;
static lv_obj_t * lbl_battery_percentage = NULL;


// Blink stuff
static TaskHandle_t battery_flash_task_handle = NULL;
static TaskHandle_t litter_picking_flash_task_handle = NULL;



void example_lvgl_demo_ui(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);

    //styles
    if (!styles_initialized) {
        styles_initialized = true;
    
        // Dk leh (Grey Green)
        lv_style_init(&style_unknown);
        lv_style_set_bg_color(&style_unknown, lv_color_make(75,80,70));  //BGR
        lv_style_set_pad_all(&style_unknown, 5);       // Minimal padding
    
        // Ooi (Red)
        lv_style_init(&style_warning);
        lv_style_set_bg_color(&style_warning, lv_color_make(0,0,255));      //BGR
        lv_style_set_pad_all(&style_warning, 5);       // Minimal padding
    
        // Okay lor (Green)
        lv_style_init(&style_normal);
        lv_style_set_bg_color(&style_normal, lv_color_make(100, 180, 30));  //BGR
        lv_style_set_pad_all(&style_normal, 5);       // Minimal padding

        // Manual Control (Blue)
        lv_style_init(&style_blue);
        lv_style_set_bg_color(&style_blue, lv_color_make(200, 170, 60));  //BGR
        lv_style_set_pad_all(&style_blue, 5);       // Minimal padding
    }

    // Define button text labels
    const char *btn_labels[] = {
        "E-Stop",
        "Safety Mode",
        "Handbrake",
        "Autonomous Control",
        "Robot Mode",
        "Battery %"
    };

    // Define pointers to your global objects (so you still have access in update functions)
    lv_obj_t **btn_ptrs[] = {
        &btn_e_stop,
        &btn_safety_mode,
        &btn_handbrake,
        &btn_autonomous,
        &btn_robot_mode,
        &btn_battery
    };

    lv_obj_t **lbl_ptrs[] = {
        &lbl_e_stop,
        &lbl_safety_mode,
        &lbl_handbrake,
        &lbl_autonomous,
        &lbl_robot_mode,
        &lbl_battery_percentage
    };

    // Create buttons in a loop
    int base_y = 5;      // Starting Y offset
    int spacing = 25;    // Vertical spacing

    for (int i = 0; i < 6; i++) {
        *(btn_ptrs[i]) = lv_button_create(scr);
        *(lbl_ptrs[i]) = lv_label_create(*(btn_ptrs[i]));
        lv_label_set_text_static(*(lbl_ptrs[i]), btn_labels[i]);
        lv_obj_align(*(btn_ptrs[i]), LV_ALIGN_TOP_MID, 0, base_y + i * spacing);
    }

}


// Flashing buttons
void flash_battery_style_task(void *arg)
{
    lv_obj_t *target = (lv_obj_t *)arg;

    while (1) {
        // Step 1: Apply style_unknown
        _lock_acquire(&lvgl_api_lock);
        lv_obj_remove_style(target, &style_normal, 0);
        lv_obj_remove_style(target, &style_unknown, 0);
        lv_obj_add_style(target, &style_unknown, 0);
        _lock_release(&lvgl_api_lock);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Step 2: Apply style_normal
        _lock_acquire(&lvgl_api_lock);
        lv_obj_remove_style(target, &style_unknown, 0);
        lv_obj_remove_style(target, &style_normal, 0);
        lv_obj_add_style(target, &style_normal, 0);
        _lock_release(&lvgl_api_lock);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

//  change to litter picking blink
void flash_litter_picking_style_task(void *arg)
{
    lv_obj_t *target = (lv_obj_t *)arg;

    while (1) {
        // Step 1: Apply style_unknown
        _lock_acquire(&lvgl_api_lock);
        lv_obj_remove_style(target, &style_blue, 0);
        lv_obj_remove_style(target, &style_unknown, 0);
        lv_obj_add_style(target, &style_unknown, 0);
        _lock_release(&lvgl_api_lock);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Step 2: Apply style_normal
        _lock_acquire(&lvgl_api_lock);
        lv_obj_remove_style(target, &style_unknown, 0);
        lv_obj_remove_style(target, &style_blue, 0);
        lv_obj_add_style(target, &style_blue, 0);
        _lock_release(&lvgl_api_lock);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}



// logic
void lvgl_update_battery_percentage(float battery_percentage)
{
    char battery_str[32];
    snprintf(battery_str, sizeof(battery_str), "Battery: %.1f%%", battery_percentage);
    lv_label_set_text(lbl_battery_percentage, battery_str);
}

void lvgl_update_battery_charge(int battery_is_charging)
{
    if (battery_is_charging == 1) {
        if (battery_flash_task_handle == NULL) {
            xTaskCreate(flash_battery_style_task, "battery_flash", 2048, btn_battery, 3, &battery_flash_task_handle);
        }
    }else {
        lv_obj_add_style(btn_battery, &style_unknown, 0);

        if (battery_flash_task_handle != NULL){
            vTaskDelete(battery_flash_task_handle);
            battery_flash_task_handle = NULL;
    
            _lock_acquire(&lvgl_api_lock);
            lv_obj_remove_style(btn_battery, &style_normal, 0);
            lv_obj_remove_style(btn_battery, &style_unknown, 0);
            lv_obj_add_style(btn_battery, &style_unknown, 0);
            _lock_release(&lvgl_api_lock);}
    }
}


void lvgl_update_e_stop(int e_stop)
{
    if (e_stop == 1) {
        lv_obj_add_style(btn_e_stop, &style_warning, 0);    //On - Red
    } else {
        lv_obj_add_style(btn_e_stop, &style_unknown, 0);    //Off / Unknown - Greyed
    }
}

void lvgl_update_handbrake(int handbrake)
{
    if (handbrake == 1) {
        lv_obj_add_style(btn_handbrake, &style_warning, 0);
    } else {
        lv_obj_add_style(btn_handbrake, &style_unknown, 0);
    }
}

void lvgl_update_autonomous(int direct_status)
{
    if (direct_status == 1) {
        lv_label_set_text(lbl_autonomous, "Manual Control");
        lv_obj_add_style(btn_autonomous, &style_blue, 0);
    } else {
        lv_label_set_text(lbl_autonomous, "Autonomous Control");
        lv_obj_add_style(btn_autonomous, &style_unknown, 0);
    }
    lv_obj_center(lbl_autonomous); //realign to button
}

void lvgl_update_safety_mode(int safety_mode)
{
    if (safety_mode == 1) {
        lv_label_set_text(lbl_safety_mode, "Safety Mode: On");
        lv_obj_add_style(btn_safety_mode, &style_normal, 0);
    } else if (safety_mode == 0) {
        lv_label_set_text(lbl_safety_mode, "Safety Mode: Off");
        lv_obj_add_style(btn_safety_mode, &style_warning, 0);
    } else {
        lv_label_set_text(lbl_safety_mode, "Safety Mode: Unknown");
        lv_obj_add_style(btn_safety_mode, &style_unknown, 0);
    }
    lv_obj_center(lbl_safety_mode); //realign to button
}

void lvgl_update_robot_mode(int robot_mode)     // 1-Idle, 2-Coverage, 3-Litter Picking, 4-Switching
{
    if (robot_mode == 1) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Mode: Idle");
        lv_obj_add_style(btn_robot_mode, &style_normal, 0);
    } else if (robot_mode == 2) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Mode: Coverage");
        lv_obj_add_style(btn_robot_mode, &style_normal, 0);
    } else if (robot_mode == 3) {
        lv_label_set_text(lbl_robot_mode, "Mode: Litter Picking");
        if (litter_picking_flash_task_handle == NULL) {
            xTaskCreate(flash_litter_picking_style_task, "litter_picking_flash", 2048, btn_robot_mode, 3, &litter_picking_flash_task_handle);
        }
    } else if (robot_mode == 4) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Switching Mode");
        lv_obj_add_style(btn_robot_mode, &style_normal, 0);
    } else if (robot_mode == 6) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Mode: Idle");
        lv_obj_add_style(btn_robot_mode, &style_normal, 0);
    } else if (robot_mode == 7) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Error");
        lv_obj_add_style(btn_robot_mode, &style_warning, 0);
    } else if (robot_mode == 0) {
        if (litter_picking_flash_task_handle != NULL) {
            vTaskDelete(litter_picking_flash_task_handle);
            litter_picking_flash_task_handle = NULL;
        }
        lv_label_set_text(lbl_robot_mode, "Mode: ?");
        lv_obj_add_style(btn_robot_mode, &style_unknown, 0);
    } else {
        lv_label_set_text(lbl_robot_mode, "Mode: ?");
        lv_obj_add_style(btn_robot_mode, &style_unknown, 0);
    }
    lv_obj_center(lbl_robot_mode); //realign to button
}