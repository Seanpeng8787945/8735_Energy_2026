/*
 * V1_yolo7v_test - HUB-8735 Ultra SD-card YOLOv7-tiny test
 * SD card file: NN_MDL/yolov7_tiny.nb
 * Input: 416x416 RGB888
 * Classes: 0 trash (general), 1 paper, 2 plastic, 3 metal
 */

#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <AmebaServo.h>
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"

static constexpr int VIDEO_CHANNEL = 0;
static constexpr int NN_CHANNEL = 3;
static constexpr int NN_WIDTH = 416;
static constexpr int NN_HEIGHT = 416;
static constexpr int CLASS_COUNT = 4;
static constexpr char MODEL_PATH[] = "sd:/NN_MDL/yolov7_tiny.nb";

char WIFI_SSID[] = "Seanpeng8787";
const char WIFI_PASSWORD[] = "0968709788";
const char *const CLASS_NAMES[CLASS_COUNT] = {"trash", "paper", "plastic", "metal"};

VideoSetting videoConfig(VIDEO_HD, CAM_FPS, VIDEO_H264, 0);
// The SD model requires 416x416 RGB input; do not change to VIDEO_HD.
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, 10, VIDEO_RGB, 0);
NNObjectDetection objectDetection;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerNN(1, 1);
RTSP rtsp;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// SG90 signal: HUB-8735 Ultra IO21 / PF8.
static constexpr int SERVO_PIN = 21;
AmebaServo sg90;
static bool paperTiming = false;
static bool servoCycleDone = false;
static unsigned long paperSince = 0;

void oledStatus(const char *a, const char *b = nullptr, const char *c = nullptr)
{
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 10, "YOLOv7 SD TEST");
  if (a) oled.drawStr(0, 28, a);
  if (b) oled.drawStr(0, 44, b);
  if (c) oled.drawStr(0, 60, c);
  oled.sendBuffer();
}

void ipText(char *out, size_t outSize)
{
  IPAddress ip = WiFi.localIP();
  snprintf(out, outSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void connectWiFi()
{
  oledStatus("WiFi connecting...", WIFI_SSID);
  Serial.print("WiFi connecting: ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) delay(250);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi retrying...");
      oledStatus("WiFi retrying...", WIFI_SSID);
      delay(1000);
    }
  }
  char ip[20];
  ipText(ip, sizeof(ip));
  Serial.print("WiFi connected, IP: ");
  Serial.println(ip);
  oledStatus("WiFi connected", "IP address:", ip);
  delay(1500);
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  oled.setI2CAddress(0x3C << 1);
  oled.begin();
  oledStatus("Starting...", "NN input: 416x416");
  Serial.println("\r\n=== V1_yolo7v_test ===");
  Serial.println("NN input: 416x416 RGB888");
  Serial.print("SD model: ");
  Serial.println(MODEL_PATH);

  Serial.println("SG90 signal: IO21");
  if (sg90.attach(SERVO_PIN)) {
    sg90.write(0);
    Serial.println("SG90 attached at 0 degrees");
  } else {
    Serial.println("[ERROR] SG90 attach failed");
  }

  connectWiFi();
  // RTSP/VLC stream is explicitly landscape: 1280x720, rotation 0 degrees.
  videoConfig.setRotation(0);
  videoConfig.setBitrate(2 * 1024 * 1024);
  Camera.configVideoChannel(VIDEO_CHANNEL, videoConfig);
  Camera.configVideoChannel(NN_CHANNEL, nnConfig);
  Camera.videoInit();
  rtsp.configVideo(videoConfig);
  rtsp.begin();
  objectDetection.configVideo(nnConfig);
  objectDetection.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV7TINY, NA_MODEL, NA_MODEL);
  objectDetection.begin();

  videoStreamer.registerInput(Camera.getStream(VIDEO_CHANNEL));
  videoStreamer.registerOutput(rtsp);
  if (videoStreamer.begin() != 0) {
    Serial.println("[ERROR] RTSP stream link failed");
    oledStatus("RTSP link FAILED");
    return;
  }
  videoStreamerNN.registerInput(Camera.getStream(NN_CHANNEL));
  videoStreamerNN.setStackSize();
  videoStreamerNN.setTaskPriority();
  videoStreamerNN.registerOutput(objectDetection);
  if (videoStreamerNN.begin() != 0) {
    Serial.println("[ERROR] NN stream link failed");
    oledStatus("NN link FAILED");
    return;
  }
  Camera.channelBegin(VIDEO_CHANNEL);
  Camera.channelBegin(NN_CHANNEL);
  OSD.configVideo(VIDEO_CHANNEL, videoConfig);
  OSD.begin();
  char ip[20];
  ipText(ip, sizeof(ip));
  oledStatus("YOLOv7 READY", "NN 416x416", ip);
  Serial.println("YOLOv7 SD model ready");
}

void loop()
{
  std::vector<ObjectDetectionResult> results = objectDetection.getResult();
  const int resultCount = objectDetection.getResultCount();
  const int imageWidth = videoConfig.width();
  const int imageHeight = videoConfig.height();
  int shown = 0;
  bool paperDetected = false;
  OSD.createBitmap(VIDEO_CHANNEL);
  for (int i = 0; i < resultCount; ++i) {
    ObjectDetectionResult item = results[i];
    const int type = item.type();
    if (type < 0 || type >= CLASS_COUNT) continue;
    if (type == 1) paperDetected = true;
    ++shown;
    const int xmin = (int)(item.xMin() * imageWidth);
    const int xmax = (int)(item.xMax() * imageWidth);
    const int ymin = (int)(item.yMin() * imageHeight);
    const int ymax = (int)(item.yMax() * imageHeight);
    OSD.drawRect(VIDEO_CHANNEL, xmin, ymin, xmax, ymax, 4, OSD_COLOR_GREEN);
    char label[48];
    snprintf(label, sizeof(label), "%s %d", CLASS_NAMES[type], item.score());
    OSD.drawText(VIDEO_CHANNEL, xmin, ymin - OSD.getTextHeight(VIDEO_CHANNEL), label, OSD_COLOR_CYAN);
    Serial.print("detected: ");
    Serial.print(CLASS_NAMES[type]);
    Serial.print(" score=");
    Serial.println(item.score());
  }
  char count[24];
  // ':' is supported by the small OSD font; '=' is not.
  snprintf(count, sizeof(count), "objects:%d", shown);
  OSD.drawText(VIDEO_CHANNEL, 8, 24, count, OSD_COLOR_RED);
  OSD.update(VIDEO_CHANNEL);

  // Paper must remain detected continuously for 3 seconds before triggering.
  // One servo cycle is allowed per paper presentation; remove the paper to arm again.
  const unsigned long now = millis();
  if (!paperDetected) {
    paperTiming = false;
    servoCycleDone = false;
    paperSince = 0;
  } else if (!servoCycleDone) {
    if (!paperTiming) {
      paperTiming = true;
      paperSince = now;
      Serial.println("paper timer started");
    } else if (now - paperSince >= 3000UL) {
      servoCycleDone = true;
      paperTiming = false;
      Serial.println("paper held 3 seconds; SG90 action");
      sg90.write(0);
      delay(250);
      sg90.write(90);
      delay(5000);
      sg90.write(0);
      Serial.println("SG90 returned to 0 degrees");
    }
  }
  delay(50);
}
