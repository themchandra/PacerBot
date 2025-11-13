#include "hal/imu.h"

void scan_i2c(void){
 printf("Starting I2C Scan\n");

  // Go through all the possible I2C addresses
  for (uint8_t i = 0; i < 128; i++) {
	  if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 3, 5) == HAL_OK)
		  printf("%2x", i);
	  else
		  printf("-- ");

	  // new line every 16 addresses
	  if (i > 0 && (i + 1) % 16 == 0)
		  printf("\n");
		 }

  printf("\n");
    }

void read_accel_data(void) {
	uint8_t data[6];
	HAL_I2C_Mem_Read(&hi2c1, IMU_ADDRESS << 1, ACCEL_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_ax =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_ay =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_az =    (int16_t)((data[4] << 8) | data[5]);

	float ax = raw_ax / ACCEL_SENSITIVITY;
	float ay = raw_ay / ACCEL_SENSITIVITY;
	float az = raw_az / ACCEL_SENSITIVITY;

	printf("x=%.3f, y=%.3f, z=%.3f\n",ax, ay, az);
}

 void read_gyro_data() {
	uint8_t data[6];
	HAL_I2C_Mem_Read(&hi2c1, IMU_ADDRESS << 1, GYRO_XOUT_H, 1, data, 6, HAL_MAX_DELAY);
	int16_t raw_gx =    (int16_t)((data[0] << 8) | data[1]);
	int16_t raw_gy =    (int16_t)((data[2] << 8) | data[3]);
	int16_t raw_gz =    (int16_t)((data[4] << 8) | data[5]);

	float gx = raw_gx * 0.00763358778;
	float gy = raw_gy * 0.00763358778;
	float gz = raw_gz * 0.00763358778;


	printf("gx=%.3f, gy=%.3f, gz=%.3f\n",gx, gy, gz);
} 