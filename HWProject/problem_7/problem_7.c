#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5

#define MPU_ADDR 0x68

// Write 1 byte to register
void mpu_write(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    i2c_write_blocking(I2C_PORT, MPU_ADDR, buf, 2, false);
}

// Read multiple bytes
void mpu_read(uint8_t reg, uint8_t *buffer, uint8_t length) {
    i2c_write_blocking(I2C_PORT, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU_ADDR, buffer, length, false);
}

// Convert raw data
int16_t read_word(uint8_t reg) {
    uint8_t data[2];
    mpu_read(reg, data, 2);
    return (int16_t)((data[0] << 8) | data[1]);
}

int main() {
    stdio_init_all();   // VERY IMPORTANT for PuTTY

    // Init I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    sleep_ms(2000);  // wait for serial

    printf("MPU6050 STARTED\n");

    // Wake MPU6050
    mpu_write(0x6B, 0x00);

    while (1) {
        uint8_t buffer[14];

        // Read all data at once (CRITICAL FIX)
        i2c_write_blocking(I2C_PORT, MPU_ADDR, (uint8_t[]){0x3B}, 1, true);
        int ret = i2c_read_blocking(I2C_PORT, MPU_ADDR, buffer, 14, false);

        if (ret < 0) {
            printf("READ ERROR\n");
            sleep_ms(1000);
            continue;
        }

        int16_t acc_x = (buffer[0] << 8) | buffer[1];
        int16_t acc_y = (buffer[2] << 8) | buffer[3];
        int16_t acc_z = (buffer[4] << 8) | buffer[5];

        int16_t gyro_x = (buffer[8] << 8) | buffer[9];
        int16_t gyro_y = (buffer[10] << 8) | buffer[11];
        int16_t gyro_z = (buffer[12] << 8) | buffer[13];

        float ax = acc_x / 16384.0;
        float ay = acc_y / 16384.0;
        float az = acc_z / 16384.0;

        float gx = gyro_x / 131.0;
        float gy = gyro_y / 131.0;
        float gz = gyro_z / 131.0;

        printf("Accelerometer (g): X=%.2f Y=%.2f Z=%.2f\n", ax, ay, az);
        printf("Gyroscope (deg/s): X=%.2f Y=%.2f Z=%.2f\n", gx, gy, gz);

        printf("------------------------------------\n");

        sleep_ms(500);
    }
}