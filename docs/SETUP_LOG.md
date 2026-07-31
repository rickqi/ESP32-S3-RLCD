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

## 阶段十一：仓库结构重构

### 11.1 背景

定制固件原位于 `docs/examples/ESP32-S3-RLCD-4.2-GitHub/02_Example/ESP-IDF/02_WIFI_STA/`，与 10 个原厂示例混在 5 层目录深处，路径冗长且难以区分自有代码与上游参考代码。

### 11.2 结构调整

| 内容 | 旧路径 | 新路径 |
|------|--------|--------|
| 定制固件 | `docs/examples/.../ESP-IDF/02_WIFI_STA/` | `projects/wifi_sta/` |
| 截屏工具 | `screenshot.py`（根目录） | `tools/screenshot.py` |
| 固件报告 | `FIRMWARE_REPORT.md`（根目录） | `docs/FIRMWARE_REPORT.md` |
| 安装日志 | `SETUP_LOG.md`（根目录） | `docs/SETUP_LOG.md` |

新目录结构：
```
ESP32-S3-RLCD/
├── projects/wifi_sta/     # ★ 定制开发固件（独立管理）
├── tools/                 # 开发工具
├── docs/                  # 参考资料与文档
│   ├── examples/          # 原厂示例（只读参考）
│   ├── FIRMWARE_REPORT.md
│   └── SETUP_LOG.md
├── README.md
└── .gitignore
```

### 11.3 操作

- 20 个文件通过 `git mv` 移动（100% rename，保留 git 历史）
- README.md 更新 §4 目录结构、§13.3 构建路径（`cd projects/wifi_sta`）、§13.4 工具路径（`tools/screenshot.py`）、§13.5 仓库结构
- 原厂示例 `docs/examples/` 保持不动
- Commit: `0fa2be3`，已推送到 GitHub

### 11.4 构建路径变化

```bash
# 旧
cd docs/examples/ESP32-S3-RLCD-4.2-GitHub/02_Example/ESP-IDF/02_WIFI_STA

# 新
cd projects/wifi_sta
```

---

## 阶段十二：NTP 时钟显示功能

### 12.1 需求

用户反馈屏幕缺少时间信息，要求在状态栏 `Connected` 行的右端增加实时时钟显示。

### 12.2 方案

通过 SNTP（Simple Network Time Protocol）在 WiFi 连接后自动同步网络时间，显示北京时区（UTC+8）的 `HH:MM:SS`。

### 12.3 实现

**文件变更：**

| 文件 | 操作 | 说明 |
|------|------|------|
| `projects/wifi_sta/main/main.cpp` | **修改** | +`<time.h>`, `"esp_sntp.h"` 头文件；+`lbl_clock` 标签；+`start_sntp()`/`sntp_sync_cb()` 函数；UI 中右对齐时钟；每 2 秒刷新 |

**SNTP 初始化流程：**
```
WiFi Connected → start_sntp() [一次触发]
  → sntp_setoperatingmode(SNTP_OPMODE_POLL)
  → sntp_setservername(0, "ntp.aliyun.com")
  → sntp_setservername(1, "pool.ntp.org")
  → sntp_set_time_sync_notification_cb(sntp_sync_cb)
  → sntp_init()
  → setenv("TZ", "CST-8"), tzset()
```

**时钟 UI 布局：**
```cpp
lbl_clock = lv_label_create(scr);
lv_label_set_text(lbl_clock, "--:--:--");
lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -25, 48);
// 右端对齐，y=48 与 "Connected" 同行
```

**刷新逻辑：** `wifi_info_task` 每 2 秒执行，NTP 同步前显示 `--:--:--`，同步后 `localtime_r()` + `strftime("%H:%M:%S")` 显示实时时间。

### 12.4 编译产物

```
03_WIFI_Test.bin binary size 0xf8710 bytes (993KB)
Smallest app partition: 0x100000 (1MB), 0x78f0 bytes (3%) free.
```

新增 ~38KB（SNTP 协议栈 + mbedtls TLS 依赖）。

### 12.5 验证

- **串口日志确认：** `I (7227) WIFI_STA: NTP time synced`
- **截图像素分析：** 状态行右侧（x=340~374, y=53~60）检测到 32 个黑色像素，符合 `HH:MM:SS` 渲染
- **自动截图时机：** WiFi 连接后 ~4s 触发，NTP 同步后 ~0.3s 截图

### 12.6 屏幕布局（最终版）

```
WiFi STA
──────────────────────────────────────
Connected                        14:23:05    ← 时钟右对齐
SSID:   rickqi11
IP:     192.168.50.36
RSSI:   -58 dBm
MAC:    A4:CB:8F:DA:8C:9C
Uptime: 120 s
──────────────────────────────────────
WIFI_STA + ST7305 RLCD + LVGL
WiFi status auto-refresh @ 2s
```

---

## 阶段十三：小智 AI 固件定制优化

### 13.1 背景

小智 AI v2.1.0（`78/xiaozhi-esp32`）是一个基于 MCP 协议的开源 AI 语音助手，支持 ESP32-S3 平台。但默认配置不包含 waveshare-s3-rlcd-4.2 的完整板级支持，且主题颜色未针对 1-bit 反射屏优化。

### 13.2 板级支持

**新增/修改文件：**

| 文件 | 操作 | 说明 |
|------|------|------|
| `main/boards/waveshare-s3-rlcd-4.2/` | **新加** | 完整板级支持目录（config.h、custom_lcd_display、board class） |
| `main/boards/common/board.h` | **修改** | 新增 `GetI2cBus()` 虚方法 |
| `main/boards/waveshare-s3-rlcd-4.2/waveshare-s3-rlcd-4.2.cc` | **修改** | 添加 `GetI2cBus()` 实现、ADC 电池读取、完整板卡初始化 |

### 13.3 显示优化

| 修改 | 说明 |
|------|------|
| **1-bit 主题颜色** | 所有颜色强制为纯黑/纯白（1-bit 下渐变/彩色无意义） |
| **气泡改用边框区分** | 用户气泡细边框，助理气泡粗边框，替代原来的绿色/灰色 |
| **SHTC3 温湿度显示** | 读取板载温湿度传感器（I2C 0x70），状态栏循环显示 |
| **ADC 电池读取** | GPIO4 电压检测，3.0V~4.12V 映射 0~100% |
| **状态栏循环显示** | 温度 → 时钟 → 电池，每 4 秒切换 |
| **深色主题反转** | 黑色背景 + 白色文字，适配 1-bit 反转模式 |

### 13.4 状态栏布局（最终版）

```
┌────────────────────────────────┐
│ 📶           🔇 🔋            │ ← 顶部状态栏（WiFi/静音/电池图标）
├────────────────────────────────┤
│     31C  45%                   │ ← 中央循环显示
│     14:23                      │    温度湿度 → 时钟 → 电池
│     BAT 85%                    │    每 4 秒切换
├────────────────────────────────┤
│         😊                     │ ← 表情/GIF
│                                │
```

### 13.5 编译方式

```bash
cd D:\xiaozhi                          # 已拷贝到短路径避免 Windows 260 字符限制
idf.py set-target esp32s3
idf.py menuconfig                      # 选择 Board Type → waveshare-s3-rlcd-4.2
idf.py build
idf.py -p COM4 flash                   # 自动烧录应用 + assets 分区
```

### 13.6 关键配置

| 配置项 | 值 |
|--------|-----|
| 唤醒词模型 | `WN9_NIHAOXIAOZHI_TTS`（你好小智） |
| 唤醒词引擎 | AFE（Acoustic Front End）+ AEC |
| 音频编解码 | ES8311（输出）+ ES7210（4 通道输入） |
| 采样率 | 24kHz I2S TDM/STD |
| 显示驱动 | ST7305，SPI 40MHz，1-bit，LUT 查表 |
| 通信协议 | MQTT + WebSocket |

### 13.7 截图功能

小智 AI 固件已集成 `tools/screenshot.py` 截图协议（SCREENSHOT_START/base64/SCREENSHOT_END）。

**触发方式：**

| 方式 | 说明 |
|------|------|
| 自动截图 | 启动后 ~12 秒自动截一次（屏幕稳定后） |
| BOOT 长按 | 按住 BOOT 键 2 秒触发 |
| 串口 SHOOT | 已实现（USB-JTAG 下串口输入不可用） |

**使用：**
```bash
# 复位自动截图（推荐）
python tools/screenshot.py COM4

# 长按 BOOT 键截图
python tools/screenshot.py COM4 --listen
```

**代码实现：**
- `custom_lcd_display.h/.cc`：新增 `GetPixel(x,y)` 像素读取（LUT 反向映射）
- `waveshare-s3-rlcd-4.2.cc`：`TakeScreenshot()` 生成 P4 PBM → base64 → 串口输出

验证：`screenshot_20260731_214221.png` (400×300) 捕获成功。

### 13.8 语音查询 MCP 工具

参考 `self.audio_speaker.set_volume` 音量控制模式，新增 2 个语音可查询的设备状态工具：

| MCP 工具 | 触发语音 | 返回 |
|----------|---------|------|
| `self.get_temperature_humidity` | "现在多少度？" / "湿度多少？" | `{"temperature_celsius":31,"humidity_percent":45}` |
| `self.get_battery_level` | "电量还有多少？" | `{"level":85,"charging":false}` |

**调用机制**（MCP = 设备端运行的工具服务器）：
```
用户语音 → ASR → LLM 识别意图
  → 云端发送 JSON-RPC "tools/call"
  → 设备端 DoToolCall() 匹配工具名
  → 本地回调直接读硬件（SHTC3 I2C / ADC）
  → 返回 JSON 结果 → LLM 组织语言 → TTS 播报
```

**代码实现：**
- `board.h`：新增虚方法 `GetTemperatureHumidity()`（默认返回 false）
- `waveshare-s3-rlcd-4.2.cc`：SHTC3 读取方法 + 2 个 MCP 工具注册

验证：启动日志确认工具注册 `Add tool: self.get_temperature_humidity` / `self.get_battery_level`。

### 13.9 PCF85063 RTC 时钟集成

解决小智 AI 时钟依赖网络时间（OTA `settimeofday()`）的问题——开机后需等 10~30 秒才有时间显示，断电丢时间。

**集成方案：**

```
上电 → RtcReadTime() → settimeofday() → 时钟立即显示 ✅
OTA 服务器同步 → SyncRtcToSystemTime() → 写回 RTC ✅
断电 → RTC 电池持续走时 → 下次开机时间正确 ✅
```

**代码实现：**
- `waveshare-s3-rlcd-4.2.cc`：轻量 PCF85063 驱动（`RtcReadTime`/`RtcWriteTime`/`InitRtcClock`），使用新 IDF I2C API，**不依赖 SensorLib**
- `board.h`：新增虚方法 `SyncRtcToSystemTime()`（默认空实现）
- `application.cc`：OTA 激活后调用 `Board::SyncRtcToSystemTime()` 写回 RTC

**验证：**
```
I (219) waveshare_rlcd_4_2: RTC: boot time 2026-08-01 00:57:53   ← 启动即读
I (7249) waveshare_rlcd_4_2: RTC: synced to 2026-08-01 00:57:19  ← OTA 写回
重启验证: 时间从 00:57:19 → 00:57:53 持续走时（RTC 电池保持）✅
```

### 13.10 KEY 按键三功能 + 状态栏循环显示优化

**KEY 按键（GPIO18）三功能：**

| 操作 | 功能 | 实现 |
|------|------|------|
| KEY 单击 | 切换麦克风静音 | `codec->EnableInput(!muted)` + 底部通知 |
| KEY 双击 | 播放提示音 | `app.PlaySound(OGG_POPUP)` |
| KEY 长按 | 显示系统信息 | IP/MAC/版本 → 底部通知 6s |

**状态栏循环显示（20s 周期，带图标）：**

```
0-4s:   [温度计] 28°     ← FONT_AWESOME_TEMPERATURE_HALF
4-8s:   [云雨]   40%     ← FONT_AWESOME_CLOUD_DRIZZLE
8-12s:  [日历]   08-01   ← FONT_AWESOME_CALENDAR（新增日期）
12-16s: [时钟]   14:23   ← FONT_AWESOME_CLOCK
16-20s: [电池]   85%     ← 5级电池图标（按电量分级）
```

图标+数值用 **flex 行容器**整体居中（`status_icon_row_`），避免与顶部 WiFi 图标重叠、图标与数值间距过大。

**布局修复：**
- 通知从顶部 `status_bar_` 移到**底部** `bottom_bar_`（避免与循环显示重叠）
- 非 idle 状态（Listening/Speaking）隐藏循环行，避免与状态文字重叠
- SHTC3 温度公式加 `-4°C` 偏移（参考 05 示例 `SHTC3_PETP_VOL`），显示层统一调用板卡 `GetTemperatureHumidity()`
- BOOT 长按截图后显示底部通知（尺寸/PBM/b64 信息）

**验证：**
- `tools/screenshot.py COM4 --listen` + BOOT 长按 → 成功保存 PNG
- 像素分析：顶部状态栏 964 黑像素、循环区 506 黑像素、底部通知区正常

---

## 附录：工具与版本

| 工具 | 版本 | 用途 |
|------|------|------|
| esptool | v5.3.1 | Flash 擦除/烧录/读取 |
| ESP-IDF | v5.5.2 | 编译框架 + 组件库 |
| xtensa-esp-elf | esp-14.2.0 | 交叉编译工具链 |
| cmake | 3.30.2 | 构建系统 |
| Python | 3.13.1 | ESP-IDF 宿主 |
| LVGL | v8.4.0 / v9.3.0 | 图形 UI 框架（projects_wifi / XiaoZhi） |
| XiaoZhi AI | v2.1.0 | MCP 协议 AI 语音助手 |
| ESP-SR | — | 离线语音唤醒（Wakenet9） |
| Arduino CLI | — | esp32_llm 固件编译工具 |
| PowerShell | 5.1 | 系统命令执行 |
| OpenCode (Sisyphus) | — | AI 编排，并行 agent 调度 |

## 附录：关键文件路径

| 文件 | 路径 |
|------|------|
| README 安装手册 | `README.md` |
| 固件分析报告 | `docs/FIRMWARE_REPORT.md` |
| 本安装调试日志 | `docs/SETUP_LOG.md` |
| 截屏工具 | `tools/screenshot.py` |
| 定制固件主程序 | `projects/wifi_sta/main/main.cpp` |
| WiFi 初始化源码 | `projects/wifi_sta/components/esp_wifi_bsp/esp_wifi_bsp.c` |
| ST7305 显示驱动 | `projects/wifi_sta/components/port_bsp/display_bsp.cpp` |
| LVGL 端口层 | `projects/wifi_sta/components/app_bsp/lvgl_bsp.cpp` |
| XiaoZhi 板级配置 | `D:\xiaozhi\main\boards\waveshare-s3-rlcd-4.2\config.h` |
| XiaoZhi 显示驱动 | `D:\xiaozhi\main\boards\waveshare-s3-rlcd-4.2\custom_lcd_display.cc` |
| XiaoZhi 板卡类 | `D:\xiaozhi\main\boards\waveshare-s3-rlcd-4.2\waveshare-s3-rlcd-4.2.cc` |
| XiaoZhi 显示主题 | `D:\xiaozhi\main\display\lcd_display.cc` |
| XiaoZhi 分区表 | `D:\xiaozhi\partitions\v2\16m.csv` |
| FactoryProgram WiFi 源码 | `docs/examples/.../10_FactoryProgram/components/app_bsp/esp_wifi_bsp.c` |

---

*日志更新：2026-07-30 | OpenCode Sisyphus Agent*
