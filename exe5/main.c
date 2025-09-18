/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

const int led_r_val = 1;
const int led_y_val = 2;

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if (gpio == BTN_PIN_R) {
            xQueueSendFromISR(xQueueBtn, &led_r_val, 0);
        } else if (gpio == BTN_PIN_Y) {
            xQueueSendFromISR(xQueueBtn, &led_y_val, 0);
        }
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 100;
    int signal = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(500)) == pdTRUE) {
            signal = !signal;
        }
        if (signal) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int delay = 100;
    int signal = 0;

    while (true) {
        if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(500)) == pdTRUE) {
            signal = !signal;
        }
        if (signal) {
            gpio_put(LED_PIN_Y, 1);
            vTaskDelay(pdMS_TO_TICKS(delay));
            gpio_put(LED_PIN_Y, 0);
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}

void btn_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int led = 0;

    while (true) {
        if (xQueueReceive(xQueueBtn, &led, 0)) {
            printf("%d\n", led);
            if (led > 0) {
                if (led == 1) {
                    xSemaphoreGive(xSemaphoreLedR);
                } else {
                    xSemaphoreGive(xSemaphoreLedY);
                }
            }
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(int));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_r_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_y_task, "LED_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }

    return 0;
}