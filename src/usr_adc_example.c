 /**
 * @file usr_adc_example.c
 * @brief usr_adc模块使用示例
 */

#include "usr_adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "USR_ADC_EXAMPLE";

void app_main_test(void)
{
    // 定义ADC通道配置
    usr_adc_channel_config_t adc_configs[] = {
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_0,     // GPIO36
            .atten = ADC_ATTEN_DB_11,     // 0-3.3V
            .bitwidth = ADC_BITWIDTH_12,  // 12位分辨率
        },
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_3,     // GPIO39
            .atten = ADC_ATTEN_DB_11,     // 0-3.3V
            .bitwidth = ADC_BITWIDTH_12,  // 12位分辨率
        },
    };
    
    // 初始化ADC模块
    usr_adc_handle_t adc_handle = NULL;
    esp_err_t ret = usr_adc_init(adc_configs, sizeof(adc_configs) / sizeof(adc_configs[0]), &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC初始化失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 循环读取ADC值
    while (1) {
        // 读取原始值
        int raw_value1 = 0;
        ret = usr_adc_read_raw(adc_handle, ADC_UNIT_1, ADC_CHANNEL_0, &raw_value1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取ADC1_CH0原始值失败: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "ADC1_CH0原始值: %d", raw_value1);
        }
        
        // 读取电压值
        int voltage1 = 0;
        ret = usr_adc_read_voltage(adc_handle, ADC_UNIT_1, ADC_CHANNEL_0, &voltage1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "读取ADC1_CH0电压值失败: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "ADC1_CH0电压值: %d mV", voltage1);
        }
        
        // 读取第二个通道
        int raw_value2 = 0;
        int voltage2 = 0;
        ret = usr_adc_read_raw(adc_handle, ADC_UNIT_1, ADC_CHANNEL_3, &raw_value2);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ADC1_CH3原始值: %d", raw_value2);
            
            ret = usr_adc_read_voltage(adc_handle, ADC_UNIT_1, ADC_CHANNEL_3, &voltage2);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "ADC1_CH3电压值: %d mV", voltage2);
            }
        }
        
        // 延时1秒
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // 注意：在实际应用中，应该在不再需要ADC时释放资源
    // usr_adc_deinit(adc_handle);
} 