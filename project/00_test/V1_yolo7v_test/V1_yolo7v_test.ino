/*
  V1_yolo7v_test
  HUB-8735 Ultra：SD 卡客製 YOLOv7-tiny 五類物件辨識測試

  SD 卡必要檔案：
    NN_MDL/yolov7_tiny.nb

  類別順序必須與訓練模型一致：
    0 glass
    1 household_waste
    2 metal
    3 paper_cardboard
    4 plastic

  OLED：SSD1306 128x64，I2C 0x3C，SDA IO3，SCL IO4
*/

#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"

#define VIDEO_CHANNEL 0
#define NN_CHANNEL 3
#define NN_WIDTH 576
#define NN_HEIGHT 320

static const char MODEL_PATH[] = "sd:/NN_MDL/yolov7_tiny.nb";

char WIFI_SSID[] = "Seanpeng8787";
const char WIFI_PASSWORD[] = "0968709788";

VideoSetting videoConfig(VIDEO_HD, CAM_FPS, VIDEO_H264, 0);
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, 10, VIDEO_RGB, 0);

NNObjectDetection objectDetection;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerNN(1, 1);
RTSP rtsp;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const char *const CLASS_NAMES[] = {
  "glass", "household", "metal", "paper", "plastic"
};

void drawStatus(const char *line1, const char *line2 = nullptr, const char *line3 = nullptr)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "YOLOv7 SD TEST");
  u8g2.drawStr(0, 28, line1);
  if (line2) u8g2.drawStr(0, 44, line2);
  if (line3) u8g2.drawStr(0, 60, line3);
  u8g2.sendBuffer();
}

void formatIpAddress(IPAddress ip, char *output, size_t outputSize)
{
  snprintf(output, outputSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void connectWiFi()
{
  int status = WL_IDLE_STATUS;
  drawStatus("WiFi connecting...");
  Serial.print("Connecting WiFi: ");
  Serial.println(WIFI_SSID);

  while (status != WL_CONNECTED) {
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long started = millis();
    while (status != WL_CONNECTED && millis() - started < 15000UL) {
      delay(250);
      status = WiFi.status();
    }
    if (status != WL_CONNECTED) {
      Serial.println("WiFi retry");
      drawStatus("WiFi retrying...");
      delay(1000);
    }
  }

  char ipString[20];
  formatIpAddress(WiFi.localIP(), ipString, sizeof(ipString));
  Serial.print("WiFi IP: ");
  Serial.println(ipString);
  drawStatus("WiFi connected", "IP address:", ipString);
  delay(1500);
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  drawStatus("Starting...");
  Serial.println("\r\n=== V1_yolo7v_test ===");

  Serial.print("SDK SD model path: ");
  Serial.println(MODEL_PATH);
  drawStatus("SD model selected", "yolov7_tiny.nb");
  delay(1000);

  connectWiFi();

  videoConfig.setBitrate(2 * 1024 * 1024);
  Camera.configVideoChannel(VIDEO_CHANNEL, videoConfig);
  Camera.configVideoChannel(NN_CHANNEL, nnConfig);
  Camera.videoInit();

  rtsp.configVideo(videoConfig);
  rtsp.begin();

  objectDetection.configVideo(nnConfig);
  // CUSTOMIZED_YOLOV7TINY reads sd:/NN_MDL/yolov7_tiny.nb.
  objectDetection.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV7TINY, NA_MODEL, NA_MODEL);
  objectDetection.begin();
  Serial.println("Custom YOLOv7 model initialization requested");

  videoStreamer.registerInput(Camera.getStream(VIDEO_CHANNEL));
  videoStreamer.registerOutput(rtsp);
  if (videoStreamer.begin() != 0) {
    Serial.println("[ERROR] RTSP stream link failed");
    drawStatus("RTSP link FAILED");
    return;
  }

  videoStreamerNN.registerInput(Camera.getStream(NN_CHANNEL));
  videoStreamerNN.setStackSize();
  videoStreamerNN.setTaskPriority();
  videoStreamerNN.registerOutput(objectDetection);
  if (videoStreamerNN.begin() != 0) {
    Serial.println("[ERROR] NN stream link failed");
    drawStatus("NN link FAILED");
    return;
  }

  Camera.channelBegin(VIDEO_CHANNEL);
  Camera.channelBegin(NN_CHANNEL);
  OSD.configVideo(VIDEO_CHANNEL, videoConfig);
  OSD.begin();

  char ipString[20];
  formatIpAddress(WiFi.localIP(), ipString, sizeof(ipString));
  drawStatus("YOLOv7 READY", ipString, "SD model active");
  Serial.println("=== YOLOv7 READY ===");
}

void loop()
{
  std::vector<ObjectDetectionResult> results = objectDetection.getResult();
  uint16_t imageWidth = videoConfig.width();
  uint16_t imageHeight = videoConfig.height();
  int detectedCount = 0;

  OSD.createBitmap(VIDEO_CHANNEL);
  for (int i = 0; i < objectDetection.getResultCount(); i++) {
    ObjectDetectionResult item = results[i];
    int objectType = item.type();
    if (objectType < 0 || objectType >= 5) continue;
    detectedCount++;

    int xmin = (int)(item.xMin() * imageWidth);
    int xmax = (int)(item.xMax() * imageWidth);
    int ymin = (int)(item.yMin() * imageHeight);
    int ymax = (int)(item.yMax() * imageHeight);
    OSD.drawRect(VIDEO_CHANNEL, xmin, ymin, xmax, ymax, 4, OSD_COLOR_GREEN);

    char label[48];
    snprintf(label, sizeof(label), "%s %d", CLASS_NAMES[objectType], item.score());
    OSD.drawText(VIDEO_CHANNEL, xmin, ymin - OSD.getTextHeight(VIDEO_CHANNEL), label, OSD_COLOR_CYAN);

    Serial.print("det: ");
    Serial.print(CLASS_NAMES[objectType]);
    Serial.print(" score=");
    Serial.println(item.score());
  }

  char countText[32];
  snprintf(countText, sizeof(countText), "objects=%d", detectedCount);
  OSD.drawText(VIDEO_CHANNEL, 8, 24, countText, OSD_COLOR_RED);
  OSD.update(VIDEO_CHANNEL);
  delay(50);
}
