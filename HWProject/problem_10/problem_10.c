#include "pico/stdlib.h"
 
// Pin definitions
#define RS 0
#define EN 1
#define D4 2
#define D5 3
#define D6 4
#define D7 5
 
void lcd_send_nibble(uint8_t nibble) {
    gpio_put(D4, (nibble >> 0) & 1);
    gpio_put(D5, (nibble >> 1) & 1);
    gpio_put(D6, (nibble >> 2) & 1);
    gpio_put(D7, (nibble >> 3) & 1);
 
    gpio_put(EN, 1);
    sleep_us(1);
    gpio_put(EN, 0);
    sleep_us(100);
}
 
void lcd_send_byte(uint8_t data, int rs) {
    gpio_put(RS, rs);
    lcd_send_nibble(data >> 4);
    lcd_send_nibble(data & 0x0F);
}
 
void lcd_cmd(uint8_t cmd) {
    lcd_send_byte(cmd, 0);
    sleep_ms(2);
}
 
void lcd_char(char c) {
    lcd_send_byte(c, 1);
}
 
void lcd_string(const char *str) {
    while (*str) {
        lcd_char(*str++);
    }
}
 
void lcd_init() {
    gpio_init(RS); gpio_set_dir(RS, GPIO_OUT);
    gpio_init(EN); gpio_set_dir(EN, GPIO_OUT);
    gpio_init(D4); gpio_set_dir(D4, GPIO_OUT);
    gpio_init(D5); gpio_set_dir(D5, GPIO_OUT);
    gpio_init(D6); gpio_set_dir(D6, GPIO_OUT);
    gpio_init(D7); gpio_set_dir(D7, GPIO_OUT);
 
    sleep_ms(50);
 
    lcd_send_nibble(0x03);
    sleep_ms(5);
    lcd_send_nibble(0x03);
    sleep_ms(5);
    lcd_send_nibble(0x03);
    sleep_ms(5);
    lcd_send_nibble(0x02);
 
    lcd_cmd(0x28); // 4-bit, 2 line
    lcd_cmd(0x0C); // display ON
    lcd_cmd(0x06); // cursor increment
    lcd_cmd(0x01); // clear
}
 
int main() {
    stdio_init_all();
    lcd_init();
 
    // Line 1 (row 0, column 4)
    lcd_cmd(0x80 + 5);
    lcd_string("Sneha");
 
    // Line 2 (row 1, column 6)
    lcd_cmd(0xC0 + 3);
    lcd_string("Lahiri");
 
    while (1);
}

