#include <stdio.h>
#include "pico/stdlib.h"

// --------------------------------------------------
// Utility: Clear leftover input from buffer
// --------------------------------------------------
void clear_input_buffer() {
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);
}

// --------------------------------------------------
// Utility: Read menu option (1–3)
// --------------------------------------------------
int read_menu_option() {
    int input_char;

    while (true) {
        input_char = getchar();

        // Skip newline characters
        if (input_char == '\n' || input_char == '\r')
            continue;

        if (input_char >= '1' && input_char <= '3') {
            printf("%c\n", input_char);  // echo input
            return input_char - '0';
        }

        printf("\nInvalid choice! Please enter 1, 2, or 3:\n");
    }
}

// --------------------------------------------------
// Utility: Read hexadecimal input from user
// --------------------------------------------------
uint32_t read_hex_value() {
    char input_buffer[20];
    int idx = 0;
    int ch;

    while (true) {
        ch = getchar();

        if (ch == '\n' || ch == '\r') {
            input_buffer[idx] = '\0';
            break;
        }

        if (idx < sizeof(input_buffer) - 1) {
            input_buffer[idx++] = ch;
            putchar(ch);  // echo
        }
    }

    printf("\n");

    uint32_t result = 0;
    sscanf(input_buffer, "%x", &result);

    clear_input_buffer();
    return result;
}

// --------------------------------------------------
// Function: Read from a register
// --------------------------------------------------
void perform_register_read() {
    printf("\nEnter register address (hex): ");

    uint32_t addr_val = read_hex_value();
    volatile uint32_t *reg_ptr = (volatile uint32_t *)addr_val;

    uint32_t reg_value = *reg_ptr;

    printf("\nValue at [0x%08X] = 0x%08X\n", addr_val, reg_value);
}

// --------------------------------------------------
// Function: Write to a register
// --------------------------------------------------
void perform_register_write() {
    printf("\nEnter register address (hex): ");
    uint32_t addr_val = read_hex_value();

    printf("\nEnter value to write (hex): ");
    uint32_t new_value = read_hex_value();

    volatile uint32_t *reg_ptr = (volatile uint32_t *)addr_val;

    // Optional: store old value (for debugging/extension)
    uint32_t old_value = *reg_ptr;

    *reg_ptr = new_value;

    sleep_ms(100);

    printf("\nUpdated [0x%08X]: 0x%08X -> 0x%08X\n",
           addr_val, old_value, new_value);
}

// --------------------------------------------------
// Main Program
// --------------------------------------------------
int main() {
    stdio_init_all();

    // Wait until serial connection is ready
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("\n=== Microcontroller Register Access ===\n");

    while (true) {
        printf("\nMenu:\n");
        printf("1. Read Register\n");
        printf("2. Write Register\n");
        printf("3. Exit\n");
        printf("Select option: ");

        int choice = read_menu_option();
        clear_input_buffer();

        switch (choice) {
            case 1:
                perform_register_read();
                break;

            case 2:
                perform_register_write();
                break;

            case 3:
                printf("\nExiting...\n");
                fflush(stdout);
                sleep_ms(300);
                return 0;

            default:
                printf("Invalid option!\n");
        }
    }

    return 0;
}