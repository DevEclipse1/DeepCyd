#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>
#include <vector>

TFT_eSPI tft = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT);

String api_key = "YOUR DEEPSEEK API KEY!!!";

String keyboard(const char* title)
{
  tft.fillScreen(TFT_BLACK);

  String buf;
  bool first = false;

  std::vector<const char*> keys = {
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "~",
      "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "~",
      "A", "S", "D", "F", "G", "H", "J", "K", "L", "~",
      "Z", "X", "C", "V", "B", "N", "M", ",", "/", "~",
      "---", "->", "<-", "CAPS", "~"
  };

  const int key_w = 20;
  const int key_h = 30;
  const int spacing_x = 5;
  const int spacing_y = 8;
  std::vector<int> keys_per_row = {10, 10, 9, 7, 2};
  bool pressed_this_frame = false;
  bool caps = false;

  while (true)
  {
    pressed_this_frame = false;

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(2, 2);
    tft.setTextWrap(false, false);
    tft.print(title);
    tft.print(" ");
    tft.print(caps ? String("(CAPS)") : "");

    tft.setTextSize(1);
    tft.setCursor(10, 35);
    tft.setTextWrap(true, true);
    tft.println(buf);

    uint16_t tx, ty;

    if (tft.getTouch(&tx, &ty) || !first)
    {
      tft.fillScreen(0x0000);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE);

      int row = 0, col = 0;
      int y = 60;

      for (int i = 0; i < keys.size(); ++i)
      {
        const char* key = keys[i];

        if (strcmp(key, "~") == 0)
        {
          row++;
          col = 0;
          y += key_h + spacing_y;
          continue;
        }

        int row_keys = keys_per_row[row];
        int total_w = 0;

        if (row == 4) {
          total_w = 4 * 60 + spacing_x;
        } else {
          total_w = row_keys * key_w + (row_keys - 1) * spacing_x;
        }

        int x = (tft.width() - total_w) / 2 + col * (row == 4 ? 60 + spacing_x : key_w + spacing_x);
        int w = (row == 4) ? 60 : key_w;

        tft.setCursor(x + (w - tft.textWidth(key)) / 2, y + (key_h - tft.fontHeight()) / 2);
        tft.print(key);
        tft.drawRoundRect(x, y, w, key_h, 4, ILI9341_LIGHTGREY);

        if (tx >= x && tx <= x + w && ty >= y && ty <= y + key_h)
        {
          if (strcmp(key, "---") == 0) buf += " ";
          else if (strcmp(key, "->") == 0) return buf;
          else if (strcmp(key, "<-") == 0 && buf.length() > 0) buf.remove(buf.length() - 1);
          else if (strcmp(key, "CAPS") == 0) caps = !caps;
          else {
            String lower = key;
            lower.toLowerCase();

            buf += caps ? key : lower;
          }

          pressed_this_frame = true;
        }

        col++;
      }

      if (pressed_this_frame)
      {
        delay(150);
      }

      first = true;
    }

    delay(1);
  }
}

void calibrate_touch()
{
  uint16_t calData[5];

  fs::File f = SPIFFS.open("/touch.dat");
  if (f && f.available() == sizeof(calData)) {
    f.read((uint8_t*)calData, sizeof(calData));
    tft.setTouch(calData);
    f.close();
    return;
  }

  tft.calibrateTouch(calData, ILI9341_WHITE, TFT_BLACK, 35);
  tft.setTouch(calData);

  f = SPIFFS.open("/touch.dat", "w");
  if (f) {
    f.write((const uint8_t*)calData, sizeof(calData));
    f.close();
  }
}

void calibrate_wifi(std::vector<String> ssids)
{
  tft.setTextSize(2);
  tft.fillScreen(0x0000);
  tft.setCursor(0, 0);

  int y = 0;
  String saved_ssid = "";
  String saved_password = "";

  if (SPIFFS.exists("/wifi_credentials.txt")) {
    fs::File credentialsFile = SPIFFS.open("/wifi_credentials.txt", "r");
    if (credentialsFile) {
      saved_ssid = credentialsFile.readStringUntil('\n');
      saved_password = credentialsFile.readStringUntil('\n');
      credentialsFile.close();
      saved_ssid.trim();
      saved_password.trim();
    }
  }

  bool ssid_not_found = true;

  for (String ssid : ssids) {
    if (ssid == saved_ssid) {
      ssid_not_found = false;
      break;
    }
  }

  if (saved_ssid == "" || saved_password == "" || ssid_not_found) {
    fs::File credentialsFile = SPIFFS.open("/wifi_credentials.txt", "w");
    credentialsFile.close();

    while (saved_ssid == "") {
      for (String ssid : ssids) {
        uint16_t h = tft.fontHeight() + 6;

        tft.setTextWrap(false, false);
        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(5, y + 3);
        tft.print(ssid);
        tft.drawRoundRect(0, y, tft.width(), h, 4, ILI9341_LIGHTGREY);

        uint16_t tx, ty;
        if (tft.getTouch(&tx, &ty)) {
          if (tx >= 0 && tx <= tft.width() && ty >= y && ty <= y + h) {
            saved_ssid = ssid;
            break;
          }
        }

        y += h;
      }

      y = 0;
      delay(1);
    }

    String password = keyboard("Enter password: ");
    saved_password = password;
  }

  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Selected SSID: \n");
  tft.print(saved_ssid);

  delay(250);

  WiFi.mode(WIFI_STA);
  WiFi.begin(saved_ssid.c_str(), saved_password.c_str());

  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setCursor(10, 10);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    tft.print(".");
    delay(500);
    attempts++;
  }

  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);

  if (WiFi.status() == WL_CONNECTED) {
    tft.print("Connected!");

    fs::File _credentialsFile = SPIFFS.open("/wifi_credentials.txt", "w");
    if (_credentialsFile) {
      _credentialsFile.println(saved_ssid);
      _credentialsFile.println(saved_password);
      _credentialsFile.close();
    }
  } else {
    fs::File _credentialsFile = SPIFFS.open("/wifi_credentials.txt", "w");
    _credentialsFile.close();

    tft.print("Failed to connect");
  
    ESP.restart();
  }

  delay(1000);
}

String buildPayload(const String& prompt) {
  JsonDocument doc;
  doc["model"] = "deepseek-chat";
  doc["stream"] = false;

  JsonArray messages = doc["messages"].to<JsonArray>();
  JsonObject systemMsg = messages.add<JsonObject>();
  systemMsg["role"] = "system";
  systemMsg["content"] = "You are a helpful assistant, and type short to save tokens please";

  JsonObject userMsg = messages.add<JsonObject>();
  userMsg["role"] = "user";
  userMsg["content"] = prompt;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

String generate_response(String prompt)
{
  String host = "api.deepseek.com";

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(5000);

  String payload = buildPayload(prompt);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true, true);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(payload);

  if (!client.connect(host.c_str(), 443));

  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true, true);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Connected");

  String request = String("POST /v1/chat/completions HTTP/1.1\r\n") +
                   "Host: " + host + "\r\n" +
                   "Authorization: Bearer " + api_key + "\r\n" +
                   "Content-Type: application/json\r\n" +
                   "Content-Length: " + payload.length() + "\r\n\r\n" +
                   payload;

  client.print(request);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true, true);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(request);
  
  String response;
  bool run = true;
  while (client.connected() || client.available() || run) {
    if (client.available()) {
      response += client.readStringUntil('\n');

      tft.fillScreen(0x0000);
      tft.setTextSize(1);
      tft.setCursor(0, 0);
      tft.setTextWrap(true, true);
      tft.setTextColor(ILI9341_WHITE);
      tft.print("Waiting for response...");

      if (response.indexOf("}") != -1)
      {
        run = false;
        break;
      }
    }
  }
  
  int jsonStart = response.indexOf('{');
  String jsonString = response.substring(jsonStart);  

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    tft.fillScreen(0x0000);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.setTextWrap(true, true);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("JSON Fail");
    return "Failed";
  }

  const char* content = doc["choices"][0]["message"]["content"];
  
  return content;
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  if (!SPIFFS.begin(false, "/deepcyd"))
  {
    SPIFFS.format();
  }

  tft.setTextSize(4);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Deep");
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("Cyd");
  
  delay(1000);

  calibrate_touch();

  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Scanning Networks...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int n = WiFi.scanNetworks();
  if (n == 0) {
    return;
  } else {
    std::vector<String> ssids;
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      ssids.push_back(ssid);
    }
    calibrate_wifi(ssids);
  }
}

void loop() {
  String prompt = keyboard("Enter prompt: ");
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true, true);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(prompt);

  String response = generate_response(prompt);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextWrap(true, true);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(response);

  bool run = true;
  uint16_t tx, ty;
  unsigned long touchStart = 0;

  while (run) {
    if (tft.getTouch(&tx, &ty)) {
      if (touchStart == 0) touchStart = millis();
      if (millis() - touchStart > 1500) {
        run = false;
      }
    } else {
      touchStart = 0;
    }
    delay(1);
  }
}