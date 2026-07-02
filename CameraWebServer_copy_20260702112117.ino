/****************************************************
  SMART BILLING CART – ESP32-CAM (AI Thinker)
  Edge Impulse Object Detection + Web Server JSON
*****************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "esp_heap_caps.h"

#include <Smart_Billing_Cart_Object_Detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

// ========== WiFi ==========
const char* ssid     = "Unknown09";
const char* password = "1234567890";

WebServer server(80);

// ======================================================
// CORS ENABLE
// ======================================================
void enableCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// ======================================================
// AI THINKER ESP32-CAM PINS
// ======================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ======================================================
// CAMERA CONFIG
// ======================================================
camera_config_t camera_config = {
  .pin_pwdn     = PWDN_GPIO_NUM,
  .pin_reset    = RESET_GPIO_NUM,
  .pin_xclk     = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7       = Y9_GPIO_NUM,
  .pin_d6       = Y8_GPIO_NUM,
  .pin_d5       = Y7_GPIO_NUM,
  .pin_d4       = Y6_GPIO_NUM,
  .pin_d3       = Y5_GPIO_NUM,
  .pin_d2       = Y4_GPIO_NUM,
  .pin_d1       = Y3_GPIO_NUM,
  .pin_d0       = Y2_GPIO_NUM,
  .pin_vsync    = VSYNC_GPIO_NUM,
  .pin_href     = HREF_GPIO_NUM,
  .pin_pclk     = PCLK_GPIO_NUM,

  .xclk_freq_hz = 20000000,
  .ledc_timer   = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_JPEG,
  .frame_size   = FRAMESIZE_QVGA, // 320×240
  .jpeg_quality = 12,
  .fb_count     = 1,
  .fb_location  = CAMERA_FB_IN_PSRAM,
  .grab_mode    = CAMERA_GRAB_WHEN_EMPTY
};

// ======================================================
// Buffers
// ======================================================
static uint8_t *rgb888_buf = nullptr;
static uint8_t *resized_buf = nullptr;
static uint8_t *ei_rgb       = nullptr;

String lastJSON = "{}";

// ======================================================
// Edge Impulse get_data callback
// ======================================================
int ei_camera_get_data(size_t offset, size_t length, float *out_ptr){
    size_t px = offset * 3;
    for(size_t i = 0; i < length; i++){
        uint32_t r = ei_rgb[px + 0];
        uint32_t g = ei_rgb[px + 1];
        uint32_t b = ei_rgb[px + 2];
        out_ptr[i] = (r << 16) | (g << 8) | b;
        px += 3;
    }
    return 0;
}

// ======================================================
// Inference
// ======================================================
void run_inference(){
    camera_fb_t *fb = esp_camera_fb_get();
    if(!fb){
        Serial.println("Frame failed!");
        return;
    }

    bool ok = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb888_buf);
    esp_camera_fb_return(fb);
    if(!ok){
        Serial.println("JPEG decode failed");
        return;
    }

    int SRC_W = 320, SRC_H = 240;
    int DST_W = EI_CLASSIFIER_INPUT_WIDTH;
    int DST_H = EI_CLASSIFIER_INPUT_HEIGHT;

    ei::image::processing::resize_image(
        rgb888_buf, SRC_W, SRC_H,
        resized_buf, DST_W, DST_H,
        3);

    ei_rgb = resized_buf;

    ei::signal_t signal;
    signal.total_length = DST_W * DST_H;
    signal.get_data     = ei_camera_get_data;

    ei_impulse_result_t result;
    run_classifier(&signal, &result, false);

    String json = "{\"objects\":[";
    bool first = true;

    for(int i = 0; i < result.bounding_boxes_count; i++){
        auto bb = result.bounding_boxes[i];
        if(bb.value <= 0) continue;

        if(!first) json += ",";
        first = false;

        json += "{";
        json += "\"label\":\"" + String(bb.label) + "\",";
        json += "\"confidence\":" + String(bb.value, 3) + ",";
        json += "\"x\":" + String(bb.x) + ",";
        json += "\"y\":" + String(bb.y) + ",";
        json += "\"w\":" + String(bb.width) + ",";
        json += "\"h\":" + String(bb.height);
        json += "}";
    }
    json += "]}";

    lastJSON = json;

    Serial.println(lastJSON);
}

// ======================================================
// HTTP HANDLERS
// ======================================================
void handle_results(){
    enableCORS();
    server.send(200, "application/json", lastJSON);
}

void handle_options(){
    enableCORS();
    server.send(204);
}

// ======================================================
// WiFi
// ======================================================
void connectWiFi(){
    WiFi.begin(ssid, password);
    Serial.print("Connecting");
    while(WiFi.status() != WL_CONNECTED){
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nConnected!");
    Serial.println(WiFi.localIP());
}

// ======================================================
// SETUP
// ======================================================
void setup(){
    Serial.begin(115200);
    delay(500);

    rgb888_buf = (uint8_t*) heap_caps_malloc(320*240*3, MALLOC_CAP_SPIRAM);
    resized_buf = (uint8_t*) heap_caps_malloc(EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3, MALLOC_CAP_SPIRAM);

    esp_camera_init(&camera_config);

    connectWiFi();

    server.on("/results", HTTP_GET, handle_results);
    server.on("/results", HTTP_OPTIONS, handle_options);
    server.begin();

    Serial.println("Server ready: /results");
}

// ======================================================
// LOOP
// ======================================================
void loop(){
    run_inference();
    server.handleClient();
    delay(200);
}
