/*
  17_face_sg90
  HUB-8735 Ultra：HD H.264 + AAC RTSP、人臉辨識與 Wi-Fi 網頁控制

  RTSP：HD 1280x720、H.264、30 FPS、AAC 麥克風聲音
  網頁：http://<板子 IP>/
  網頁按鈕：Register、Delete、Reset、Backup、Restore

  注意：Register 時鏡頭畫面中只能有一張臉。
  OLED：SSD1306 128x64、I2C 0x3C、SDA -> IO3、SCL -> IO4
*/

#include <WiFi.h>
#include <FreeRTOS.h>
#include <task.h>
#include <AmebaServo.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include "NNFaceDetectionRecognition.h"
#include "VideoStreamOverlay.h"

#define VIDEO_CHANNEL 0
#define JPEG_CHANNEL 1
#define NN_CHANNEL 3
#define NN_WIDTH 576
#define NN_HEIGHT 320
#define SERVO_PIN 21
#define GREEN_LED_PIN 24
#define YELLOW_LED_PIN 23
#define RED_LED_PIN 22

char WIFI_SSID[] = "Seanpeng8787";
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";

VideoSetting videoConfig(VIDEO_HD, CAM_FPS, VIDEO_H264, 0);
VideoSetting jpegConfig(VIDEO_HD, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting nnConfig(NN_WIDTH, NN_HEIGHT, CAM_NN_FPS, VIDEO_RGB, 0);
AudioSetting audioConfig(3);  // 16 kHz、單聲道、數位 PDM 麥克風

Audio audio;
AAC aac;
RTSP rtsp;
NNFaceDetectionRecognition facerecog;
WiFiServer webServer(80);
WiFiServer mjpegServer(8080);

StreamIO audioStreamer(1, 1);    // Audio -> AAC
StreamIO avMixStreamer(2, 1);    // Video + AAC -> RTSP
StreamIO videoStreamerNN(1, 1);  // RGB NN channel -> face recognition

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
String lastWebMessage = "Ready";
WiFiClient mjpegClient;
bool mjpegClientActive = false;
bool mjpegStreaming = false;
unsigned long mjpegClientStartMs = 0;
uint32_t jpegImageAddress = 0;
uint32_t jpegImageLength = 0;

AmebaServo sg90;
volatile bool recognizedFaceInFrame = false;
volatile bool unknownFaceInFrame = false;
volatile bool anyFaceInFrame = false;
bool facePresenceHandled = false;
bool servoHoldingAt90 = false;
unsigned long servoHoldStartMs = 0;
unsigned long bootMs = 0;
unsigned long recognizedSinceMs = 0;
unsigned long noRecognizedSinceMs = 0;

const char MJPEG_BOUNDARY[] = "frame";

void webControlTask(void* parameter);

void drawStatus(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "HUB-8735 RTSP");
  u8g2.drawStr(0, 30, line1);
  if (line2 != nullptr) u8g2.drawStr(0, 45, line2);
  if (line3 != nullptr) u8g2.drawStr(0, 60, line3);
  u8g2.sendBuffer();
}

void formatIpAddress(IPAddress ip, char* output, size_t outputSize) {
  snprintf(output, outputSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void drawReadyScreen(IPAddress ip, int rtspPort) {
  char ipString[20];
  char rtspString[24];
  formatIpAddress(ip, ipString, sizeof(ipString));
  snprintf(rtspString, sizeof(rtspString), "RTSP:%d WEB:80", rtspPort);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "RTSP + FACE READY");
  u8g2.setCursor(0, 30);
  u8g2.print("IP: ");
  u8g2.print(ipString);
  u8g2.drawStr(0, 45, rtspString);
  u8g2.drawStr(0, 60, "HD H264 AAC WEB");
  u8g2.sendBuffer();
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

  IPAddress ip = WiFi.localIP();
  char ipString[20];
  formatIpAddress(ip, ipString, sizeof(ipString));
  drawStatus("WiFi connected", "IP address:", ipString);
  delay(1000);
}

char hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

void urlDecode(String& value) {
  String decoded;
  for (unsigned int i = 0; i < value.length(); i++) {
    if (value[i] == '%' && i + 2 < value.length()) {
      decoded += char((hexValue(value[i + 1]) << 4) | hexValue(value[i + 2]));
      i += 2;
    } else if (value[i] == '+') {
      decoded += ' ';
    } else {
      decoded += value[i];
    }
  }
  value = decoded;
}

String queryValue(const String& path, const char* key) {
  String marker = String("?") + key + "=";
  int start = path.indexOf(marker);
  if (start < 0) {
    marker = String("&") + key + "=";
    start = path.indexOf(marker);
  }
  if (start < 0) return "";

  start += marker.length();
  int end = path.indexOf('&', start);
  if (end < 0) end = path.length();
  String value = path.substring(start, end);
  urlDecode(value);
  return value;
}

void executeWebCommand(const String& path) {
  if (path.startsWith("/register")) {
    String name = queryValue(path, "name");
    if (name.length() == 0) {
      lastWebMessage = "Register needs a name";
    } else {
      facerecog.registerFace(name);
      facerecog.backupRegisteredFace();
      lastWebMessage = "Register requested";
    }
  } else if (path.startsWith("/delete")) {
    String name = queryValue(path, "name");
    if (name.length() == 0) {
      lastWebMessage = "Delete needs a name";
    } else {
      facerecog.removeFace(name);
      facerecog.backupRegisteredFace();
      lastWebMessage = "Delete requested";
    }
  } else if (path.startsWith("/reset")) {
    facerecog.resetRegisteredFace();
    facerecog.backupRegisteredFace();
    lastWebMessage = "All faces reset";
  } else if (path.startsWith("/backup")) {
    facerecog.backupRegisteredFace();
    lastWebMessage = "Faces backed up";
  } else if (path.startsWith("/restore")) {
    facerecog.restoreRegisteredFace();
    lastWebMessage = "Faces restored";
  }
}

void sendWebPage(WiFiClient& client) {
  char ipString[20];
  formatIpAddress(WiFi.localIP(), ipString, sizeof(ipString));
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  client.println("<title>HUB-8735 Face Control</title>");
  client.println("<style>body{font-family:Arial;max-width:520px;margin:24px auto;padding:0 16px}button{font-size:18px;padding:10px 16px;margin:5px 0;width:100%}input{font-size:18px;padding:10px;width:94%;margin:5px 0}.danger{background:#f8b0b0}.info{background:#e9f2ff;padding:10px}</style>");
  client.println("</head><body><h2>HUB-8735 Face Control</h2>");
  client.println("<div class='info'>Register 時，請讓鏡頭中只出現一張臉。</div>");
  client.print("<p><b>Last action:</b> ");
  client.print(lastWebMessage);
  client.println("</p>");
  client.println("<form action='/register' method='get'><input name='name' maxlength='24' placeholder='Name for the face' required><button type='submit'>Register face</button></form>");
  client.println("<form action='/delete' method='get'><input name='name' maxlength='24' placeholder='Registered name' required><button type='submit'>Delete face</button></form>");
  client.println("<form action='/backup' method='get'><button type='submit'>Backup faces to Flash</button></form>");
  client.println("<form action='/restore' method='get'><button type='submit'>Restore faces from Flash</button></form>");
  client.println("<form action='/reset' method='get' onsubmit=\"return confirm('Delete all registered faces?')\"><button class='danger' type='submit'>Reset all faces</button></form>");
  client.print("<p>RTSP: rtsp://");
  client.print(ipString);
  client.print(":");
  client.print(rtsp.getPort());
  client.println("</p>");
  client.print("<p>MJPEG preview: <a href='http://");
  client.print(ipString);
  client.println(":8080/'>http://" );
  client.print(ipString);
  client.println(":8080/</a></p>");
  client.println("</body></html>");
}

void handleWebServer() {
  WiFiClient client = webServer.available();
  if (!client) return;

  String requestLine;
  unsigned long startMs = millis();
  while (client.connected() && millis() - startMs < 1000UL) {
    while (client.available()) {
      char c = client.read();
      if (c == '\n') break;
      if (c != '\r' && requestLine.length() < 200) requestLine += c;
    }
    if (requestLine.length() > 0) break;
    delay(1);
  }

  if (requestLine.startsWith("GET ")) {
    int pathStart = 4;
    int pathEnd = requestLine.indexOf(' ', pathStart);
    if (pathEnd > pathStart) {
      String path = requestLine.substring(pathStart, pathEnd);
      executeWebCommand(path);
    }
  }

  sendWebPage(client);
  // 等待網頁內容確實送入網路緩衝區，再關閉 HTTP 連線。
  client.flush();
  delay(150);
  client.stop();
}

void webControlTask(void* parameter) {
  (void)parameter;
  for (;;) {
    handleWebServer();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void sendMjpegHeader(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Cache-Control: no-cache");
  client.println("Pragma: no-cache");
  client.print("Content-Type: multipart/x-mixed-replace; boundary=");
  client.println(MJPEG_BOUNDARY);
  client.println("Connection: close");
  client.println();
}

void sendMjpegFrame(WiFiClient& client, uint8_t* image, uint32_t length) {
  client.print("--");
  client.print(MJPEG_BOUNDARY);
  client.println();
  client.println("Content-Type: image/jpeg");
  client.print("Content-Length: ");
  client.println(length);
  client.println();
  client.write(image, length);
  client.println();
}

void handleMjpegServer() {
  if (!mjpegClientActive) {
    WiFiClient incoming = mjpegServer.available();
    if (incoming) {
      mjpegClient = incoming;
      mjpegClient.setNonBlockingMode();
      mjpegClientActive = true;
      mjpegClientStartMs = millis();
    }
    return;
  }

  if (!mjpegClient.connected()) {
    mjpegClient.stop();
    mjpegClientActive = false;
    mjpegStreaming = false;
    return;
  }

  // 等瀏覽器送完 HTTP request headers，再開始 multipart JPEG 串流。
  if (!mjpegStreaming) {
    while (mjpegClient.available()) mjpegClient.read();
    if (millis() - mjpegClientStartMs < 100UL) return;
    sendMjpegHeader(mjpegClient);
    mjpegStreaming = true;
  }

  Camera.getImage(JPEG_CHANNEL, &jpegImageAddress, &jpegImageLength);
  if (jpegImageAddress == 0 || jpegImageLength == 0) return;
  sendMjpegFrame(mjpegClient, (uint8_t*)jpegImageAddress, jpegImageLength);
  delay(30);
}

void FRPostProcess(std::vector<FaceRecognitionResult> results) {
  uint16_t imageWidth = videoConfig.width();
  uint16_t imageHeight = videoConfig.height();
  bool recognized = false;
  bool unknown = false;

  OSD.createBitmap(VIDEO_CHANNEL);
  for (int i = 0; i < facerecog.getResultCount(); i++) {
    FaceRecognitionResult item = results[i];
    if (String(item.name()) != String("unknown")) recognized = true;
    else unknown = true;
    int xmin = (int)(item.xMin() * imageWidth);
    int xmax = (int)(item.xMax() * imageWidth);
    int ymin = (int)(item.yMin() * imageHeight);
    int ymax = (int)(item.yMax() * imageHeight);
    uint32_t color = String(item.name()) == String("unknown") ? OSD_COLOR_RED : OSD_COLOR_GREEN;
    OSD.drawRect(VIDEO_CHANNEL, xmin, ymin, xmax, ymax, 3, color);

    char text[40];
    snprintf(text, sizeof(text), "Face:%s", item.name());
    OSD.drawText(VIDEO_CHANNEL, xmin, ymin - OSD.getTextHeight(VIDEO_CHANNEL), text, color);
  }
  recognizedFaceInFrame = recognized;
  unknownFaceInFrame = unknown;
  anyFaceInFrame = recognized || unknown;
  OSD.update(VIDEO_CHANNEL);
}

void updateStatusLeds() {
  // 優先顯示已辨識人臉；沒有已辨識人臉時，unknown 顯示紅燈；沒有人臉顯示黃燈。
  bool recognized = recognizedFaceInFrame;
  bool unknown = unknownFaceInFrame;
  digitalWrite(GREEN_LED_PIN, recognized ? HIGH : LOW);
  digitalWrite(RED_LED_PIN, (!recognized && unknown) ? HIGH : LOW);
  digitalWrite(YELLOW_LED_PIN, (!recognized && !unknown) ? HIGH : LOW);
}

void updateServoAction() {
  bool recognized = recognizedFaceInFrame;

  // 開機保護，避免模型啟動／還原資料時的短暫誤判觸發馬達。
  if (millis() - bootMs < 5000UL) {
    recognizedSinceMs = 0;
    noRecognizedSinceMs = 0;
    return;
  }

  // 必須連續看見已辨識人臉 1 秒才算有效，避免單幀誤判。
  if (recognized) {
    noRecognizedSinceMs = 0;
    if (recognizedSinceMs == 0) recognizedSinceMs = millis();
  } else {
    recognizedSinceMs = 0;
    if (noRecognizedSinceMs == 0) noRecognizedSinceMs = millis();
    // 已辨識人臉離開至少 1 秒後，才允許下一次觸發。
    if (millis() - noRecognizedSinceMs >= 1000UL) facePresenceHandled = false;
  }

  if (recognized && recognizedSinceMs != 0 &&
      millis() - recognizedSinceMs >= 1000UL &&
      !facePresenceHandled && !servoHoldingAt90) {
    sg90.write(90);
    servoHoldingAt90 = true;
    servoHoldStartMs = millis();
    facePresenceHandled = true;
  }

  if (servoHoldingAt90 && millis() - servoHoldStartMs >= 5000UL) {
    sg90.write(0);
    servoHoldingAt90 = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  bootMs = millis();
  sg90.attach(SERVO_PIN);
  sg90.write(0);
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  drawStatus("Starting...");
  connectWiFi();

  Camera.configVideoChannel(VIDEO_CHANNEL, videoConfig);
  Camera.configVideoChannel(JPEG_CHANNEL, jpegConfig);
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

  facerecog.configVideo(nnConfig);
  facerecog.modelSelect(FACE_RECOGNITION, NA_MODEL, DEFAULT_SCRFD, DEFAULT_MOBILEFACENET);
  facerecog.begin();
  facerecog.setResultCallback(FRPostProcess);

  videoStreamerNN.registerInput(Camera.getStream(NN_CHANNEL));
  videoStreamerNN.setStackSize();
  videoStreamerNN.setTaskPriority();
  videoStreamerNN.registerOutput(facerecog);
  if (videoStreamerNN.begin() != 0) {
    drawStatus("Face stream error");
    return;
  }

  // 開機自動還原先前備份的人臉資料，避免重新啟動後遺失。
  facerecog.restoreRegisteredFace();

  Camera.channelBegin(VIDEO_CHANNEL);
  Camera.channelBegin(JPEG_CHANNEL);
  Camera.channelBegin(NN_CHANNEL);

  OSD.configVideo(VIDEO_CHANNEL, videoConfig);
  OSD.begin();
  webServer.begin();
  mjpegServer.begin();
  xTaskCreate(
    webControlTask,
    "webControlTask",
    4096,
    nullptr,
    tskIDLE_PRIORITY + 1,
    nullptr
  );

  delay(1000);
  IPAddress ip = WiFi.localIP();
  drawReadyScreen(ip, rtsp.getPort());
}

void loop() {
  handleMjpegServer();
  updateStatusLeds();
  updateServoAction();
  delay(10);
}


