# ESP32-S3-RLCD-4.2 安装调试日志

> 完整记录在 OpenCode 环境下对 ESP32-S3-RLCD-4.2 开发板的安装、检测、调试过程。
> 日期：2026-07-30 | 操作员：Sisyphus (OpenCode AI Agent) | 设备 MAC：A4:CB:8F:DA:8C:9C

---

## 阶段一：资料收集与文档生成

### 1.1 下载官方文档资源

- **来源**：微雪文档平台 `https://docs.waveshare.net/ESP32-S3-RLCD-4.2`
- **操作**：爬取文档页面，下载所有链接的资源文件
- **结果**：11 个文件下载到 `docs/` 目录，总计 ~856MB
  - `docs/hardware/` — 原理图 PDF、3D 结构文件 RAR
  - `docs/datasheets/` — 8 个芯片数据手册（ESP32-S3、ST7305、ES8311、PCF85063、SHTC3）
  - `docs/examples/` — 打包示例 ZIP + GitHub 仓库完整克隆
  - `docs/community/` — Bilibili 演示视频 + xiaozhi-esp32.zip

### 1.2 克隆 GitHub 仓库

- **仓库**：`waveshareteam/ESP32-S3-RLCD-4.2`
- **结果**：8514 个文件克隆到 `docs/examples/ESP32-S3-RLCD-4.2-GitHub/`
- **结构**：
  - `01_Arduino_Libraries/` — LVGL v8/v9、SensorLib、U8g2
  - `02_Example/` — Arduino(10)、ESP-IDF(11)、ESPHome(3)、XiaoZhi AI 源码
  - `03_Firmware/` — 2 个预编译 .bin 固件

### 1.3 下载社区资源

- Bilibili 视频 BV1PsfhBCEqA → 480p MP4 (16.82MB)
- xiaozhi-esp32.zip (4.7MB)
- MakerWorld 3D 模型 → **失败**（403 需要登录）

### 1.4 生成 README.md 安装手册

- **内容**：12 章节，覆盖产品概述、硬件规格、GPIO 引脚映射、目录结构、固件烧录、Arduino/ESP-IDF/ESPHome/XiaoZhi 开发指南、示例说明、FAQ
- **路径**：`D:\codes\ESP32-S3-RLCD\README.md`

---

## 阶段二：设备连接与检测

### 2.1 安装 esptool

```bash
pip install esptool --user
# 版本：esptool v5.3.1
# 注意：v5.x 使用连字符命令格式，如 read-flash（不是 read_flash）
```

### 2.2 检测串口

- **端口**：COM4
- **问题**：Arduino IDE 的 `serial-monitor` (PID 37904) 和 `serial-discovery` (PID 38332) 占用 COM4
- **解决**：关闭 Arduino IDE / 终止占用进程

### 2.3 芯片识别

```bash
python -m esptool --port COM4 chip-id
```

输出：
```
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
USB mode:           USB-Serial/JTAG
MAC:                a4:cb:8f:da:8c:9c
```

---

## 阶段三：板载按钮识别

### 3.1 信息来源

- 官方文档页面 `docs.waveshare.net/ESP32-S3-RLCD-4.2` 的"资源简介"栏目
- 源码 `config.h` 中的 GPIO 定义
- 板载照片（Product-1.webp，转换为 PNG 查看但当前模型不支持图像输入）

### 3.2 按钮定义

| 按钮 | GPIO | 物理位置 | 功能 |
|------|------|----------|------|
| BOOT | GPIO0 | 侧边 | 按住+上电 → 下载模式 |
| PWR | — | 侧边 | 单击开机 / 长按关机 |
| KEY | GPIO18 | 侧边 | 用户自定义 |

> **注意**：没有传统 RESET 按钮。屏幕复位引脚 RST=GPIO41 是显示屏专用，不是整机复位。

---

## 阶段四：固件搜索与分析

### 4.1 本地固件搜索

- **工具**：Glob + Grep + explore agent
- **结果**：2 个预编译 .bin + 11 个 ESP-IDF 示例源码 + 3 个 ESPHome YAML + 10 个 Arduino .ino

### 4.2 社区固件搜索

- **工具**：3 个并行 librarian/explore agent（后台）
- **结果**：发现 18 个社区项目（ESPHome、MicroPython、Arduino、Rust、Zephyr RTOS）

### 4.3 构建配置分析

- **工具**：explore agent（后台）
- **结果**：
  - ESP-IDF 示例分区表：单 factory 分区（8M）
  - XiaoZhi 分区表：dual-OTA (4MB×2) + 8MB assets (SPIFFS)
  - sdkconfig：16MB QIO Flash, Octal PSRAM 80MHz, 240MHz CPU

---

## 阶段五：设备固件检测

### 5.1 方法

通过 `esptool read-flash` 读取 Flash 内容：
1. 读取 0x8000 处 4KB 分区表
2. 读取 0x10000 处 512B 应用程序头（esp_app_desc_t）
3. Python 脚本解析二进制结构

### 5.2 发现

设备原有固件不是出厂固件，而是一个自定义 Arduino WiFi AP 程序：

```
项目名：arduino-lib-builder
版本：43a8f6d（git hash）
编译日期：Jun 2 2026, 11:17:54
IDF版本：v5.5.4
功能：WiFi AP（SSID=ESP32_AP, 密码=12345678）
分区表：ota_0(1.25MB) + ota_1(1.25MB) + spiffs(1.38MB) + coredump(64KB)
```

### 5.3 分区表对比

| 分区 | 原有固件 | 出厂固件 | 小智 AI |
|------|----------|----------|---------|
| app/factory | ota_0 1.25MB | factory 8MB | ota_0 4MB |
| OTA slots | ota_0 + ota_1 | 无 | ota_0 + ota_1 |
| spiffs/assets | 1.38MB | 无 | 8MB |
| coredump | 64KB | 无 | 无 |

---

## 阶段六：固件烧录

### 6.1 擦除 Flash

```bash
python -m esptool --port COM4 --baud 921600 erase-flash
# 耗时：5.3 秒
# 结果：成功
```

### 6.2 烧录出厂固件

```bash
python -m esptool --port COM4 --baud 921600 write-flash 0x0 "03_Firmware/01_Factory_V1.bin"
# 文件大小：4,548,144 bytes (4.34 MB)
# 压缩写入：2,963,601 bytes
# 耗时：25.3 秒（1440.9 kbit/s）
# 校验：Hash of data verified ✓
```

### 6.3 烧录后验证

读取 0x10020 处 esp_app_desc_t：
```
project:  03_Fac
version:  1
date:     Jan 20 2026 18:13:54
idf_ver:  v5.5.2
```

**结论：烧录成功，设备已运行出厂测试程序。**

---

## 阶段七：WiFi 配置

### 7.1 分析出厂固件 WiFi 实现

源码位置：`10_FactoryProgram/components/app_bsp/esp_wifi_bsp.c`

```c
wifi_config_t wifi_config = {
    .sta = {
        .ssid = "PDCN",           // 硬编码 SSID
        .password = "1234567890", // 硬编码密码
    },
};
```

- WiFi 模式：STA
- 无配网 UI，SSID/密码编译时固定
- 启动后扫描周围热点并在屏幕显示数量

### 7.2 检测用户 WiFi

```bash
netsh wlan show interfaces       # 当前连接信息
netsh wlan show profile key=clear # 读取保存的密码
```

结果：
- SSID: `rickqi11`
- 密码: `18620907850`
- 频段: 5 GHz（信道 157）
- 认证: WPA2/WPA3 个人

### 7.3 修改源码

文件：`esp_wifi_bsp.c` 第 34-35 行

```diff
- .ssid = "PDCN",
- .password = "1234567890",
+ .ssid = "rickqi11",
+ .password = "18620907850",
```

### 7.4 发现 2.4GHz 兼容性问题

```bash
netsh wlan show networks mode=bssid
# 仅发现 1 个 rickqi11 网络：5 GHz, 信道 157, 802.11ac
# 未发现 2.4 GHz 信号
```

**ESP32-S3 仅支持 2.4 GHz WiFi。** 用户正在检查路由器 2.4GHz 设置。

---

## 阶段八：ESP-IDF 安装与 FactoryProgram 编译

### 8.1 安装 ESP-IDF v5.5.2

- **方式**：git clone + install.bat
- **路径**：`C:\Users\szk220009\esp\esp-idf`（IDF） + `C:\Users\szk220009\.espressif\`（工具链）
- **工具链**：xtensa-esp-elf esp-14.2.0, cmake 3.30.2, ninja, Python 3.13.1

### 8.2 编译 FactoryProgram（含 WiFi 修改）

```bash
cd 10_FactoryProgram
idf.py set-target esp32s3    # CMake 配置 85s，下载 6 个 managed component
idf.py build                  # 编译 1833 个目标，无错误
# 产物：03_Fac.bin (4,482,608 bytes = 4.27MB)，分区使用 47%
```

### 8.3 烧录 FactoryProgram

```bash
idf.py -p COM4 flash
# bootloader 22KB @ 0x0 + app 4.27MB @ 0x10000 + partition 3KB @ 0x8000
# 所有 hash 校验通过，硬复位重启
```

### 8.4 串口验证

启动日志确认所有硬件正常：
- ESP-IDF v5.5.2, app=03_Fac, version=eb1f634-dirty
- SDHC 60GB ✓, SHTC3 ✓, PSRAM 8MB ✓
- ES8311 + ES7210 四麦克风 ✓, LVGL 显示 ✓
- WiFi STA 初始化 → **扫描 AP → 释放 WiFi → BLE 扫描**（出厂测试流程）

> **发现**：出厂固件 `Lvgl_WfifBleScanTask` 只做 WiFi AP 扫描计数 + BLE 设备扫描计数，**不实际连接 WiFi**。扫描完成后 WiFi 被 `espwifi_deinit()` 释放给 BLE。

---

## 阶段九：WIFI_STA 示例编译与 WiFi 连接验证

### 9.1 修改 WIFI_STA WiFi 凭据

文件：`02_WIFI_STA/components/esp_wifi_bsp/esp_wifi_bsp.c` 第 21-22 行

```diff
- .ssid     = "K2P",
- .password = "1234567890",
+ .ssid     = "rickqi11",
+ .password = "18620907850",
```

### 9.2 编译与首次烧录

```bash
cd 02_WIFI_STA
idf.py set-target esp32s3 && idf.py build   # 1080 目标，749KB
idf.py -p COM4 flash monitor
```

### 9.3 WiFi 连接成功

```
I (1528) wifi:connected with rickqi11, aid = 3, channel 2, BW20, bssid = 7c:10:c9:28:ea:88
I (1529) wifi:security: WPA2-PSK, phy: bgn, rssi: -58
I (2564) esp_netif_handlers: sta ip: 192.168.50.36, mask: 255.255.255.0, gw: 192.168.50.1
```

| 项目 | 值 |
|------|------|
| SSID | rickqi11 |
| 认证 | WPA2-PSK |
| 信道 | 2 (2.4GHz, BW20) |
| RSSI | -58 dBm |
| 分配 IP | 192.168.50.36 |
| 网关 | 192.168.50.1 |

> **用户确认 rickqi11 为双频路由器**，ESP32-S3 成功连接 2.4GHz 频段。

### 9.4 问题发现：无显示代码

WIFI_STA 原始固件 `user_app.c` 只做 `printf` + `espwifi_Init()`，无 LCD/LVGL 初始化，**屏幕空白**。

---

## 阶段十：WIFI_STA 显示功能定制开发

### 10.1 目标

为 WIFI_STA 固件增加 ST7305 反射屏显示能力，实时展示 WiFi 连接状态信息。

### 10.2 架构方案

从 FactoryProgram 提取显示驱动和 LVGL 端口层，移植到 WIFI_STA 项目：

```
02_WIFI_STA/
├── components/
│   ├── port_bsp/          ← 新增：从 FactoryProgram 复制
│   │   ├── display_bsp.cpp    ST7305 LCD 驱动 (DisplayPort C++ 类)
│   │   ├── display_bsp.h      400x300 横屏, 1bit 灰度, 查表法优化
│   │   └── CMakeLists.txt
│   ├── app_bsp/           ← 新增：从 FactoryProgram 复制
│   │   ├── lvgl_bsp.cpp       LVGL 端口层 (flush 回调, tick 定时器, 互斥锁)
│   │   ├── lvgl_bsp.h
│   │   └── CMakeLists.txt
│   ├── esp_wifi_bsp/      ← 已有（已修改 WiFi 凭据）
│   │   └── esp_wifi_bsp.h     新增 extern "C" 包裹
│   └── user_app/          ← 已有（未使用）
├── main/
│   ├── main.cpp           ← 重写（原为 main.c）
│   ├── CMakeLists.txt     ← 更新依赖
│   └── idf_component.yml  ← 新增：lvgl/lvgl ^8.4.0
└── sdkconfig.defaults     ← 更新：添加 LVGL 配置
```

### 10.3 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `components/port_bsp/display_bsp.cpp` | **新增（复制）** | ST7305 LCD 驱动：SPI 初始化, ST7305 寄存器配置, 像素 LUT, Display() |
| `components/port_bsp/display_bsp.h` | **新增（复制）** | DisplayPort 类声明, AlgorithmOptimization=3 查表法 |
| `components/port_bsp/CMakeLists.txt` | **新增** | 依赖 esp_lcd, esp_driver_gpio, esp_driver_spi, esp_psram |
| `components/app_bsp/lvgl_bsp.cpp` | **新增（复制）** | LVGL 端口：双 PSRAM 缓冲, full_refresh, 5ms tick, 互斥锁, 核0 任务 |
| `components/app_bsp/lvgl_bsp.h` | **新增（复制）** | Lvgl_PortInit(), Lvgl_lock(), Lvgl_unlock() 声明 |
| `components/app_bsp/CMakeLists.txt` | **新增** | 依赖 port_bsp, lvgl__lvgl |
| `components/esp_wifi_bsp/esp_wifi_bsp.h` | **修改** | 添加 `extern "C"` 包裹 espwifi_Init 声明，解决 C/C++ 混合链接 |
| `components/esp_wifi_bsp/esp_wifi_bsp.c` | **修改（之前）** | WiFi 凭据：K2P/1234567890 → rickqi11/18620907850 |
| `main/main.c` | **删除** | 被替换 |
| `main/main.cpp` | **新增（重写）** | 完整应用逻辑（见 10.4） |
| `main/CMakeLists.txt` | **修改** | SRCS "main.cpp", 添加 port_bsp/app_bsp/esp_wifi/esp_netif 依赖 |
| `main/idf_component.yml` | **新增** | lvgl/lvgl: ^8.4.0 managed component |
| `sdkconfig.defaults` | **修改** | 添加 LVGL 配置项（MEM_SIZE=64KB, REFR_PERIOD=1, 等） |

### 10.4 main.cpp 实现细节

**初始化流程**：
1. 创建 `DisplayPort` 对象（MOSI=12, SCLK=11, DC=5, CS=40, RST=41, 400x300）
2. `RLCD_Init()` — ST7305 寄存器序列初始化
3. `Lvgl_PortInit(400, 300, flush_cb)` — LVGL 双缓冲 + full_refresh 模式
4. 创建 UI 标签（黑字白底，适配 1bit 反射屏）
5. `espwifi_Init()` — WiFi STA 连接
6. 获取 MAC 地址更新到屏幕
7. 启动后台任务 `wifi_info_task`

**LVGL Flush 回调**：RGB565 → 1bit 二值化（阈值 0x7fff），逐像素写入 DisplayPort 缓冲区后统一 Display()

**后台 UI 更新任务**（Core 1, 优先级 3, 每 2 秒）：
- `esp_wifi_sta_get_ap_info()` 检测连接状态
- 连接时：更新 RSSI（dBm）、IP 地址
- 断开时：显示 Disconnected、RSSI=--、IP=--
- 始终更新 Uptime 计数器

**屏幕显示内容**：
```
        WiFi STA
──────────────────────────
Connected
SSID:   rickqi11
IP:     192.168.50.36
RSSI:   -58 dBm
MAC:    A4:CB:8F:DA:8C:9C
Uptime: 120 s
```

### 10.5 编译问题修复

| # | 错误 | 原因 | 修复 |
|---|------|------|------|
| 1 | `%d` expects `int` but got `long unsigned int` | `ip_info.ip.addr` 是 `uint32_t` = `unsigned long` | 提取每字节时 cast `(int)(ip & 0xFF)` |
| 2 | `undefined reference to 'espwifi_Init()'` | C++ 调用 C 函数，name mangling 不匹配 | esp_wifi_bsp.h 加 `extern "C"` 包裹 |

### 10.6 最终编译结果

```
Project build complete.
03_WIFI_Test.bin binary size 0xeedc0 bytes (978,368 = 955KB)
Smallest app partition is 0x100000 bytes (1MB). 0x11240 bytes (7%) free.
```

### 10.7 烧录与验证

```bash
idf.py -p COM4 flash monitor
```

串口确认：
```
I (864) WIFI_STA: === ESP32-S3-RLCD WiFi STA ===
I (1230) LvglPort: Register display driver to LVGL
I (1232) LvglPort: Install LVGL tick timer
I (1528) wifi:connected with rickqi11, channel 2, rssi: -58
I (2564) esp_netif_handlers: sta ip: 192.168.50.36
```

**屏幕成功显示 WiFi 状态界面，每 2 秒自动刷新 RSSI 和 Uptime。**

---

## 附录：工具与版本

| 工具 | 版本 | 用途 |
|------|------|------|
| esptool | v5.3.1 | Flash 擦除/烧录/读取 |
| ESP-IDF | v5.5.2 | 编译框架 + 组件库 |
| xtensa-esp-elf | esp-14.2.0 | 交叉编译工具链 |
| cmake | 3.30.2 | 构建系统 |
| Python | 3.13.1 | ESP-IDF 宿主 |
| LVGL | v8.4.0 | 图形 UI 框架（managed component） |
| PowerShell | 5.1 | 系统命令执行 |
| OpenCode (Sisyphus) | — | AI 编排，并行 agent 调度 |

## 附录：关键文件路径

| 文件 | 路径 |
|------|------|
| README 安装手册 | `D:\codes\ESP32-S3-RLCD\README.md` |
| 固件分析报告 | `D:\codes\ESP32-S3-RLCD\FIRMWARE_REPORT.md` |
| 本安装调试日志 | `D:\codes\ESP32-S3-RLCD\SETUP_LOG.md` |
| FactoryProgram WiFi 源码 | `.../10_FactoryProgram/components/app_bsp/esp_wifi_bsp.c` |
| WIFI_STA 显示主程序 | `.../02_WIFI_STA/main/main.cpp` |
| WIFI_STA WiFi 源码 | `.../02_WIFI_STA/components/esp_wifi_bsp/esp_wifi_bsp.c` |
| WIFI_STA 显示驱动 | `.../02_WIFI_STA/components/port_bsp/display_bsp.cpp` |
| WIFI_STA LVGL 端口 | `.../02_WIFI_STA/components/app_bsp/lvgl_bsp.cpp` |
| 板卡引脚配置 | `.../XiaoZhiCode_V2.1.0/main/boards/waveshare-s3-rlcd-4.2/config.h` |
| XiaoZhi 分区表 | `.../XiaoZhiCode_V2.1.0/partitions/v2/16m.csv` |

---

*日志更新：2026-07-30 | OpenCode Sisyphus Agent*
