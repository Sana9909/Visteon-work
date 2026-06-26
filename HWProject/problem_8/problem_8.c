#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 15    // Define GPIO pin for LED
#define BUTTON_PIN 14 // Define GPIO pin for Button

volatile bool interrupt_flag = false; // Interrupt flag

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) {
        interrupt_flag = true;
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (1) {
        if (interrupt_flag) {
            interrupt_flag = false;
            for (int i = 0; i < 5; i++) {
                gpio_put(LED_PIN, 1);
                sleep_ms(500);
                gpio_put(LED_PIN, 0);
                sleep_ms(500);
            }
        }
    }
}