/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "app/app_main.h"

#include "cmsis_os.h"
#include "main.h"
#include "hal/imu.h"
#include <cstdio>

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
}

extern "C" void app_main(void) {
    // initialize other modules and start new threads and stuff
    // call C++ functions here
    //std::array<float, 3> a_data;
    std::array<int,3> raw_data;
    
    IMU MPU6050(&hi2c1,0x68);

    MPU6050.scan_i2c();

    while (true){
       // MPU6050.get_gyro(a_data);
       //printf("x =%f, y=%f , z=%f\n",a_data[0],a_data[1],a_data[2]);
       MPU6050.get_accel_raw(raw_data);
       printf("x =%d, y=%d , z=%d\n",raw_data[0],raw_data[1],raw_data[2]);
       vTaskDelay(500);
    }
}