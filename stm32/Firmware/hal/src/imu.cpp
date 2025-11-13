#include "hal/imu.h"
#include "stm32f4xx_hal_i2c.h"
#include <array>


IMU::IMU(I2C_HandleTypeDef *handle, int address)
	: hi2c_(handle), address_(address)
	{}

void IMU::scan_i2c(){
 printf("Starting I2C Scan\n");

  // Go through all the possible I2C addresses
  for (uint8_t i = 0; i < 128; i++) {
	  if (HAL_I2C_IsDeviceReady(hi2c_, (uint16_t)(i << 1), 3, 5) == HAL_OK)
		  printf("%2x", i);
	  else
		  printf("-- ");

	  // new line every 16 addresses
	  if (i > 0 && (i + 1) % 16 == 0)
		  printf("\n");
		 }

  printf("\n");
    }

void IMU::get_accel_raw(std::array<int, 3> & accel) {
	uint8_t data[6];
	HAL_I2C_Mem_Read(hi2c_, address_ << 1, ACCEL_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_ax =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_ay =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_az =    (int16_t)((data[4] << 8) | data[5]);

	accel[0] = raw_ax;
	accel[1] = raw_ay;
	accel[2] = raw_az;
}
	
void IMU::get_accel(std::array<float, 3> & accel) {
	uint8_t data[6];
	HAL_I2C_Mem_Read(hi2c_, address_ << 1, ACCEL_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_ax =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_ay =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_az =    (int16_t)((data[4] << 8) | data[5]);

	// divide by sensivity to get g units
	accel[0] = raw_ax / ACCEL_SENSITIVITY;
	accel[1] = raw_ay / ACCEL_SENSITIVITY;
	accel[2] = raw_az / ACCEL_SENSITIVITY;
}

void IMU::get_gyro_raw(std::array<int, 3> & gyro) {
	uint8_t data[6];
	HAL_I2C_Mem_Read(hi2c_, address_ << 1, GYRO_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_gx =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_gy =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_gz =    (int16_t)((data[4] << 8) | data[5]);

	gyro[0] = raw_gx;
	gyro[1] = raw_gy;
	gyro[2] = raw_gz;
} 

 void IMU::get_gyro(std::array<float, 3> & gyro) {
	uint8_t data[6];
	HAL_I2C_Mem_Read(hi2c_, address_ << 1, GYRO_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_gx =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_gy =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_gz =    (int16_t)((data[4] << 8) | data[5]);

	gyro[0] = raw_gx / GYRO_SENSITIVITY;
	gyro[1] = raw_gy / GYRO_SENSITIVITY;
	gyro[2] = raw_gz / GYRO_SENSITIVITY;
} 