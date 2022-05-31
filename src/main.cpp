// LED Marquee Totem
// Sasha Ramirez 2021

#include <Arduino.h>
#include <WiFi.h>

#include <FastLED.h>
#include <LEDMatrix.h>
#include <LEDText.h>
#include <FontMatrise.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <Arduino_JSON.h>
#include <arduinoFFT.h>

// FFT Init

// Web Server Init
const char *ssid = "Marquee1";
const char *password = "letsparty1";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "inputColor1";
const char *PARAM_INPUT_2 = "inputColor2";
const char *PARAM_INPUT_3 = "brightness";
const char *PARAM_INPUT_4 = "inputString";
const char *PARAM_INPUT_5 = "colorMode";
const char *PARAM_INPUT_6 = "speed";
const char *PARAM_INPUT_7 = "reactivityMode";
const char *PARAM_INPUT_8 = "systemMode";

// Variables to save values from HTML form
String inputColor1, inputColor2, inputString, brightness, colorMode, speed, reactivityMode, systemMode;

bool newRequest = false;

uint messageSize = 110;
// LED Initialization
#define NUM_LEDS_RING 25
#define NUM_LEDS_DL 30

#define LED_PIN1 15
#define LED_PIN2 27
#define LED_PIN3 12
#define LED_PIN4 14
#define LED_PIN5 32

#define COLOR_ORDER GRB
#define CHIPSET WS2812B

#define MATRIX_WIDTH 60
#define MATRIX_HEIGHT -7
#define MATRIX_TYPE HORIZONTAL_ZIGZAG_MATRIX

cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds1;
cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds2;

CRGB leds3[NUM_LEDS_RING];
CRGB leds4[NUM_LEDS_DL];
CRGB leds5[NUM_LEDS_DL];

cLEDText ScrollingMsg1;
cLEDText ScrollingMsg2;

String currentText;

unsigned char TxtDisplay[256] = {"            Wake up Neo...       The Matrix has you...      Follow the White Rabbit     Knock, knock, Neo.   "}; // 4 chars + null char

// List of patterns to cycle through.  Each is defined as a separate function below.

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // rotating "base color" used by many of the patterns

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

String message = "";
String sliderValue1 = "0";
String sliderValue2 = "0";

String toggleValue1 = "off";
String toggleValue2 = "off";
String toggleValue3 = "1";
String colorValue1 = "#0000ff";
String colorValue2 = "#0000ff";
String colorValue3 = "#0000ff";

// Json Variable to Hold Values
JSONVar elementValues;

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void addGlitter(fract8 chanceOfGlitter)
{
  if (random8() < chanceOfGlitter)
  {
    leds3[random16(NUM_LEDS_RING)] += CRGB::White;
    leds4[random16(NUM_LEDS_DL)] += CRGB::White;
    leds5[random16(NUM_LEDS_DL)] += CRGB::White;
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds3, NUM_LEDS_RING, 20);
  fadeToBlackBy(leds4, NUM_LEDS_DL, 20);
  fadeToBlackBy(leds5, NUM_LEDS_DL, 20);
  int pos1 = beatsin16(13, 0, NUM_LEDS_RING - 1);
  int pos2 = beatsin16(13, 0, NUM_LEDS_DL - 1);
  leds3[pos1] += CHSV(gHue, 255, 255);
  leds4[pos2] += CHSV(gHue, 255, 255);
  leds5[pos2] += CHSV(gHue, 255, 255);
}

void juggle()
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds3, NUM_LEDS_RING, 20);
  fadeToBlackBy(leds4, NUM_LEDS_DL, 20);
  fadeToBlackBy(leds5, NUM_LEDS_DL, 20);
  uint8_t dothue = 0;
  for (int i = 0; i < 8; i++)
  {
    leds3[beatsin16(i + 7, 0, NUM_LEDS_RING - 1)] |= CHSV(dothue, 200, 255);
    leds4[beatsin16(i + 7, 0, NUM_LEDS_DL - 1)] |= CHSV(dothue, 200, 255);
    leds5[beatsin16(i + 7, 0, NUM_LEDS_DL - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds3, NUM_LEDS_RING, 20);
  fadeToBlackBy(leds4, NUM_LEDS_DL, 20);
  fadeToBlackBy(leds5, NUM_LEDS_DL, 20);
  int pos1 = random16(NUM_LEDS_RING);
  int pos2 = random16(NUM_LEDS_DL);
  leds3[pos1] += CHSV(gHue + random8(64), 200, 255);
  leds4[pos2] += CHSV(gHue + random8(64), 200, 255);
  leds5[pos2] += CHSV(gHue + random8(64), 200, 255);
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow(leds3, NUM_LEDS_RING, gHue, 7);
  fill_rainbow(leds4, NUM_LEDS_DL, gHue, 7);
  fill_rainbow(leds5, NUM_LEDS_DL, gHue, 7);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS_RING; i++)
  { // 9948
    leds3[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
  for (int i = 0; i < NUM_LEDS_DL; i++)
  { // 9948
    leds4[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    leds5[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm};

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

// Get Slider Values
String getElementValues()
{
  elementValues["sliderValue1"] = String(elementValue1);
  elementValues["sliderValue2"] = String(elementValue2);
  String jsonString = JSON.stringify(elementValues);
  return jsonString;
}

// Initialize SPIFFS
void initFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi()
{
  Serial.print("Setting soft access point mode");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void notifyClients(String temp)
{
  ws.textAll(temp);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;
    if (message.indexOf("1e") >= 0)
    {
      sliderValue1 = message.substring(2);
      // dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);
      // Serial.println(dutyCycle1);
      Serial.print(getElementValues());
      notifyClients(getElementValues());
    }
    if (message.indexOf("2s") >= 0)
    {
      sliderValue2 = message.substring(2);
      // dutyCycle2 = map(sliderValue2.toInt(), 0, 100, 0, 255);
      // Serial.println(dutyCycle2);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (message.indexOf("1t") >= 0)
    {
      toggleValue1 = message.substring(2);
      Serial.print(getToggleValues());
      notifyClients(getToggleValues());
    }
    if (message.indexOf("2t") >= 0)
    {
      toggleValue2 = message.substring(2);
      Serial.print(getToggleValues());
      notifyClients(getToggleValues());
    }
    if (message.indexOf("3t") >= 0)
    {
      toggleValue3 = message.substring(2);
      Serial.print(getToggleValues());
      notifyClients(getToggleValues());
    }
    if (message.indexOf("1c") >= 0)
    {
      colorValue1 = message.substring(2);
      Serial.print(getColorValues());
      notifyClients(getColorValues());
    }
    if (message.indexOf("2c") >= 0)
    {
      colorValue2 = message.substring(2);
      Serial.print(getColorValues());
      notifyClients(getColorValues());
    }
    if (message.indexOf("3c") >= 0)
    {
      colorValue3 = message.substring(2);
      Serial.print(getColorValues());
      notifyClients(getColorValues());
    }
    if (strcmp((char *)data, "getValues") == 0)
    {
      notifyClients(getSliderValues());
      notifyClients(getToggleValues());
      notifyClients(getColorValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup()
{
  Serial.begin(115200);
  initFS();
  initWiFi();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

  // //SPIFFS Setup
  // if(!SPIFFS.begin(true)){
  //   Serial.println("An Error has occurred while mounting SPIFFS");
  //   return;
  // }
  // Serial.print("Setting soft access point mode");
  // WiFi.softAP(ssid, password);
  // IPAddress IP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(IP);

  // // Web Server Root URL
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(SPIFFS, "/index.html", "text/html");
  // });

  // server.serveStatic("/", SPIFFS, "/");

  // server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
  //   int params = request->params();
  //   for(int i=0;i<params;i++){
  //     AsyncWebParameter* p = request->getParam(i);
  //     if(p->isPost()){
  //       // HTTP POST input1 value
  //       if (p->name() == PARAM_INPUT_1) {
  //         inputColor1 = p->value().c_str();
  //         inputColor1.remove(0,1);
  //         Serial.print("Color: ");
  //         Serial.println(inputColor1);
  //       }
  //       // HTTP POST input1 value
  //       if (p->name() == PARAM_INPUT_2) {
  //         inputColor2 = p->value().c_str();
  //         inputColor2.remove(0,1);
  //         Serial.print("Color: ");
  //         Serial.println(inputColor2);
  //       }
  //       // HTTP POST input2 value
  //       if (p->name() == PARAM_INPUT_3) {
  //         brightness = p->value().c_str();
  //         Serial.print("Brightness: ");
  //         Serial.println(brightness);
  //       }
  //       // HTTP POST input3 value
  //       if (p->name() == PARAM_INPUT_4) {
  //         inputString = p->value().c_str();
  //         Serial.print("Display Text: ");
  //         Serial.println(inputString);
  //       }
  //       // HTTP POST input4 value
  //       if (p->name() == PARAM_INPUT_5) {
  //         colorMode = p->value().c_str();
  //         Serial.print("Color Mode: ");
  //         Serial.println(colorMode);
  //       }
  //       // HTTP POST input3 value
  //       if (p->name() == PARAM_INPUT_6) {
  //         speed = p->value().c_str();
  //         Serial.print("Speed: ");
  //         Serial.println(speed);
  //       }
  //       newRequest = true;
  //       //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
  //     }
  //   }
  //   request->send(SPIFFS, "/index.html", "text/html");
  // });
  // server.onNotFound(notFound);
  // server.begin();

  // LED Setup

  FastLED.addLeds<CHIPSET, LED_PIN1, COLOR_ORDER>(leds1[0], leds1.Size()).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, LED_PIN2, COLOR_ORDER>(leds2[0], leds2.Size()).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, LED_PIN3, COLOR_ORDER>(leds3, NUM_LEDS_RING).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, LED_PIN4, COLOR_ORDER>(leds4, NUM_LEDS_DL).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<CHIPSET, LED_PIN5, COLOR_ORDER>(leds4, NUM_LEDS_DL).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(64);
  FastLED.clear(true);
  delay(500);
  FastLED.show();
  ScrollingMsg1.SetFont(MatriseFontData);
  ScrollingMsg1.Init(&leds1, leds1.Width(), ScrollingMsg1.FontHeight() + 1, 0, 0);
  ScrollingMsg1.SetText((unsigned char *)TxtDisplay, messageSize);
  ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0, 255, 0);
  ScrollingMsg1.SetFrameRate(2);
  ScrollingMsg2.SetFont(MatriseFontData);
  ScrollingMsg2.Init(&leds2, leds2.Width(), ScrollingMsg2.FontHeight() + 1, 0, 0);
  ScrollingMsg2.SetText((unsigned char *)TxtDisplay, messageSize);
  ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_SINGLE, 0, 255, 0);
  ScrollingMsg2.SetFrameRate(2);
}

void marquee()
{
  if (ScrollingMsg1.UpdateText() == -1 || ScrollingMsg2.UpdateText() == -1)
  {
    if (newRequest)
    {
      char temp[2];

      temp[0] = (inputColor1.charAt(0));
      temp[1] = (inputColor1.charAt(1));
      uint8_t r1 = strtoul(temp, 0, 16);

      temp[0] = (inputColor1.charAt(2));
      temp[1] = (inputColor1.charAt(3));
      uint8_t g1 = strtoul(temp, 0, 16);

      temp[0] = (inputColor1.charAt(4));
      temp[1] = (inputColor1.charAt(5));
      uint8_t b1 = strtoul(temp, 0, 16);

      temp[0] = (inputColor2.charAt(0));
      temp[1] = (inputColor2.charAt(1));
      uint8_t r2 = strtoul(temp, 0, 16);

      temp[0] = (inputColor2.charAt(2));
      temp[1] = (inputColor2.charAt(3));
      uint8_t g2 = strtoul(temp, 0, 16);

      temp[0] = (inputColor2.charAt(4));
      temp[1] = (inputColor2.charAt(5));
      uint8_t b2 = strtoul(temp, 0, 16);

      if (colorMode == "CM1")
      {
        Serial.print("Color Mode 1");
        ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_SINGLE, r1, g1, b1);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_SINGLE, r1, g1, b1);
      }
      else if (colorMode == "CM2")
      {
        Serial.print("Color Mode 2");
        ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_GRAD_CV, r1, g1, b1, r2, g2, b2);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_GRAD_CV, r1, g1, b1, r2, g2, b2);
      }
      else if (colorMode == "CM3")
      {
        Serial.print("Color Mode 3");
        ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_GRAD_AV, r1, g1, b1, r2, g2, b2);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_GRAD_CV, r1, g1, b1, r2, g2, b2);
      }
      else if (colorMode == "CM4")
      {
        Serial.print("Color Mode 4");
        ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_GRAD_CH, r1, g1, b1, r2, g2, b2);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_GRAD_CV, r1, g1, b1, r2, g2, b2);
      }
      else if (colorMode == "CM5")
      {
        Serial.print("Color Mode 5");
        ScrollingMsg1.SetTextColrOptions(COLR_RGB | COLR_GRAD_AH, r1, g1, b1, r2, g2, b2);
        ScrollingMsg2.SetTextColrOptions(COLR_RGB | COLR_GRAD_CV, r1, g1, b1, r2, g2, b2);
      }
      ScrollingMsg1.SetFrameRate(speed.toInt());
      ScrollingMsg2.SetFrameRate(speed.toInt());
      FastLED.setBrightness(brightness.toInt());
      newRequest = false;
      // Clear first
      memset(&TxtDisplay, 0, sizeof(TxtDisplay) - 1);
      strcpy((char *)&TxtDisplay, inputString.c_str());
      messageSize = inputString.length();
      ScrollingMsg1.SetText((unsigned char *)TxtDisplay, messageSize);
      ScrollingMsg2.SetText((unsigned char *)TxtDisplay, messageSize);
      Serial.println(inputString.length());
    }
    else
    {
      ScrollingMsg1.SetText((unsigned char *)TxtDisplay, messageSize);
      ScrollingMsg2.SetText((unsigned char *)TxtDisplay, messageSize);
    }
  }

  else
  {
    FastLED.show();
  }
}

void loop()
{
  // marquee();
  ws.cleanupClients();
  delay(10);
}
