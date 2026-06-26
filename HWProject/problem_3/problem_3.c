#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
 
#define ADC_PIN 26
#define LED_PIN 15
 
int main() {
    stdio_init_all();
    sleep_ms(2000);
 
    // -------- ADC SETUP --------
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0); // GP26 = ADC0
 
    // -------- PWM SETUP --------
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN);
 
    pwm_set_wrap(slice_num, 65535); // 16-bit resolution
    pwm_set_enabled(slice_num, true);
 
    printf("ADC to PWM LED Control Started\n");
 
    while (1) {
 
        uint16_t adc_value = adc_read(); // 0–4095
 
        // Scale ADC (12-bit) → PWM (16-bit)
        uint16_t pwm_value = adc_value * 16;
 
        pwm_set_gpio_level(LED_PIN, pwm_value);
 
        printf("ADC: %d  PWM: %d\n", adc_value, pwm_value);
 
        sleep_ms(100);
    }
}
 