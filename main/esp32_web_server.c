#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "esp_timer.h" // Needed for the 5-second automatic shutoff

#include "secrets.h"

// ==========================================
// CONFIGURATION & PINS
// ==========================================
// Mode selection:
// 1 = STA ONLY
// 2 = STA with AP FALLBACK
// 3 = BOTH (Dual Mode)
#define WIFI_OPERATION_MODE 2

#define RELAY_PIN_A     18
#define RELAY_PIN_B     19
#define BLUE_LED_PIN    2  // The built-in LED on most DevKit boards

static const char *TAG = "H_BRIDGE_PROJECT";

// Global variables to keep track of what the ESP32 is doing
static char last_direction[20] = "Stopped"; // Stores "Forward", "Reverse", or "Stopped"
static int led_blink_speed_ms = 0;           // 0 = Off, 500 = Slow, 100 = Fast
static esp_timer_handle_t motor_timer;       // The "alarm clock" for the 5s shutoff

// ==========================================
// MOTOR & LED LOGIC
// ==========================================

// This function turns everything off
static void stop_everything() {
    gpio_set_level(RELAY_PIN_A, 0);
    gpio_set_level(RELAY_PIN_B, 0);
    led_blink_speed_ms = 0; // Stop the LED blinking
    gpio_set_level(BLUE_LED_PIN, 0); // Ensure LED is off
    ESP_LOGI(TAG, "Automatic Shutoff: Motor Stopped");
}

// This is the "Alarm Clock" callback. It runs when 5 seconds are up.
static void motor_timer_callback(void* arg) {
    stop_everything();
    strcpy(last_direction, "Timed Out (Stop)");
}

// Function to reset/start the 5-second timer
static void start_motor_timeout() {
    esp_timer_stop(motor_timer); // Stop any existing timer first
    // Start a one-time timer for 5,000,000 microseconds (5 seconds)
    esp_timer_start_once(motor_timer, 5000000);
}

// A "Task" that runs in the background to blink the LED
void led_blink_task(void *pvParameter) {
    while(1) {
        if (led_blink_speed_ms > 0) {
            gpio_set_level(BLUE_LED_PIN, 1); // LED ON
            vTaskDelay(pdMS_TO_TICKS(led_blink_speed_ms));
            gpio_set_level(BLUE_LED_PIN, 0); // LED OFF
            vTaskDelay(pdMS_TO_TICKS(led_blink_speed_ms));
        } else {
            gpio_set_level(BLUE_LED_PIN, 0); // Stay off if speed is 0
            vTaskDelay(pdMS_TO_TICKS(100));  // Wait a bit before checking again
        }
    }
}

// ==========================================
// WEB SERVER HANDLERS
// ==========================================

static esp_err_t send_big_html_page(httpd_req_t *req) {
    // 1. Get connection mode
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    const char* mode_str = (mode == WIFI_MODE_STA) ? "Home Network (Eero)" : "Direct Hotspot";

    // 2. Send Header & CSS in a chunk
    httpd_resp_sendstr_chunk(req, "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    httpd_resp_sendstr_chunk(req, "<style>body{font-family:sans-serif;text-align:center;background:#f4f4f4;margin:0;padding:10px;}"
                                  ".status-bar{background:#333;color:#fff;padding:8px;font-size:14px;border-radius:5px;margin-bottom:15px;}"
                                  "h1{font-size:28px;margin:10px 0;}"
                                  ".m-status{font-size:24px;color:blue;font-weight:bold;margin:15px 0;}"
                                  ".btn{width:85%;padding:20px;margin:10px;font-size:22px;font-weight:bold;border-radius:50px;border:none;color:white;cursor:pointer;}"
                                  ".fwd{background:#4CAF50;}.rev{background:#2196F3;}.stop{background:#f44336;width:60%;padding:15px;font-size:18px;}</style></head><body>");

    // 3. Send the Status Bar
    char status_html[256];
    snprintf(status_html, sizeof(status_html), "<div class='status-bar'>Connected via: %s</div><h1>H-Bridge Control</h1>", mode_str);
    httpd_resp_sendstr_chunk(req, status_html);

    // 4. Send the Motor Status and Buttons
    char motor_html[512];
    snprintf(motor_html, sizeof(motor_html), "<div>Motor Status:</div><div class='m-status'>%s</div>"
                                             "<button class='btn fwd' onclick=\"location.href='/fwd'\">FORWARD</button>"
                                             "<button class='btn rev' onclick=\"location.href='/rev'\">REVERSE</button>"
                                             "<button class='btn stop' onclick=\"location.href='/stop'\">STOP NOW</button>"
                                             "</body></html>", last_direction);
    httpd_resp_sendstr_chunk(req, motor_html);

    // 5. Tell the server we are done
    return httpd_resp_sendstr_chunk(req, NULL);
}

static esp_err_t on_forward(httpd_req_t *req) {
    gpio_set_level(RELAY_PIN_A, 1);
    gpio_set_level(RELAY_PIN_B, 0);
    led_blink_speed_ms = 600; // Slow blink for forward
    strcpy(last_direction, "Forward");
    start_motor_timeout();
    return send_big_html_page(req);
}

static esp_err_t on_reverse(httpd_req_t *req) {
    gpio_set_level(RELAY_PIN_A, 0);
    gpio_set_level(RELAY_PIN_B, 1);
    led_blink_speed_ms = 150; // Fast blink for reverse
    strcpy(last_direction, "Reverse");
    start_motor_timeout();
    return send_big_html_page(req);
}

static esp_err_t on_stop(httpd_req_t *req) {
    stop_everything();
    esp_timer_stop(motor_timer); // Kill the 30s timer since we stopped manually
    strcpy(last_direction, "Stopped Manually");
    return send_big_html_page(req);
}

// URI mappings (links the buttons to the code)
static const httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = send_big_html_page };
static const httpd_uri_t uri_fwd  = { .uri = "/fwd", .method = HTTP_GET, .handler = on_forward };
static const httpd_uri_t uri_rev  = { .uri = "/rev", .method = HTTP_GET, .handler = on_reverse };
static const httpd_uri_t uri_stop = { .uri = "/stop", .method = HTTP_GET, .handler = on_stop };

// ==========================================
// INITIALIZATION
// ==========================================

// This handles Wi-Fi events (connecting, disconnecting, getting IP)
static int s_retry_num = 0;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) { // Try 5 times
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying connection to Eero...");
        } else {
            ESP_LOGI(TAG, "Failed to connect to Eero.");
#if WIFI_OPERATION_MODE == 2
            ESP_LOGI(TAG, "Switching to SoftAP Fallback...");
            esp_wifi_set_mode(WIFI_MODE_AP);
#endif
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

void app_main(void) {
    // 1. Initialize Memory
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) { nvs_flash_erase(); nvs_flash_init(); }

    // 2. Setup GPIOs
    gpio_reset_pin(RELAY_PIN_A);
    gpio_reset_pin(RELAY_PIN_B);
    gpio_reset_pin(BLUE_LED_PIN);
    gpio_set_direction(RELAY_PIN_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(RELAY_PIN_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT);

    // 3. Create the 5-second timer "object"
    const esp_timer_create_args_t timer_args = {
        .callback = &motor_timer_callback,
        .name = "motor_timeout"
    };
    esp_timer_create(&timer_args, &motor_timer);

    // 4. Start the Background Blink Task
    // This allows the LED to blink without pausing the rest of the program
    xTaskCreate(&led_blink_task, "blink_task", 2048, NULL, 5, NULL);

    // 5. Start WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create both interfaces (STA and AP)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // SET HOSTNAME: This is what Eero will see
    esp_netif_set_hostname(sta_netif, DEVICE_HOSTNAME);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register the event handler we pasted in Step 1
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Pull credentials from secrets.h
    wifi_config_t sta_config = { .sta = { .ssid = EXAMPLE_ESP_WIFI_SSID, .password = EXAMPLE_ESP_WIFI_PASS } };
    wifi_config_t ap_config  = { .ap  = { .ssid = AP_SSID, .password = AP_PASS, .max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK } };

    // Apply the logic for your 3 modes
    #if WIFI_OPERATION_MODE == 1
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    #elif WIFI_OPERATION_MODE == 2
        esp_wifi_set_mode(WIFI_MODE_STA); // Start with STA, handler will switch to AP if fail
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    #elif WIFI_OPERATION_MODE == 3
        esp_wifi_set_mode(WIFI_MODE_APSTA);
        esp_wifi_set_config(WIFI_IF_STA, &sta_config);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    #endif

    ESP_ERROR_CHECK(esp_wifi_start());

    // 6. Start the Server
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // This allows the server to listen on both STA and AP interfaces
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_fwd);
        httpd_register_uri_handler(server, &uri_rev);
        httpd_register_uri_handler(server, &uri_stop);
        ESP_LOGI(TAG, "Web server started successfully!");
    }
}
