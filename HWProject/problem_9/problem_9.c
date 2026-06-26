//TRANSMITTER CODE FOR CAN BUS USING MCP2515 AND RASPBERRY PI PICO

#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define MCP_RESET    0xC0
#define MCP_WRITE    0x02
#define MCP_RTS_TXB0 0x81

#define CANCTRL      0x0F
#define TXB0SIDH     0x31
#define TXB0SIDL     0x32
#define TXB0DLC      0x35
#define TXB0D0       0x36

void spi_write_byte(uint8_t address, uint8_t data) {
    gpio_put(PIN_CS, 0);

    uint8_t buf[3] = {MCP_WRITE, address, data};
    spi_write_blocking(SPI_PORT, buf, 3);

    gpio_put(PIN_CS, 1);
}

void mcp2515_reset() {
    gpio_put(PIN_CS, 0);

    uint8_t cmd = MCP_RESET;
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_CS, 1);
    sleep_ms(10);
}

void mcp2515_init() {
    mcp2515_reset();

    spi_write_byte(0x2A, 0x00);
    spi_write_byte(0x29, 0x90);
    spi_write_byte(0x28, 0x02);

    spi_write_byte(CANCTRL, 0x00);
}

void can_send_message() {
    spi_write_byte(TXB0SIDH, 0x01);
    spi_write_byte(TXB0SIDL, 0x00);
    spi_write_byte(TXB0DLC, 0x01);
    spi_write_byte(TXB0D0, 0xAA);

    gpio_put(PIN_CS, 0);

    uint8_t cmd = MCP_RTS_TXB0;
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_CS, 1);
}

int main() {
    stdio_init_all();

    spi_init(SPI_PORT, 500000);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    sleep_ms(2000);

    mcp2515_init();

    printf("CAN Sender Ready...\n");

    while (1) {
        can_send_message();
        printf("Message sent!\n");
        sleep_ms(1000);
    }
}

//RECEIVER CODE FOR CAN BUS USING MCP2515 AND RASPBERRY PI PICO

#include "pico/stdlib.h"
#include "stdio.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define MCP_RESET    0xC0
#define MCP_WRITE    0x02
#define MCP_READ     0x03

#define CANCTRL      0x0F
#define RXB0CTRL     0x60
#define RXB0D0       0x66
#define CANINTF      0x2C

// SPI write
void spi_write_byte(uint8_t address, uint8_t data) {
    gpio_put(PIN_CS, 0);

    uint8_t buf[3] = {MCP_WRITE, address, data};
    spi_write_blocking(SPI_PORT, buf, 3);

    gpio_put(PIN_CS, 1);
}

// SPI read
uint8_t spi_read_byte(uint8_t address) {
    gpio_put(PIN_CS, 0);

    uint8_t tx[3] = {MCP_READ, address, 0x00};
    uint8_t rx[3];

    spi_write_read_blocking(SPI_PORT, tx, rx, 3);

    gpio_put(PIN_CS, 1);

    return rx[2];
}

// Reset
void mcp2515_reset() {
    gpio_put(PIN_CS, 0);

    uint8_t cmd = MCP_RESET;
    spi_write_blocking(SPI_PORT, &cmd, 1);

    gpio_put(PIN_CS, 1);

    sleep_ms(10);
}

// Init
void mcp2515_init() {
    mcp2515_reset();

    // Set CAN bitrate
    spi_write_byte(0x2A, 0x00);
    spi_write_byte(0x29, 0x90);
    spi_write_byte(0x28, 0x02);

    // Enable receive buffer
    spi_write_byte(RXB0CTRL, 0x00);

    // Normal mode
    spi_write_byte(CANCTRL, 0x00);
}

// Receive message
void can_receive_message() {
    uint8_t intf = spi_read_byte(CANINTF);

    if (intf & 0x01) {
        uint8_t data = spi_read_byte(RXB0D0);

        printf("Message Received: 0x%X\n", data);

        // Clear flag
        spi_write_byte(CANINTF, 0x00);
    }
}

int main() {
    stdio_init_all();

    spi_init(SPI_PORT, 500000);

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    sleep_ms(2000);

    mcp2515_init();

    printf("CAN Receiver Ready...\n");

    while (1) {
        can_receive_message();
        sleep_ms(200);
    }
}