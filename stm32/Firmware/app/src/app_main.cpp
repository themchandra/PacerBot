/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Dec-17-2025
 */

#include "app/app_main.h"

#include "cmsis_os.h"
#include "hal/imu.h"
#include "hal/ultrasonic.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"
#include <cstdint>
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

// Forward HAL timer input-capture callback into the Ultrasonic class
extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    (void)htim;
    if (Ultrasonic::instance)
        Ultrasonic::instance->handle_capture_callback();
}

extern "C" void app_main(void)
{
    // initialize other modules and start new threads and stuff
    // call C++ functions here
    // std::array<float, 3> a_data;
    // std::array<int, 3> raw_data;

    IMU MPU6050(&hi2c1, 0x68);
    Ultrasonic HC(&htim1, TIM_CHANNEL_1, GPIOB, GPIO_PIN_10);

    MPU6050.scan_i2c();

    while (true) {
        HC.trigger();
        float distance;
        distance = HC.get_distance_cm();
        printf("Distance = %f\n", distance);
        //  MPU6050.get_gyro(a_data);
        //  printf("x =%f, y=%f , z=%f\n",a_data[0],a_data[1],a_data[2]);
        // MPU6050.get_accel(a_data);
        // printf("x =%f, y=%f , z=%f\n", a_data[0], a_data[1], a_data[2]);


        vTaskDelay(500);
    }
}