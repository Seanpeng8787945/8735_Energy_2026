/*
  19_YOLO_person
  HUB-8735 Ultra：YOLO 人員辨識 + HD H.264/AAC RTSP

  YOLO 模型：內建 DEFAULT_YOLOV4TINY，只標示 person
  RTSP：HD 1280x720、H.264、AAC 麥克風聲音
  OLED：SSD1306 128x64、SDA -> IO3、SCL -> IO4
*/

#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"

#define VIDEO_CHANNEL 0
#define NN_CHANNEL 3
#define NN_WIDTH 576
#define NN_HEIGHT 320

char WIFI_SSID[] = "Seanpeng8787";
const char WIFI_PASSWORD[] = "0968709788";

VideoSetting videoConfig(VIDEO_HD, CAM_FPS, VIDEO_H264, 0);
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, 10, VIDEO_RGB, 0);
AudioSetting audioConfig(3);

Audio audio;
AAC aac;
RTSP rtsp;
NNObjectDetection objectDetection;
StreamIO audioStreamer(1, 1);
StreamIO avMixStreamer(2, 1);
StreamIO videoStreamerNN(1, 1);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void drawStatus(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "HUB-8735 YOLO");
  u8g2.drawStr(0, 30, line1);
  if (line2 != nullptr) u8g2.drawStr(0, 45, line2);
  if (line3 != nullptr) u8g2.drawStr(0, 60, line3);
  u8g2.sendBuffer();
}

void formatIpAddress(IPAddress ip, char* output, size_t outputSize) {
  snprintf(output, outputSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void connectWiFi() {
  int status = WL_IDLE_STATUS;
  drawStatus("WiFi connecting...");
  while (status != WL_CONNECTED) {
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long startMs = millis();
    while (status != WL_CONNECTED && millis() - startMs < 15000UL) {
      delay(250);
      status = WiFi.status();
    }
    if (status != WL_CONNECTED) {
      drawStatus("WiFi retrying...");
      delay(1000);
    }
  }

  char ipString[20];
  formatIpAddress(WiFi.localIP(), ipString, sizeof(ipString));
  drawStatus("WiFi connected", "IP address:", ipString);
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  drawStatus("Starting...");
  connectWiFi();

  videoConfig.setBitrate(2 * 1024 * 1024);
  Camera.configVideoChannel(VIDEO_CHANNEL, videoConfig);
  Camera.configVideoChannel(NN_CHANNEL, nnConfig);
  Camera.videoInit();

  audio.configAudio(audioConfig);
  audio.begin();
  aac.configAudio(audioConfig);
  aac.begin();

  rtsp.configVideo(videoConfig);
  rtsp.configAudio(audioConfig, CODEC_AAC);
  rtsp.begin();

  audioStreamer.registerInput(audio);
  audioStreamer.registerOutput(aac);
  if (audioStreamer.begin() != 0) {
    drawStatus("Audio stream error");
    return;
  }

  avMixStreamer.registerInput1(Camera.getStream(VIDEO_CHANNEL));
  avMixStreamer.registerInput2(aac);
  avMixStreamer.registerOutput(rtsp);
  if (avMixStreamer.begin() != 0) {
    drawStatus("AV stream error");
    return;
  }

  objectDetection.configVideo(nnConfig);
  objectDetection.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV4TINY, NA_MODEL, NA_MODEL);
  objectDetection.begin();

  videoStreamerNN.registerInput(Camera.getStream(NN_CHANNEL));
  videoStreamerNN.setStackSize();
  videoStreamerNN.setTaskPriority();
  videoStreamerNN.registerOutput(objectDetection);
  if (videoStreamerNN.begin() != 0) {
    drawStatus("YOLO stream error");
    return;
  }

  Camera.channelBegin(VIDEO_CHANNEL);
  Camera.channelBegin(NN_CHANNEL);
  OSD.configVideo(VIDEO_CHANNEL, videoConfig);
  OSD.begin();

  char ipString[20];
  formatIpAddress(WiFi.localIP(), ipString, sizeof(ipString));
  char rtspString[24];
  snprintf(rtspString, sizeof(rtspString), "RTSP:%d", rtsp.getPort());
  drawStatus("YOLO READY", ipString, rtspString);
}

void loop() {
  std::vector<ObjectDetectionResult> results = objectDetection.getResult();
  uint16_t imageWidth = videoConfig.width();
  uint16_t imageHeight = videoConfig.height();

  OSD.createBitmap(VIDEO_CHANNEL);
  int personCount = 0;
  for (int i = 0; i < objectDetection.getResultCount(); i++) {
    ObjectDetectionResult item = results[i];
    int objectType = item.type();
    // COCO 類別索引 0 為 person；其他物件不標示、不計數。
    if (objectType != 0) continue;
    personCount++;

    int xmin = (int)(item.xMin() * imageWidth);
    int xmax = (int)(item.xMax() * imageWidth);
    int ymin = (int)(item.yMin() * imageHeight);
    int ymax = (int)(item.yMax() * imageHeight);
    OSD.drawRect(VIDEO_CHANNEL, xmin, ymin, xmax, ymax, 5, OSD_COLOR_RED);

    char text[32];
    snprintf(text, sizeof(text), "%s %d", itemList[objectType].objectName, item.score());
    OSD.drawText(VIDEO_CHANNEL, xmin, ymin - OSD.getTextHeight(VIDEO_CHANNEL), text, OSD_COLOR_CYAN);
  }
  char personText[20];
  snprintf(personText, sizeof(personText), "Person=%d", personCount);
  OSD.drawText(VIDEO_CHANNEL, 8, 24, personText, OSD_COLOR_RED);
  OSD.update(VIDEO_CHANNEL);
  delay(50);
}
