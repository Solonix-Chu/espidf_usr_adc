/**
 * @file usr_adc.c
 * @brief 用户ADC模块实现
 */

#include "usr_adc.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "USR_ADC";

// 全局变量，用于跟踪ADC单元的初始化状态
static bool g_adc1_initialized = false;
static bool g_adc2_initialized = false;
static adc_oneshot_unit_handle_t g_adc1_handle = NULL;
static adc_oneshot_unit_handle_t g_adc2_handle = NULL;
static int g_adc_ref_count = 0;  // 引用计数，用于跟踪使用ADC的模块数量

/**
 * @brief ADC模块句柄结构体
 */
struct usr_adc_handle_s {
    bool uses_adc1;                          /*!< 是否使用ADC1 */
    bool uses_adc2;                          /*!< 是否使用ADC2 */
    adc_cali_handle_t *cali_handles;         /*!< 校准句柄数组 */
    uint8_t cali_handle_num;                 /*!< 校准句柄数量 */
    usr_adc_channel_config_t *channel_configs; /*!< 通道配置数组 */
    uint8_t channel_num;                     /*!< 通道数量 */
};

esp_err_t usr_adc_init(usr_adc_channel_config_t *channel_configs, uint8_t channel_num, usr_adc_handle_t *handle)
{
    if (channel_configs == NULL || channel_num == 0 || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 分配句柄内存
    struct usr_adc_handle_s *adc_handle = calloc(1, sizeof(struct usr_adc_handle_s));
    if (adc_handle == NULL) {
        ESP_LOGE(TAG, "分配内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 保存通道配置
    adc_handle->channel_configs = malloc(channel_num * sizeof(usr_adc_channel_config_t));
    if (adc_handle->channel_configs == NULL) {
        ESP_LOGE(TAG, "分配通道配置内存失败");
        free(adc_handle);
        return ESP_ERR_NO_MEM;
    }
    memcpy(adc_handle->channel_configs, channel_configs, channel_num * sizeof(usr_adc_channel_config_t));
    adc_handle->channel_num = channel_num;

    // 分配校准句柄数组
    adc_handle->cali_handles = calloc(channel_num, sizeof(adc_cali_handle_t));
    if (adc_handle->cali_handles == NULL) {
        ESP_LOGE(TAG, "分配校准句柄内存失败");
        free(adc_handle->channel_configs);
        free(adc_handle);
        return ESP_ERR_NO_MEM;
    }

    // 初始化ADC单元和通道
    bool need_adc1 = false;
    bool need_adc2 = false;
    
    // 检查需要初始化的ADC单元
    for (int i = 0; i < channel_num; i++) {
        if (channel_configs[i].unit == ADC_UNIT_1) {
            need_adc1 = true;
            adc_handle->uses_adc1 = true;
        } else if (channel_configs[i].unit == ADC_UNIT_2) {
            need_adc2 = true;
            adc_handle->uses_adc2 = true;
        }
    }

    // 初始化ADC1（如果尚未初始化）
    if (need_adc1 && !g_adc1_initialized) {
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        esp_err_t ret = adc_oneshot_new_unit(&init_config1, &g_adc1_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "初始化ADC1失败: %s", esp_err_to_name(ret));
            free(adc_handle->cali_handles);
            free(adc_handle->channel_configs);
            free(adc_handle);
            return ret;
        }
        g_adc1_initialized = true;
        ESP_LOGI(TAG, "ADC1单元初始化成功");
    } else if (need_adc1) {
        ESP_LOGI(TAG, "ADC1单元已经初始化，复用现有实例");
    }

    // 初始化ADC2（如果尚未初始化）
    if (need_adc2 && !g_adc2_initialized) {
        adc_oneshot_unit_init_cfg_t init_config2 = {
            .unit_id = ADC_UNIT_2,
        };
        esp_err_t ret = adc_oneshot_new_unit(&init_config2, &g_adc2_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "初始化ADC2失败: %s", esp_err_to_name(ret));
            free(adc_handle->cali_handles);
            free(adc_handle->channel_configs);
            free(adc_handle);
            return ret;
        }
        g_adc2_initialized = true;
        ESP_LOGI(TAG, "ADC2单元初始化成功");
    } else if (need_adc2) {
        ESP_LOGI(TAG, "ADC2单元已经初始化，复用现有实例");
    }

    // 配置ADC通道
    for (int i = 0; i < channel_num; i++) {
        adc_oneshot_chan_cfg_t config = {
            .atten = channel_configs[i].atten,
            .bitwidth = channel_configs[i].bitwidth,
        };
        
        adc_oneshot_unit_handle_t unit_handle = (channel_configs[i].unit == ADC_UNIT_1) ? 
                                                g_adc1_handle : g_adc2_handle;
        
        esp_err_t ret = adc_oneshot_config_channel(unit_handle, channel_configs[i].channel, &config);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "配置通道失败，可能已被其他模块配置: %s", esp_err_to_name(ret));
            // 继续配置其他通道，不返回错误
        }
        
        // 初始化校准
        adc_cali_handle_t cali_handle = NULL;
        
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = channel_configs[i].unit,
            .atten = channel_configs[i].atten,
            .bitwidth = channel_configs[i].bitwidth,
        };
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = channel_configs[i].unit,
            .atten = channel_configs[i].atten,
            .bitwidth = channel_configs[i].bitwidth,
        };
        ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
#endif
        adc_handle->cali_handles[i] = cali_handle;
        adc_handle->cali_handle_num++;
    }

    // 增加引用计数
    g_adc_ref_count++;
    
    *handle = adc_handle;
    ESP_LOGI(TAG, "ADC模块初始化成功，当前引用计数: %d", g_adc_ref_count);
    return ESP_OK;
}

esp_err_t usr_adc_read_raw(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_raw)
{
    if (handle == NULL || out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_oneshot_unit_handle_t unit_handle;
    if (unit == ADC_UNIT_1) {
        if (!g_adc1_initialized || !handle->uses_adc1) {
            ESP_LOGE(TAG, "ADC1未初始化或当前句柄未使用ADC1");
            return ESP_ERR_INVALID_STATE;
        }
        unit_handle = g_adc1_handle;
    } else if (unit == ADC_UNIT_2) {
        if (!g_adc2_initialized || !handle->uses_adc2) {
            ESP_LOGE(TAG, "ADC2未初始化或当前句柄未使用ADC2");
            return ESP_ERR_INVALID_STATE;
        }
        unit_handle = g_adc2_handle;
    } else {
        ESP_LOGE(TAG, "无效的ADC单元");
        return ESP_ERR_INVALID_ARG;
    }

    return adc_oneshot_read(unit_handle, channel, out_raw);
}

esp_err_t usr_adc_read_voltage(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_voltage)
{
    if (handle == NULL || out_voltage == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 读取原始值
    int raw_value;
    esp_err_t ret = usr_adc_read_raw(handle, unit, channel, &raw_value);
    if (ret != ESP_OK) {
        return ret;
    }

    // 查找对应的校准句柄
    adc_cali_handle_t cali_handle = NULL;
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_configs[i].unit == unit && 
            handle->channel_configs[i].channel == channel) {
            cali_handle = handle->cali_handles[i];
            break;
        }
    }

    if (cali_handle == NULL) {
        ESP_LOGE(TAG, "未找到对应的校准句柄");
        return ESP_ERR_NOT_FOUND;
    }

    // 转换为电压值
    return adc_cali_raw_to_voltage(cali_handle, raw_value, out_voltage);
}

esp_err_t usr_adc_deinit(usr_adc_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 释放校准句柄
    for (int i = 0; i < handle->cali_handle_num; i++) {
        if (handle->cali_handles[i]) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
            ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_delete_scheme_curve_fitting(handle->cali_handles[i]));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
            ESP_ERROR_CHECK_WITHOUT_ABORT(adc_cali_delete_scheme_line_fitting(handle->cali_handles[i]));
#endif
        }
    }

    // 减少引用计数
    g_adc_ref_count--;
    ESP_LOGI(TAG, "ADC模块释放，当前引用计数: %d", g_adc_ref_count);

    // 只有当引用计数为0时才释放ADC单元
    if (g_adc_ref_count == 0) {
        // 删除ADC单元
        if (g_adc1_initialized && handle->uses_adc1) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_del_unit(g_adc1_handle));
            g_adc1_initialized = false;
            g_adc1_handle = NULL;
            ESP_LOGI(TAG, "ADC1单元已释放");
        }
        
        if (g_adc2_initialized && handle->uses_adc2) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_del_unit(g_adc2_handle));
            g_adc2_initialized = false;
            g_adc2_handle = NULL;
            ESP_LOGI(TAG, "ADC2单元已释放");
        }
    }

    // 释放内存
    free(handle->cali_handles);
    free(handle->channel_configs);
    free(handle);

    ESP_LOGI(TAG, "ADC模块句柄已释放");
    return ESP_OK;
} 