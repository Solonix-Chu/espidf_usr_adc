/**
 * @file usr_adc.h
 * @brief 用户ADC模块头文件
 */

#ifndef USR_ADC_H
#define USR_ADC_H

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC通道配置结构体
 */
typedef struct {
    adc_unit_t unit;           /*!< ADC单元 (ADC1 或 ADC2) */
    adc_channel_t channel;     /*!< ADC通道 */
    adc_atten_t atten;         /*!< ADC衰减 */
    adc_bitwidth_t bitwidth;   /*!< ADC位宽 */
} usr_adc_channel_config_t;

/**
 * @brief ADC模块句柄
 */
typedef struct usr_adc_handle_s* usr_adc_handle_t;

/**
 * @brief 初始化ADC模块
 * 
 * @param[in] channel_configs ADC通道配置数组
 * @param[in] channel_num 通道数量
 * @param[out] handle 返回的ADC句柄
 * @return esp_err_t 
 */
esp_err_t usr_adc_init(usr_adc_channel_config_t *channel_configs, uint8_t channel_num, usr_adc_handle_t *handle);

/**
 * @brief 读取ADC原始值
 * 
 * @param[in] handle ADC句柄
 * @param[in] unit ADC单元
 * @param[in] channel ADC通道
 * @param[out] out_raw 输出的原始值
 * @return esp_err_t 
 */
esp_err_t usr_adc_read_raw(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_raw);

/**
 * @brief 读取ADC电压值(mV)
 * 
 * @param[in] handle ADC句柄
 * @param[in] unit ADC单元
 * @param[in] channel ADC通道
 * @param[out] out_voltage 输出的电压值(mV)
 * @return esp_err_t 
 */
esp_err_t usr_adc_read_voltage(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_voltage);

/**
 * @brief 反初始化ADC模块
 * 
 * @param[in] handle ADC句柄
 * @return esp_err_t 
 */
esp_err_t usr_adc_deinit(usr_adc_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* USR_ADC_H */ 