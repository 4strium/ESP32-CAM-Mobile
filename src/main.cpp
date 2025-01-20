#include <Arduino.h>
#include <esp_camera.h>
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include "FS.h"
#include "SD_MMC.h"

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define FLASHLED_GPIO_NUM 4

#define PHOTO_MODE 1

#define PART_BOUNDARY "123456789000000000000987654321"

const char *ssid = "ESP32-CAM";
const char *password = "12345678";

httpd_handle_t stream_httpd = NULL;

// Initialize the camera
camera_config_t config;

// Variable to track LED state
bool led_on = false;

void startCameraServer();

// Handler to toggle the LED state
static esp_err_t led_handler(httpd_req_t *req)
{
  led_on = !led_on; // Toggle LED state
  digitalWrite(FLASHLED_GPIO_NUM, led_on ? HIGH : LOW);
  const char *response = led_on ? "LED is ON" : "LED is OFF";
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

// Handler to capture and save a photo to the SD card
static esp_err_t capture_handler(httpd_req_t *req)
{
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  // Generate a filename for the photo
  char path[32];
  snprintf(path, sizeof(path), "/photo_%u.jpg", (unsigned int)esp_timer_get_time());

  // Open the file on the SD card
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file)
  {
    esp_camera_fb_return(fb);
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  // Write the photo data to the file
  file.write(fb->buf, fb->len);
  file.close();

  esp_camera_fb_return(fb);

  // Send a response back to the client
  char response[64];
  snprintf(response, sizeof(response), "Photo saved to %s", path);
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}
static esp_err_t quality_handler(httpd_req_t *req)
{
  char buf[100];
  size_t buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1)
  {
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      char param[32];
      if (httpd_query_key_value(buf, "val", param, sizeof(param)) == ESP_OK)
      {

        ESP_LOGI(TAG, "Found URL query parameter => val=%s", param);
        if (strcmp(param, "SVGA") == 0)
        {
          config.frame_size = FRAMESIZE_SVGA;
        }
        else if (strcmp(param, "XGA") == 0)
        {
          config.frame_size = FRAMESIZE_XGA;
        }
        else if (strcmp(param, "SXGA") == 0)
        {
          config.frame_size = FRAMESIZE_SXGA;
        }
        else if (strcmp(param, "UXGA") == 0)
        {
          config.frame_size = FRAMESIZE_UXGA;
        }
        else
        {
          config.frame_size = FRAMESIZE_VGA;
        }

        esp_camera_deinit();

        if (esp_camera_init(&config) != ESP_OK)
        {

          return ESP_FAIL;
        }

        startCameraServer();
      }
    }
  }
  // Réponse HTTP
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "Quality set", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len;
  uint8_t *_jpg_buf;
  char *part_buf[64];

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=" PART_BOUNDARY);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->format != PIXFORMAT_JPEG)
      {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if (!jpeg_converted)
        {
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      }
      else
      {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }

    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
      if (res == ESP_OK)
      {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
      }
      if (res == ESP_OK)
      {
        res = httpd_resp_send_chunk(req, "\r\n--" PART_BOUNDARY "\r\n", strlen("\r\n--" PART_BOUNDARY "\r\n"));
      }
    }

    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if (res != ESP_OK)
    {
      break;
    }
  }

  return res;
}

// Start the camera server
void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t stream_uri = {
      .uri = "/stream",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  httpd_uri_t capture_uri = {
      .uri = "/capture",
      .method = HTTP_GET,
      .handler = capture_handler,
      .user_ctx = NULL};
  httpd_uri_t led_uri = {
      .uri = "/led",
      .method = HTTP_GET,
      .handler = led_handler,
      .user_ctx = NULL};
  httpd_uri_t framesize_uri = {
      .uri = "/quality",
      .method = HTTP_GET,
      .handler = quality_handler,
      .user_ctx = NULL};

  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &framesize_uri);
    if (PHOTO_MODE)
    {
      httpd_register_uri_handler(stream_httpd, &capture_uri);
    }
    else
    {
      httpd_register_uri_handler(stream_httpd, &led_uri);
    }
  }
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  pinMode(FLASHLED_GPIO_NUM, OUTPUT);

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  // Camera initialization
  if (esp_camera_init(&config) != ESP_OK)
  {
    Serial.println("Camera init failed");
    return;
  }

  if (PHOTO_MODE)
  {
    // Initialize SD card
    if (!SD_MMC.begin())
    {
      Serial.println("SD Card Mount Failed");
      return;
    }
  }

  // Connect to Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start the camera server
  startCameraServer();
}

void loop()
{
  delay(1);
}
