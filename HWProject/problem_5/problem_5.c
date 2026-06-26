#include <stdio.h>
#include "pico/stdlib.h"
 
#define TRIG_PIN 3
#define ECHO_PIN 2
 
float get_distance() {
    // Trigger pulse
    gpio_put(TRIG_PIN, 0);
    sleep_us(2);
 
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
 
    // Wait for echo start
    while (gpio_get(ECHO_PIN) == 0);
    absolute_time_t start = get_absolute_time();
 
    // Wait for echo end
    while (gpio_get(ECHO_PIN) == 1);
    absolute_time_t end = get_absolute_time();
 
    int64_t duration = absolute_time_diff_us(start, end);
 
    // Distance in cm
    float distance = (duration * 0.0343) / 2;
 
    return distance;
}
 
int main() {
    stdio_init_all();
 
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
 
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
 
    while (1) {
        float dist = get_distance();
        printf("Distance: %.2f cm\n", dist);
        sleep_ms(500);
    }
}