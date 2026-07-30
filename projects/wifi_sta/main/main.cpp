/**
 * 02_WIFI_STA with ST7305 RLCD display
 * 
 * Custom firmware: WiFi connection + real-time status display
 * Hardware: ESP32-S3-RLCD-4.2 (400x300 reflective LCD)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "driver/gpio.h"
#include "mbedtls/base64.h"

#include "display_bsp.h"
#include "lvgl_bsp.h"
#include "esp_wifi_bsp.h"

static const char *TAG = "WIFI_STA";

// Display: MOSI=12, SCLK=11, DC=5, CS=40, RST=41, 400x300 landscape
static DisplayPort *g_display = nullptr;

// UI labels
static lv_obj_t *lbl_title;
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_clock;
static lv_obj_t *lbl_ssid;
static lv_obj_t *lbl_ip;
static lv_obj_t *lbl_rssi;
static lv_obj_t *lbl_mac;
static lv_obj_t *lbl_uptime;

// SNTP state
static volatile bool sntp_started = false;
static volatile bool sntp_synced = false;

// LVGL flush callback: convert RGB565 -> 1-bit for ST7305
static void take_screenshot(void);

// SNTP: sync real-world clock after WiFi connects
static void sntp_sync_cb(struct timeval *tv)
{
    sntp_synced = true;
    ESP_LOGI(TAG, "NTP time synced");
}

static void start_sntp(void)
{
    if (sntp_started) return;
    sntp_started = true;

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");   // Alibaba NTP (fast in China)
    sntp_setservername(1, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(sntp_sync_cb);
    sntp_init();

    setenv("TZ", "CST-8", 1);   // UTC+8 China Standard Time
    tzset();
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    uint16_t *buffer = (uint16_t *)color_map;
    for (int y = area->y1; y <= area->y2; y++) {
        for (int x = area->x1; x <= area->x2; x++) {
            uint8_t color = (*buffer < 0x7fff) ? ColorBlack : ColorWhite;
            g_display->RLCD_SetPixel(x, y, color);
            buffer++;
        }
    }
    g_display->RLCD_Display();
    lv_disp_flush_ready(drv);
}

// Background task: poll WiFi state and update UI every 2 seconds
static void wifi_info_task(void *arg)
{
    bool was_connected = false;
    uint32_t boot_sec = 0;
    bool auto_shot_done = false;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        boot_sec += 2;

        wifi_ap_record_t ap;
        bool connected = (esp_wifi_sta_get_ap_info(&ap) == ESP_OK);

        if (connected) {
            start_sntp();   // one-shot, fires on first connect
        }

        if (Lvgl_lock(100)) {
            char buf[80];

            if (connected) {
                if (!was_connected) {
                    lv_label_set_text(lbl_status, "Connected");
                    was_connected = true;
                }
                snprintf(buf, sizeof(buf), "RSSI:   %d dBm", ap.rssi);
                lv_label_set_text(lbl_rssi, buf);

                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
                uint32_t ip = ip_info.ip.addr;
                snprintf(buf, sizeof(buf), "IP:     %d.%d.%d.%d",
                    (int)(ip & 0xFF), (int)((ip >> 8) & 0xFF),
                    (int)((ip >> 16) & 0xFF), (int)((ip >> 24) & 0xFF));
                lv_label_set_text(lbl_ip, buf);

                // Auto-screenshot 4s after first connection (display settled)
                if (!auto_shot_done && boot_sec >= 4) {
                    auto_shot_done = true;
                    Lvgl_unlock();
                    ESP_LOGI(TAG, "Auto screenshot after WiFi connect");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    take_screenshot();
                    continue;
                }
            } else {
                if (was_connected) {
                    lv_label_set_text(lbl_status, "Disconnected");
                    was_connected = false;
                }
                lv_label_set_text(lbl_rssi, "RSSI:   --");
                lv_label_set_text(lbl_ip,   "IP:     --");
            }

            // Clock: update every loop (real time after NTP sync)
            if (sntp_synced) {
                time_t now;
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
                lv_label_set_text(lbl_clock, buf);
            }

            snprintf(buf, sizeof(buf), "Uptime: %lu s", (unsigned long)boot_sec);
            lv_label_set_text(lbl_uptime, buf);

            Lvgl_unlock();
        }
    }
}

/*
 * Screenshot via serial command.
 * Protocol:
 *   PC → MCU:  "SHOOT\n"
 *   MCU → PC:  "SCREENSHOT_START\n"
 *              <base64 of PBM, 72 chars per line>
 *              "SCREENSHOT_END\n"
 *
 * PBM format (P4 binary):
 *   Header: "P4\n400 300\n"
 *   Data:   1-bit, MSB first, top-to-bottom, 1=black 0=white
 *   Size:   50 bytes/row × 300 rows = 15000 bytes
 */
static void take_screenshot(void)
{
    int w = g_display->GetWidth();
    int h = g_display->GetHeight();
    int row_bytes = (w + 7) / 8;                    // 50 for 400px
    int hdr_len = snprintf(NULL, 0, "P4\n%d %d\n", w, h);
    int pbm_size = hdr_len + row_bytes * h;         // 13 + 15000 = 15013

    uint8_t *pbm = (uint8_t *)malloc(pbm_size);
    if (!pbm) {
        printf("SCREENSHOT_ERROR: out of memory\n");
        return;
    }

    // Write PBM header
    snprintf((char *)pbm, hdr_len + 1, "P4\n%d %d\n", w, h);

    // Lock LVGL to freeze the framebuffer during capture
    if (!Lvgl_lock(500)) {
        printf("SCREENSHOT_ERROR: LVGL lock timeout\n");
        free(pbm);
        return;
    }

    // Convert DispBuffer → PBM pixel data (row-major, MSB first, 1=black)
    uint8_t *pdata = pbm + hdr_len;
    for (int y = 0; y < h; y++) {
        for (int bx = 0; bx < row_bytes; bx++) {
            uint8_t byte = 0;
            for (int b = 0; b < 8; b++) {
                int x = bx * 8 + b;
                if (x >= w) break;
                if (g_display->GetPixel(x, y) == ColorBlack)
                    byte |= (0x80 >> b);
            }
            *pdata++ = byte;
        }
    }

    Lvgl_unlock();

    // Base64 encode
    size_t b64_len = 0;
    mbedtls_base64_encode(NULL, 0, &b64_len, pbm, pbm_size);
    uint8_t *b64 = (uint8_t *)malloc(b64_len + 1);
    if (!b64) {
        free(pbm);
        printf("SCREENSHOT_ERROR: base64 alloc\n");
        return;
    }
    mbedtls_base64_encode(b64, b64_len, &b64_len, pbm, pbm_size);
    b64[b64_len] = '\0';

    // Output: marker + chunked base64 + marker
    printf("SCREENSHOT_START\n");
    const int chunk = 72;
    for (size_t i = 0; i < b64_len; i += chunk) {
        int remain = (int)b64_len - (int)i;
        int len = (remain < chunk) ? remain : chunk;
        printf("%.*s\n", len, (char *)b64 + i);
    }
    printf("SCREENSHOT_END\n");
    fflush(stdout);

    free(b64);
    free(pbm);
}

// Serial command listener: waits for "SHOOT" on stdin
static void screenshot_cmd_task(void *arg)
{
    char line[64];
    for (;;) {
        if (fgets(line, sizeof(line), stdin) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strcmp(line, "SHOOT") == 0) {
                ESP_LOGI(TAG, "Screenshot requested");
                take_screenshot();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
 * Button-triggered screenshot (BOOT = GPIO0).
 * Polls GPIO0 every 50ms; press to capture screenshot.
 * Also keeps serial SHOOT command as fallback.
 */

static void screenshot_btn_task(void *arg)
{
    uint32_t last_press = 0;
    bool was_pressed = false;
    for (;;) {
        int level = gpio_get_level(GPIO_NUM_0);
        if (level == 0 && !was_pressed) {
            was_pressed = true;
            uint32_t now = xTaskGetTickCount();
            if (now - last_press >= pdMS_TO_TICKS(3000)) {
                last_press = now;
                ESP_LOGI(TAG, "BOOT pressed, taking screenshot");
                take_screenshot();
            }
        } else if (level == 1) {
            was_pressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3-RLCD WiFi STA ===");

    /* 1. Init ST7305 reflective LCD */
    g_display = new DisplayPort(12, 11, 5, 40, 41, 400, 300);
    g_display->RLCD_Init();

    /* 2. Init LVGL (full-refresh mode for 1-bit display) */
    Lvgl_PortInit(400, 300, lvgl_flush_cb);

    /* 3. Build WiFi status UI */
    if (Lvgl_lock(-1)) {
        lv_obj_t *scr = lv_scr_act();
        lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
        lv_obj_set_style_pad_all(scr, 0, 0);

        // Title - centered at top
        lbl_title = lv_label_create(scr);
        lv_label_set_text(lbl_title, "WiFi STA");
        lv_obj_set_style_text_color(lbl_title, lv_color_black(), 0);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

        // Separator line
        lv_obj_t *line1 = lv_line_create(scr);
        static lv_point_t line_pts1[] = {{0, 0}, {360, 0}};
        lv_line_set_points(line1, line_pts1, 2);
        lv_obj_set_style_line_color(line1, lv_color_black(), 0);
        lv_obj_set_style_line_width(line1, 2, 0);
        lv_obj_align(line1, LV_ALIGN_TOP_MID, 0, 38);

        // Status
        lbl_status = lv_label_create(scr);
        lv_label_set_text(lbl_status, "Connecting...");
        lv_obj_set_style_text_color(lbl_status, lv_color_black(), 0);
        lv_obj_align(lbl_status, LV_ALIGN_TOP_LEFT, 25, 48);

        // Clock - right-aligned on same row as status
        lbl_clock = lv_label_create(scr);
        lv_label_set_text(lbl_clock, "--:--:--");
        lv_obj_set_style_text_color(lbl_clock, lv_color_black(), 0);
        lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -25, 48);

        // SSID
        lbl_ssid = lv_label_create(scr);
        lv_label_set_text(lbl_ssid, "SSID:   rickqi11");
        lv_obj_set_style_text_color(lbl_ssid, lv_color_black(), 0);
        lv_obj_align(lbl_ssid, LV_ALIGN_TOP_LEFT, 25, 73);

        // IP
        lbl_ip = lv_label_create(scr);
        lv_label_set_text(lbl_ip, "IP:     --");
        lv_obj_set_style_text_color(lbl_ip, lv_color_black(), 0);
        lv_obj_align(lbl_ip, LV_ALIGN_TOP_LEFT, 25, 98);

        // RSSI
        lbl_rssi = lv_label_create(scr);
        lv_label_set_text(lbl_rssi, "RSSI:   --");
        lv_obj_set_style_text_color(lbl_rssi, lv_color_black(), 0);
        lv_obj_align(lbl_rssi, LV_ALIGN_TOP_LEFT, 25, 123);

        // MAC (placeholder, will be filled after WiFi init)
        lbl_mac = lv_label_create(scr);
        lv_label_set_text(lbl_mac, "MAC:    --");
        lv_obj_set_style_text_color(lbl_mac, lv_color_black(), 0);
        lv_obj_align(lbl_mac, LV_ALIGN_TOP_LEFT, 25, 148);

        // Uptime
        lbl_uptime = lv_label_create(scr);
        lv_label_set_text(lbl_uptime, "Uptime: 0 s");
        lv_obj_set_style_text_color(lbl_uptime, lv_color_black(), 0);
        lv_obj_align(lbl_uptime, LV_ALIGN_TOP_LEFT, 25, 173);

        // Separator line before summary
        lv_obj_t *line2 = lv_line_create(scr);
        static lv_point_t line_pts2[] = {{0, 0}, {360, 0}};
        lv_line_set_points(line2, line_pts2, 2);
        lv_obj_set_style_line_color(line2, lv_color_black(), 0);
        lv_obj_set_style_line_width(line2, 1, 0);
        lv_obj_align(line2, LV_ALIGN_TOP_MID, 0, 198);

        // Summary - brief description of this firmware modification
        lv_obj_t *lbl_sum1 = lv_label_create(scr);
        lv_label_set_text(lbl_sum1, "WIFI_STA + ST7305 RLCD + LVGL");
        lv_obj_set_style_text_color(lbl_sum1, lv_color_black(), 0);
        lv_obj_align(lbl_sum1, LV_ALIGN_TOP_MID, 0, 210);

        lv_obj_t *lbl_sum2 = lv_label_create(scr);
        lv_label_set_text(lbl_sum2, "WiFi status auto-refresh @ 2s");
        lv_obj_set_style_text_color(lbl_sum2, lv_color_black(), 0);
        lv_obj_align(lbl_sum2, LV_ALIGN_TOP_MID, 0, 235);

        Lvgl_unlock();
    }

    /* 4. Init WiFi (calls espwifi_Init from esp_wifi_bsp) */
    espwifi_Init();
    ESP_LOGI(TAG, "WiFi init done");

    /* 5. Show MAC address now that WiFi is up */
    {
        uint8_t mac[6];
        if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
            if (Lvgl_lock(-1)) {
                char buf[80];
                snprintf(buf, sizeof(buf), "MAC:    %02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                lv_label_set_text(lbl_mac, buf);
                Lvgl_unlock();
            }
        }
    }

    /* 6. Start background WiFi info updater */
    xTaskCreatePinnedToCore(wifi_info_task, "wifi_info", 4 * 1024, NULL, 3, NULL, 1);

    /* 7. Start serial screenshot command listener (fallback) */
    xTaskCreatePinnedToCore(screenshot_cmd_task, "scr_cmd", 6 * 1024, NULL, 2, NULL, 1);

    /* 8. Configure BOOT button (GPIO0) for screenshot trigger */
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask  = BIT64(0);
    io_conf.mode          = GPIO_MODE_INPUT;
    io_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(screenshot_btn_task, "scr_btn", 6 * 1024, NULL, 4, NULL, 1);
    ESP_LOGI(TAG, "Press BOOT (GPIO0) to capture screenshot");
}
