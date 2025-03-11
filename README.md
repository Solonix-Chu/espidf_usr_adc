# usr_adc 模块使用说明

## 简介

`usr_adc` 是一个基于ESP-IDF的ADC（模数转换器）模块封装，提供简单易用的ADC读取接口。该模块支持ESP32系列芯片的ADC功能，可以同时使用多个ADC通道，并支持多个模块共享ADC资源。

## 功能特点

- 支持ESP32的ADC1和ADC2单元
- 支持多通道配置和读取
- 支持读取原始值和校准后的电压值(mV)
- 提供简单的初始化和反初始化接口
- 支持多模块共享ADC资源，避免重复初始化
- 自动处理ADC校准

## 文件结构

- `include/usr_adc.h`: 模块头文件，定义接口和数据结构
- `src/usr_adc.c`: 模块实现文件
- `src/usr_adc_example.c`: 使用示例

## 安装方法

1. 将`usr_adc`目录复制到您的ESP-IDF项目的`components`目录下
2. 在项目的`CMakeLists.txt`中添加对`usr_adc`组件的依赖：
   ```cmake
   REQUIRES usr_adc
   ```

## API说明

### 数据结构

#### usr_adc_channel_config_t

ADC通道配置结构体，用于配置ADC通道的参数。

```c
typedef struct {
    adc_unit_t unit;           // ADC单元 (ADC1 或 ADC2)
    adc_channel_t channel;     // ADC通道
    adc_atten_t atten;         // ADC衰减
    adc_bitwidth_t bitwidth;   // ADC位宽
} usr_adc_channel_config_t;
```

#### usr_adc_handle_t

ADC模块句柄，用于操作ADC。

```c
typedef struct usr_adc_handle_s* usr_adc_handle_t;
```

### 函数接口

#### usr_adc_init

初始化ADC模块。

```c
esp_err_t usr_adc_init(usr_adc_channel_config_t *channel_configs, uint8_t channel_num, usr_adc_handle_t *handle);
```

参数：
- `channel_configs`: ADC通道配置数组
- `channel_num`: 通道数量
- `handle`: 返回的ADC句柄

返回值：
- `ESP_OK`: 初始化成功
- 其他ESP错误码: 初始化失败

#### usr_adc_read_raw

读取ADC原始值。

```c
esp_err_t usr_adc_read_raw(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_raw);
```

参数：
- `handle`: ADC句柄
- `unit`: ADC单元
- `channel`: ADC通道
- `out_raw`: 输出的原始值

返回值：
- `ESP_OK`: 读取成功
- 其他ESP错误码: 读取失败

#### usr_adc_read_voltage

读取ADC电压值(mV)。

```c
esp_err_t usr_adc_read_voltage(usr_adc_handle_t handle, adc_unit_t unit, adc_channel_t channel, int *out_voltage);
```

参数：
- `handle`: ADC句柄
- `unit`: ADC单元
- `channel`: ADC通道
- `out_voltage`: 输出的电压值(mV)

返回值：
- `ESP_OK`: 读取成功
- 其他ESP错误码: 读取失败

#### usr_adc_deinit

反初始化ADC模块，释放资源。

```c
esp_err_t usr_adc_deinit(usr_adc_handle_t handle);
```

参数：
- `handle`: ADC句柄

返回值：
- `ESP_OK`: 释放成功
- 其他ESP错误码: 释放失败

## 使用示例

### 基本使用流程

1. 定义ADC通道配置
2. 初始化ADC模块
3. 读取ADC值
4. 释放ADC资源

```c
#include "usr_adc.h"
#include "esp_log.h"

static const char *TAG = "ADC_EXAMPLE";

void adc_example(void)
{
    // 1. 定义ADC通道配置
    usr_adc_channel_config_t adc_configs[] = {
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_0,     // GPIO36 (ESP32)
            .atten = ADC_ATTEN_DB_11,     // 0-3.3V
            .bitwidth = ADC_BITWIDTH_12,  // 12位分辨率
        },
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_3,     // GPIO39 (ESP32)
            .atten = ADC_ATTEN_DB_11,     // 0-3.3V
            .bitwidth = ADC_BITWIDTH_12,  // 12位分辨率
        },
    };
    
    // 2. 初始化ADC模块
    usr_adc_handle_t adc_handle = NULL;
    esp_err_t ret = usr_adc_init(adc_configs, sizeof(adc_configs) / sizeof(adc_configs[0]), &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC初始化失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 3. 读取ADC值
    // 读取原始值
    int raw_value = 0;
    ret = usr_adc_read_raw(adc_handle, ADC_UNIT_1, ADC_CHANNEL_0, &raw_value);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC1_CH0原始值: %d", raw_value);
    }
    
    // 读取电压值
    int voltage = 0;
    ret = usr_adc_read_voltage(adc_handle, ADC_UNIT_1, ADC_CHANNEL_0, &voltage);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC1_CH0电压值: %d mV", voltage);
    }
    
    // 4. 释放ADC资源
    usr_adc_deinit(adc_handle);
}
```

### 多模块共享ADC资源

`usr_adc`模块支持多个模块共享ADC资源，每个模块可以独立调用`usr_adc_init`和`usr_adc_deinit`，而不会导致资源冲突。

```c
// 模块A
void module_a_init(void)
{
    usr_adc_channel_config_t adc_configs[] = {
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_0,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_12,
        },
    };
    
    usr_adc_handle_t adc_handle_a = NULL;
    usr_adc_init(adc_configs, 1, &adc_handle_a);
    
    // 使用ADC...
    
    // 模块卸载时
    // usr_adc_deinit(adc_handle_a);
}

// 模块B
void module_b_init(void)
{
    usr_adc_channel_config_t adc_configs[] = {
        {
            .unit = ADC_UNIT_1,
            .channel = ADC_CHANNEL_3,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_12,
        },
    };
    
    usr_adc_handle_t adc_handle_b = NULL;
    usr_adc_init(adc_configs, 1, &adc_handle_b);
    
    // 使用ADC...
    
    // 模块卸载时
    // usr_adc_deinit(adc_handle_b);
}
```

## 注意事项

1. **ADC2与Wi-Fi共存问题**：
   - ESP32的ADC2在使用Wi-Fi时可能会受到影响，建议优先使用ADC1
   - 如果必须使用ADC2，请注意在Wi-Fi活动期间可能会出现读取错误

2. **衰减设置与输入电压范围**：
   不同的衰减设置对应不同的输入电压范围：
   - `ADC_ATTEN_DB_0`: 0 - 1.1V
   - `ADC_ATTEN_DB_2_5`: 0 - 1.5V
   - `ADC_ATTEN_DB_6`: 0 - 2.2V
   - `ADC_ATTEN_DB_11`: 0 - 3.3V

3. **通道配置冲突**：
   - 当多个模块使用相同通道但配置不同时，后初始化的模块配置会覆盖先初始化的模块配置
   - 建议不同模块使用不同通道，或者使用相同的通道配置

4. **资源释放**：
   - 每个模块应该在不再需要ADC时调用`usr_adc_deinit`释放资源
   - 只有当最后一个使用ADC的模块释放资源时，ADC硬件才会被真正释放

5. **校准功能**：
   - 校准功能依赖于ESP-IDF的校准方案，不同芯片可能支持不同的校准方法
   - 模块会自动选择可用的校准方法

## 支持的芯片

- ESP32
- ESP32-S2
- ESP32-S3
- ESP32-C3
- 其他支持ADC功能的ESP32系列芯片

## 许可证

此模块遵循 [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) 开源许可证。 