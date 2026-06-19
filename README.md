# ESP32-S3 桌面智能终端

[![PlatformIO](https://img.shields.io/badge/PlatformIO-IDE-blue)](https://platformio.org/)
[![ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3--N16R8-green)](https://www.espressif.com/)

基于 ESP32-S3 的桌面智能终端，集成语音控制、OLED 显示、心率监测、温湿度检测、天气/时间查询、电量检测等功能。软件采用 BSP/DRV/APP 三层架构，基于 FreeRTOS 多任务并发，搭载栈溢出监控、NVS 故障日志记录与带加密校验及异常回滚的 A/B 分区 IoT OTA 升级。

---

## 功能介绍

| 功能 | 说明 |
|------|------|
| 🎤 **语音控制** | SU-03T 语音模块，UART 指令切换功能 |
| 🖥️ **OLED 显示** | 0.96寸 128×64 OLED，显示时间、天气、心率、温湿度、WiFi 状态等 |
| 🫀 **心率监测** | MAX30102 传感器，10 秒测量，显示平均心率（BPM） |
| 🌡️ **温湿度检测** | DHT11 传感器，读取环境温湿度 |
| 🌤️ **天气查询** | HTTP 获取心知天气 API 数据并 JSON 解析显示 |
| ⏰ **网络授时** | HTTP 校时 + 硬件 RTC 持时，联网自动校准 |
| 🔋 **电量检测** | ADC 采集电池电压，实时显示百分比 |
| 🔊 **音频输出** | SU-03T 语音模块指令反馈语音播报 |
| 📡 **安全 OTA** | A/B 双分区 + SHA256 校验 + AES-128-CBC 加密 + 启动计数回滚 |
| 📊 **栈监控** | FreeRTOS 栈溢出钩子 + 周期性高水位打印 |
| 📝 **故障记录** | NVS 持久化存储复位原因与连续崩溃次数 |

---

## 硬件设计

### 引脚分配

| 外设 | 接口 | 引脚 |
|------|------|------|
| OLED SSD1306 | I2C0 | SDA=21, SCL=20 |
| SU-03T 语音模块 | UART | RX=41, TX=42 |
| MAX30102 心率 | I2C1 | SDA=16, SCL=15 |
| DHT11 温湿度 | GPIO | DATA=18 |
| 电池检测 | ADC | GPIO=4 |

> 原理图与 PCB 使用嘉立创 EDA 设计并打板，已焊接调试完成。

---

## 软件架构

### BSP / DRV / APP 三层架构

```
smart-desk-terminal/
├── src/                          # 源码
│   ├── main.cpp                  # 入口：setup() + loop()
│   ├── app/                      # 应用层
│   │   ├── app_main.cpp          # 系统初始化、任务创建、OTA 回滚检测、引导分区修正
│   │   ├── app_data.cpp          # 全局数据与互斥锁封装
│   │   ├── ota.cpp               # OTA 升级任务（HTTP + SHA256 + AES 流式解密）
│   │   ├── ota_aes.cpp           # 手写 AES-128-CBC 实现（密钥扩展 + CBC 模式 + 流式接口）
│   │   ├── ota_sha256.cpp        # 手写 SHA256 实现（64 轮压缩 + 流式更新）
│   │   ├── state_machine.cpp     # 状态机：空闲/时间/天气/心率/环境/电量
│   │   ├── fault_handler.cpp     # NVS 故障日志：复位原因 + 连续崩溃计数
│   │   └── stack_monitor.cpp     # 栈监控：FreeRTOS 溢出钩子 + 周期性高水位打印
│   ├── bsp/                      # 板级支持包
│   │   └── board.cpp             # 引脚初始化、I2C/UART 配置
│   └── drv/                      # 驱动层
│       ├── display_drv.cpp       # OLED 驱动（U8g2 封装，~1200 行 UI）
│       ├── dht11_drv.cpp         # DHT11 温湿度驱动
│       ├── max30102_drv.cpp      # MAX30102 心率驱动
│       ├── wifi_drv.cpp          # WiFi 状态管理
│       ├── time_drv.cpp          # HTTP 校时 + RTC 读时
│       ├── weather_drv.cpp       # 心知天气 API + JSON 解析
│       ├── battery_drv.cpp       # ADC 电池电压采集
│       └── self_test.cpp         # 开机自检（OLED 显示测试结果）
├── include/                      # 头文件（结构与 src 对应）
├── ota-server/                   # OTA 服务器工具
│   ├── encrypt_firmware.py       # 固件 AES 加密 + SHA256 生成
│   ├── serve.bat                 # 一键加密 + 启动 HTTP 服务器
│   └── version.txt               # 服务器端版本号
├── partitions_ota.csv            # 双分区表（app0 + app1 + otadata）
├── platformio.ini                # PlatformIO 配置
└── README.md
```

### 系统任务

| 任务 | 优先级 | 核心 | 栈大小 | 功能 |
|------|--------|------|--------|------|
| `stateTask` | 2 | 1 | 8KB | 状态机核心编排，20ms 循环 |
| `voiceTask` | 3 | 0 | 4KB | SU-03T 串口指令监听 |
| `sensorTask` | 2 | 0 | 4KB | DHT11 + 电池采集，2s 周期 |
| `wifiTask` | 1 | 1 | 4KB | WiFi 状态轮询，500ms 周期 |
| `otaTask` | 1 | 1 | 8KB | OTA 升级监听，10s 周期 |
| `otaHealthTask` | 1 | 1 | 2KB | 30s 延迟确认新固件稳定 |
| `stackMon` | 1 | 1 | 4KB | 栈余量监控，10s 周期打印 |

### 技术要点

- **BSP/DRV/APP 三层架构**：软硬件解耦，驱动接口统一，应用层不直接操作硬件
- **FreeRTOS 多任务**：消息队列 + 互斥锁，7 个任务并发调度，跨核心分配
- **栈溢出检测**：configCHECK_FOR_STACK_OVERFLOW=2（canary 法）+ 周期性 uxTaskGetStackHighWaterMark 打印
- **NVS 故障日志**：esp_reset_reason() 区分 PANIC / TaskWDT / Brownout / SW Reset 等，连续崩溃计数防死循环
- **手写安全算法**：AES-128-CBC 和 SHA256 均为手写软实现（非 mbedTLS），适配 ESP32 资源约束
- **开机自检**：POST 流程，OLED 显示各模块测试结果
- **WiFi 管理**：硬件自动重连 + 软件状态轮询
- **状态机**：7 种显示状态，语音/传感器驱动切换
- **引导分区自动修正**：USB 烧录后自动检测并修正 OTA 引导分区

### 安全 OTA 流程

```
USB 烧录 → 本地运行 serve.bat 启动 OTA 服务器
        → 设备检测新版本（HTTP version.txt）
        → 下载 firmware.enc（AES-128-CBC 加密固件）
        → pending-block 流式解密 + SHA256 实时校验
        → 写入 pending 回滚保护标记
        → Update.end() 切换 A/B 分区
        → 重启入新固件 → 30s 稳定后清除 pending
        → 异常崩溃 → 启动计数器 >=4 次 → 自动回滚旧分区
```

### Flash 分区布局

| 分区 | 偏移 | 大小 | 用途 |
|------|------|------|------|
| nvs | 0x9000 | 16KB | WiFi 配置、用户数据 |
| otadata | 0xD000 | 8KB | OTA 引导选择 |
| app0 (ota_0) | 0x10000 | 2MB | 固件 A |
| app1 (ota_1) | 0x210000 | 2MB | 固件 B |
| spiffs | 0x410000 | ~4MB | 文件存储 |

---

## 开发环境

| 项目 | 内容 |
|------|------|
| 主控芯片 | ESP32-S3 |
| 开发框架 | Arduino (PlatformIO) |
| 主要依赖 | U8g2, ArduinoJson, DHT sensor library |
| PCB 设计 | 嘉立创 EDA |

---

## GitHub

[https://github.com/ergouzi332/ESP32-smart-desk-terminal](https://github.com/ergouzi332/ESP32-smart-desk-terminal)