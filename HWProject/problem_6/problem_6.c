#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Flash Commands for W25Q64
#define FLASH_CMD_WRITE_ENABLE 0x06
#define FLASH_CMD_WRITE_DISABLE 0x04
#define FLASH_CMD_READ_STATUS 0x05
#define FLASH_CMD_READ_DATA 0x03
#define FLASH_CMD_PAGE_PROGRAM 0x02
#define FLASH_CMD_SECTOR_ERASE 0x20
#define FLASH_CMD_READ_ID 0x9F

// SPI Pin Configuration for Pico
#define SPI_PORT spi0
#define FLASH_CS_PIN 17 // GPIO Pin 17 for Chip Select (CS)

// Flash Memory Size Constants
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096
#define FLASH_ADDR 0x000000 // Address for testing

// Function Prototypes
void flash_write_enable();
void flash_write_disable();
void flash_wait_until_ready();
void flash_sector_erase(uint32_t address);
void flash_page_program(uint32_t address, uint8_t *data, uint16_t length);
void flash_read_data(uint32_t address, uint8_t *data, uint16_t length);
void print_buffer(uint8_t *buffer, uint16_t length);

int main() {
    // Initialize UART for communication with the serial console
    stdio_init_all();
    sleep_ms(30000);

    // Initialize SPI0 at 1 MHz
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(16, GPIO_FUNC_SPI); // MISO (DO)
    gpio_set_function(17, GPIO_FUNC_SPI); // CS
    gpio_set_function(18, GPIO_FUNC_SPI); // SCK (CLK)
    gpio_set_function(19, GPIO_FUNC_SPI); // MOSI (DI)

    // Set Chip Select (CS) pin as output and high initially
    gpio_init(FLASH_CS_PIN);
    gpio_put(FLASH_CS_PIN, 1);
    gpio_set_dir(FLASH_CS_PIN, GPIO_OUT);

    // Example address and data to write and read
    uint32_t address = FLASH_ADDR;
    uint8_t write_data[FLASH_PAGE_SIZE] = {0};
    uint8_t read_data[FLASH_PAGE_SIZE] = {0};

    // Fill write_data with example values
    for (int i = 0; i < FLASH_PAGE_SIZE; i++) {
        write_data[i] = i; // Example pattern 0, 1, 2, ...
    }

    // Erase the flash sector
    printf("Erasing flash sector...\n");
    flash_sector_erase(address);
    printf("Sector erased.\n");

    // Write data to flash
    printf("Writing data to flash...\n");
    flash_page_program(address, write_data, FLASH_PAGE_SIZE);
    printf("Data written to flash.\n");

    // Read data back from flash
    printf("Reading data from flash...\n");
    flash_read_data(address, read_data, FLASH_PAGE_SIZE);

    // Print the read data
    printf("Read Data:\n");
    print_buffer(read_data, FLASH_PAGE_SIZE);

    // Wait for a while before exiting
    sleep_ms(5000);
    return 0;
}

void flash_write_enable() {
    uint8_t cmd = FLASH_CMD_WRITE_ENABLE;
    gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
}

void flash_write_disable() {
    uint8_t cmd = FLASH_CMD_WRITE_DISABLE;
    gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
}

void flash_wait_until_ready() {
    uint8_t status;
    do {
        uint8_t cmd = FLASH_CMD_READ_STATUS;
        gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
        spi_write_blocking(SPI_PORT, &cmd, 1);
        spi_read_blocking(SPI_PORT, 0, &status, 1);
        gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
    } while (status & 0x01); // Check if BUSY bit is set
}

void flash_sector_erase(uint32_t address) {
    flash_write_enable(); // Enable write operations
    uint8_t cmd[4] = {
        FLASH_CMD_SECTOR_ERASE,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };
    gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
    spi_write_blocking(SPI_PORT, cmd, 4);
    gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
    flash_wait_until_ready(); // Wait until the operation is complete
}

void flash_page_program(uint32_t address, uint8_t *data, uint16_t length) {
    flash_write_enable(); // Enable write operations
    uint8_t cmd[4] = {
        FLASH_CMD_PAGE_PROGRAM,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };
    gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
    spi_write_blocking(SPI_PORT, cmd, 4); // Send the command and address
    spi_write_blocking(SPI_PORT, data, length); // Send the data to be written
    gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
    flash_wait_until_ready(); // Wait until the operation is complete
}

void flash_read_data(uint32_t address, uint8_t *data, uint16_t length) {
    uint8_t cmd[4] = {
        FLASH_CMD_READ_DATA,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };
    gpio_put(FLASH_CS_PIN, 0); // CS low to start communication
    spi_write_blocking(SPI_PORT, cmd, 4); // Send the command and address
    spi_read_blocking(SPI_PORT, 0, data, length); // Read the data into the buffer
    gpio_put(FLASH_CS_PIN, 1); // CS high to end communication
}

void print_buffer(uint8_t *buffer, uint16_t length) {
    for (int i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("\n");
        }
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}