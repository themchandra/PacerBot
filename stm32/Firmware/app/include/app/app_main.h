/**
 * @file app_main.h
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Nov-03-2025
 */

#ifndef APP_MAIN_H_
#define APP_MAIN_H_

// C-callable functions
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initalizes firmware code and creates tasks */
void app_main(void);

#ifdef __cplusplus
}
#endif


#endif