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
#define USE_SOFTAP_MODE 1
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
    // We use <h1> for huge titles and <button> with large padding
    // style='font-size: 40px' makes it very readable on an old phone
    const char* html =
        "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<style>"
        "body { font-family: sans-serif; text-align: center; background: #f0f0f0; }"
        "h1 { font-size: 50px; color: #333; }"
        "h2 { font-size: 40px; color: blue; }"
        ".btn { width: 90%; padding: 40px; margin: 20px; font-size: 35px; border-radius: 15px; border: none; cursor: pointer; }"
        ".fwd { background-color: #4CAF50; color: white; }"
        ".rev { background-color: #2196F3; color: white; }"
        ".stop { background-color: #f44336; color: white; }"
        "</style></head><body>"
        "<h1>H-Bridge Control</h1>"
        "<h1>Status:</h1>"
        "<h2>%s</h2>" // This %s will be replaced by our 'last_direction' string
        "<button class='btn fwd' onclick=\"location.href='/fwd'\">FORWARD</button>"
        "<button class='btn rev' onclick=\"location.href='/rev'\">REVERSE</button>"
        "<button class='btn stop' onclick=\"location.href='/stop'\">STOP NOW</button>"
        "</body></html>";

    char formatted_html[2000];
    sprintf(formatted_html, html, last_direction); // Put the status into the HTML
    return httpd_resp_send(req, formatted_html, HTTPD_RESP_USE_STRLEN);
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
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = { .ssid = AP_SSID, .password = AP_PASS, .max_connection = 4, .authmode = WIFI_AUTH_WPA2_PSK }
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    // 6. Start the Server
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_root);
        httpd_register_uri_handler(server, &uri_fwd);
        httpd_register_uri_handler(server, &uri_rev);
        httpd_register_uri_handler(server, &uri_stop);
    }
}