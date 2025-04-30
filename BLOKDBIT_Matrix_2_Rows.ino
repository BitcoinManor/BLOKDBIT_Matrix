// 🚀 BLOKDBIT Matrix: Dual-Row Static Display Version
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Font_Data.h"  // Optional, if using custom fonts
#include "time.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
DNSServer dnsServer;
#include <Preferences.h>
Preferences prefs;

bool wifiConnected = false;

String savedSSID;
String savedPassword;
String savedCity;
int savedTimezone = -99;

// Fetch and Display Cycles
uint8_t fetchCycle = 0;  // 👈 for rotating which API we fetch
uint8_t displayCycle = 0; // 👈 for rotating which screen we show



//initializes the server so we can later attach our custom HTML page routes
AsyncWebServer server(80);


// 🌍 API Endpoints
const char* BTC_API = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";
const char* BLOCK_API = "https://blockchain.info/q/getblockcount";
const char* FEES_API = "https://mempool.space/api/v1/fees/recommended";
const char* MEMPOOL_BLOCKS_API = "https://mempool.space/api/blocks";
const char* BLOCKSTREAM_TX_API_BASE = "https://blockstream.info/api/block/";



// Miner Pool Detection
String minerName = "Unknown";

struct MinerTag {
  const char* tag;
  const char* name;
};

const MinerTag knownTags[] = {
  { "f2pool", "F2Pool" }, { "antpool", "AntPool" }, { "viabtc", "ViaBTC" },
  { "poolin", "Poolin" }, { "btccom", "BTC.com" }, { "binance", "Binance Pool" },
  { "carbon", "Carbon Negative" }, { "slush", "Slush Pool" }, { "braiins", "Braiins Pool" },
  { "foundry", "Foundry USA" }, { "ocean", "Ocean Pool" }, { "mara", "Marathon" },
  { "marathon", "Marathon" }, { "luxor", "Luxor" }, { "ultimus", "ULTIMUSPOOL" },
  { "novablock", "NovaBlock" }, { "sigma", "SigmaPool" }, { "spider", "SpiderPool" },
  { "tera", "TERA Pool" }, { "okex", "OKEx Pool" }, { "kucoin", "KuCoin Pool" },
  { "sbi", "SBI Crypto" }, { "btctop", "BTC.TOP" }, { "emcd", "EMCD Pool" },
  { "secpool", "SECPOOL" }, { "hz", "HZ Pool" }, { "solo.ckpool", "Solo CKPool" },
  { "solopool", "Solo Pool" }, { "solo", "Solo Miner" }, { "bitaxe", "Bitaxe Solo Miner" },
  { "node.pw", "Node.PW" }, { "/axe/", "Bitaxe Solo Miner" }
};

String hexToAscii(const String& hex) {
  String ascii = "";
  for (unsigned int i = 0; i < hex.length(); i += 2) {
    char c = (char) strtol(hex.substring(i, i + 2).c_str(), nullptr, 16);
    if (isPrintable(c)) ascii += c;
  }
  return ascii;
}

String identifyMiner(String scriptSig) {
  scriptSig.toLowerCase();
  for (const auto& tag : knownTags) {
    if (scriptSig.indexOf(tag.tag) != -1) return tag.name;
  }
  return "Unknown";
}




String mapWeatherCode(int code) {
  if (code == 0) return "Sunny";
  else if (code == 1) return "Mostly Sunny";
  else if (code == 2) return "Partly Cloudy";
  else if (code == 3) return "Cloudy";
  else if (code >= 45 && code <= 48) return "Foggy";
  else if (code >= 51 && code <= 57) return "Drizzle";
  else if (code >= 61 && code <= 67) return "Rain";
  else if (code >= 71 && code <= 77) return "Snowy";
  else if (code >= 80 && code <= 82) return "Showers";
  else if (code >= 85 && code <= 86) return "Snow Showers";
  else if (code >= 95 && code <= 99) return "Thunderstorm";
  else return "Unknown";
}



// Time Config
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = -7 * 3600;
int daylightOffset_sec = 3600;

//Global Data Variables
int btcPrice = 0, blockHeight = 0, feeRate = 0, satsPerDollar = 0;
char btcText[16], blockText[16], feeText[16], satsText[16];
char timeText[16], dateText[16], dayText[16];
float latitude = 0.0;
float longitude = 0.0;
String weatherCondition = "Unknown";
int temperature = 0;




String formatWithCommas(int number) {
  String numStr = String(number);
  String result = "";
  int len = numStr.length();
  for (int i = 0; i < len; i++) {
    if (i > 0 && (len - i) % 3 == 0) result += ",";
    result += numStr[i];
  }
  return result;
}




// LED Matrix Config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_ZONES 2
#define ZONE_SIZE 8
#define MAX_DEVICES (MAX_ZONES * ZONE_SIZE)
#define FETCH_INTERVAL 120000

#define ZONE_LOWER 0
#define ZONE_UPPER 1

#define CLK_PIN 18
#define DATA_PIN 23
#define CS_PIN 5



MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
//int brightnessLevel = 6;  // 0 = dimmest, 15 = brightest
unsigned long lastFetchTime = 0;
uint8_t cycle = 0;                 // 🔥 Needed for animation control
unsigned long lastApiCall = 0;      // 🔥 Needed for fetch timing
unsigned long lastMemoryCheck = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastNTPUpdate = 0;

const unsigned long WEATHER_UPDATE_INTERVAL = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long NTP_UPDATE_INTERVAL = 10UL * 60UL * 1000UL; // 10 minutes
const unsigned long MEMORY_CHECK_INTERVAL = 5UL * 60UL * 1000UL; // 5 minutes


//Pre Connection Message for home users
void showPreConnectionMessage() {
  static uint8_t step = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate < 2500) return; // Wait for 2.5 seconds between steps
  lastUpdate = millis();

  switch (step) {
    case 0:
      P.displayZoneText(ZONE_UPPER, "WELCOME to", PA_CENTER, 0, 2500, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "BLOKDBIT", PA_CENTER, 0, 2500, PA_FADE, PA_FADE);
      break;
    case 1:
      P.displayZoneText(ZONE_UPPER, "Connect Your", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "Device Inside", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      break;
    case 2:
      P.displayZoneText(ZONE_UPPER, "WiFi Settings", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "Labelled", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      break;
    case 3:
      P.displayZoneText(ZONE_UPPER, "BLOKDBIT-", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "MATRIX", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      break;
    case 4:
      P.displayZoneText(ZONE_UPPER, "OR TYPE", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "192.168.4.1", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      break;
    case 5:
      P.displayZoneText(ZONE_UPPER, "SETUP WiFi", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      P.displayZoneText(ZONE_LOWER, "and hit SAVE", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
      break;
    default:
      step = 0; // Reset the sequence
      return;
  }

  step++;
}



  //Load Saved WiFi + City + Timezone on Boot
void loadSavedSettingsAndConnect() {
  prefs.begin("blokdbit", true);  

  savedSSID = prefs.getString("ssid", "");
  savedPassword = prefs.getString("password", "");
  savedCity = prefs.getString("city", "");
  savedTimezone = prefs.getInt("timezone", -99);

  prefs.end();

  if (savedSSID != "" && savedPassword != "") {
    Serial.println("✅ Found Saved WiFi Credentials:");
    Serial.println("SSID: " + savedSSID);
    Serial.println("Password: " + savedPassword);
    Serial.println("City: " + savedCity);
    Serial.print("Timezone offset (hours): ");
    Serial.println(savedTimezone);

    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    Serial.print("🔌 Connecting to WiFi...");
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Connected to WiFi successfully!");
      Serial.print("🌍 IP Address: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true; // 👉 set this!!
      
      if (savedTimezone != -99) {
        gmtOffset_sec = savedTimezone * 3600;
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Serial.println("🕒 Timezone configured");
      }
    } else {
      Serial.println("\n❌ Failed to connect to WiFi, falling back to Access Point...");
      startAccessPoint();
    }
  } else {
    Serial.println("⚠️ No saved WiFi credentials found, starting Access Point...");
    startAccessPoint();
  }
}




//Access Point Code
void startAccessPoint() {
  Serial.println("🚀 Starting Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("BLOKDBIT-MATRIX");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("🌍 AP IP address: ");
  Serial.println(myIP);

  // DNS Captive portal
  dnsServer.start(53, "*", myIP);
  Serial.println("🚀 DNS Server started for captive portal.");
}





// Fetch Functions
void fetchBitcoinData() {
  Serial.println("🔄 Fetching BTC Price...");
  HTTPClient http;
  http.begin(BTC_API);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());
    btcPrice = doc["bitcoin"]["usd"];
    satsPerDollar = 100000000 / btcPrice;
    sprintf(btcText, "$%s", formatWithCommas(btcPrice).c_str());
    sprintf(satsText, "%d sats/$", satsPerDollar);
    Serial.printf("✅ Updated BTC Price: $%d | Sats per $: %d\n", btcPrice, satsPerDollar);
    Serial.printf("✅ BTC Price: %s (%s)\n", btcText, satsText);
  } else {
    Serial.println("❌ Failed to fetch BTC Price");
  }
  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}





void fetchBlockHeight() {
  Serial.println("🔄 Fetching Block Height...");
  HTTPClient http;
  http.begin(BLOCK_API);
  if (http.GET() == 200) {
    blockHeight = http.getString().toInt();
    sprintf(blockText, "%d", blockHeight);
    Serial.printf("✅ Updated Block Height: %d\n", blockHeight);
    Serial.printf("✅ Block Height: %s\n", blockText);
  } else {
    Serial.println("❌ Failed to fetch Block Height");
  }
  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}

void fetchFeeRate() {
  Serial.println("🔄 Fetching Fee Rate...");
  HTTPClient http;
  http.begin(FEES_API);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());
    feeRate = doc["fastestFee"];
    sprintf(feeText, "%d sat/vB", feeRate);
    Serial.printf("✅ Updated Fee Rate: %d sat/vB\n", feeRate);
    Serial.printf("✅ Fee Rate: %s\n", feeText);
  } else {
    Serial.println("❌ Failed to fetch Fee Rate");
  }
  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}

void fetchMinerName() {
  Serial.println("🔄 Fetching Miner Name...");
  HTTPClient http;
  http.begin(MEMPOOL_BLOCKS_API);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());
    String blockHash = doc[0]["id"];
    http.end();
    http.begin(String(BLOCKSTREAM_TX_API_BASE) + blockHash + "/txs");
    if (http.GET() == 200) {
      DynamicJsonDocument txDoc(8192);
      deserializeJson(txDoc, http.getString());
      String scriptSig = txDoc[0]["vin"][0]["scriptsig"].as<String>();
      minerName = identifyMiner(hexToAscii(scriptSig));
      Serial.printf("✅ Updated Miner Name: %s\n", minerName.c_str());
      Serial.printf("✅ Mined By: %s\n", minerName.c_str());
    } else {
      Serial.println("❌ Failed to fetch Miner Name (txs)");
    }
  } else {
    Serial.println("❌ Failed to fetch Mempool Blocks");
  }
  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}


void fetchTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("❌ Failed to fetch local time!");
    return;
  }

  Serial.println("⏰ Local time fetched successfully!");

  // 1) Format into a temporary buffer with %I (01–12), then strip leading ‘0’
  char buf[16];
  strftime(buf, sizeof(buf), "%I:%M%p", &timeinfo);
  
  Serial.print("🕒 Raw time format: ");
  Serial.println(buf);

  // If hour is 01–09, buf[0]=='0' → shift everything left by one
  if (buf[0] == '0') {
    Serial.println("🔧 Leading zero detected, removing...");
    memmove(buf, buf + 1, strlen(buf + 1) + 1);
  }

  // Copy into your global
  strncpy(timeText, buf, sizeof(timeText));
  timeText[sizeof(timeText)-1] = '\0';

  Serial.print("✅ Final formatted timeText: ");
  Serial.println(timeText);
  Serial.printf("✅ Updated Time: %s | Date: %s | Day: %s\n", timeText, dateText, dayText);


  // 2) Date and day as before
  strftime(dateText, sizeof(dateText), "%b %d", &timeinfo);
  strftime(dayText, sizeof(dayText), "%A", &timeinfo);

  Serial.print("📅 Date: ");
  Serial.println(dateText);
  Serial.print("📆 Day: ");
  Serial.println(dayText);
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());
}



void fetchLatLonFromCity() {
  if (savedCity == "") {
    Serial.println("⚠️ No saved city found, skipping geolocation fetch.");
    return;
  }

  HTTPClient http;
  String url = "https://nominatim.openstreetmap.org/search?city=" + savedCity + "&format=json";
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getString());

    if (!doc.isNull() && doc.size() > 0) {
      String latStr = doc[0]["lat"];
      String lonStr = doc[0]["lon"];

      Serial.println("🌎 Found City Location:");
      Serial.println("Latitude: " + latStr);
      Serial.println("Longitude: " + lonStr);

      latitude = latStr.toFloat();
      longitude = lonStr.toFloat();
    } else {
      Serial.println("❌ No matching city found!");
    }
  } else {
    Serial.print("❌ HTTP Request failed, code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());

}





void fetchWeather() {
  if (savedCity == "") {
    Serial.println("❌ City not set, skipping weather fetch.");
    return;
  }

  String weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 6) +
                      "&longitude=" + String(longitude, 6) +
                      "&current=temperature_2m,weather_code&timezone=auto";

  HTTPClient http;
  http.begin(weatherURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    if (payload.length() == 0) {
      Serial.println("❌ Empty weather payload received!");
      http.end();
      return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float temp = doc["current"]["temperature_2m"];
      int weatherCode = doc["current"]["weather_code"];
      String condition = mapWeatherCode(weatherCode);

      temperature = (int)temp;
      weatherCondition = condition;
      Serial.printf("✅ Updated Weather: %d°C | Condition: %s\n", temperature, weatherCondition.c_str());
      Serial.print("🌡️ Temperature: ");
      Serial.println(temperature);
      Serial.println("🌦️ Condition: " + weatherCondition);
    } else {
      Serial.println("❌ Failed to parse weather JSON");
    }
  } else {
    Serial.println("❌ Weather fetch failed, HTTP code: " + String(httpCode));
  }

  http.end();  // ✅ Always clean up!
  Serial.printf("📈 Free heap after fetch: %d bytes\n", ESP.getFreeHeap());

}






  


//Setup of device

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLOKDBIT Matrix Setup...");

  
  // Monitor available heap memory
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Minimum free heap: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

  // 🗂️ Mount SPIFFS
  Serial.println("🗂️ Mounting SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Failed to mount SPIFFS");
    return;
  }
  Serial.println("✅ SPIFFS mounted successfully!");

  if (!SPIFFS.exists("/BLOKDBIT_Wifi_Portal.html.gz")) {
    Serial.println("❌ HTML file NOT found");
  } else {
    Serial.println("✅ Custom HTML file found");
  }



  // Try WiFi first, fallback if needed
  Serial.println("📡 Loading saved WiFi and settings...");
  loadSavedSettingsAndConnect();



  // LED Matrix Startup
  Serial.println("💡 Initializing LED Matrix...");
  P.begin(MAX_ZONES);
  P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
  P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
  P.setFont(nullptr);
  //P.setIntensity(brightnessLevel);

  // Show Welcome Loop
  if (!wifiConnected) {
  // Show Welcome Loop only if WiFi NOT connected
  unsigned long startTime = millis();
  while (millis() - startTime < 21000) { 
    showPreConnectionMessage();
    P.displayAnimate();
  }
}


  // 🕒 Time Config
  Serial.println("🕒 Configuring time...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Serve Custom HTML File
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  if (SPIFFS.exists("/BLOKDBIT_Wifi_Portal.html.gz")) {
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/BLOKDBIT_Wifi_Portal.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip"); // Inform the browser that the file is GZIP-compressed
    request->send(response);
  } else {
    request->send(404, "text/plain", "Custom HTML file not found");
  }
});


  // 📝 Handle Save Form Submission
server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();
    String city = request->getParam("city", true)->value();
    String timezone = request->getParam("timezone", true)->value();

    Serial.println("✅ Saving WiFi Settings:");
    Serial.println("SSID: " + ssid);
    Serial.println("Password: " + password);
    Serial.println("City: " + city);
    Serial.println("Timezone: " + timezone);

    prefs.begin("blokdbit", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.putString("city", city);
    prefs.putInt("timezone", timezone.toInt());
    prefs.end();
    Serial.println("✅ Settings saved to NVS!");


    // ✅ SEND HTTP 200 RESPONSE FIRST
    request->send(200, "text/plain", "Settings saved! Rebooting...");

    delay(1000); // small delay to let browser receive the message
    ESP.restart();
  } else {
    Serial.println("❌ Missing parameters in form submission!");
    request->send(400, "text/plain", "Missing parameters");
  }
});



  
  // Captive Portal Redirect
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });


  // Start Web Server
  Serial.println("🌐 Starting Async Web Server...");
  delay(2000);  // 🕒 Let WiFi fully stabilize first
  server.begin();
  Serial.println("🌍 Async Web server started");
  delay(2000);  // 🕒 Let server stabilize after starting



   // Initial API Fetch
  Serial.println("🌍 Fetching initial data...");
  fetchBitcoinData();
  fetchBlockHeight();
  fetchFeeRate();
  fetchMinerName();
  fetchTime();
  fetchLatLonFromCity();
  fetchWeather();
  lastFetchTime = millis();
  Serial.println("✅ Initial data fetch complete!");


  lastWeatherUpdate = millis() - WEATHER_UPDATE_INTERVAL;   // ⬅️ force weather update ready immediately
  



  // Show Connection Success Message
  Serial.println("📢 Displaying WiFi connected message on Matrix...");
  P.displayZoneText(ZONE_UPPER, "WIFI", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
  P.displayZoneText(ZONE_LOWER, "CONNECTED", PA_CENTER, 0, 2000, PA_FADE, PA_FADE);
  delay(2000);
  //Going to see if commenting these out will stop dimming of bottom row
  //P.displayClear();
  //P.synchZoneStart();

  // 👇  Manually trigger first animation cycle!
  cycle = 0;   // Start at first data set
  lastApiCall = millis() - FETCH_INTERVAL; // Force immediate fetch
  lastWeatherUpdate = millis() - WEATHER_UPDATE_INTERVAL; // Force weather update soon
  lastNTPUpdate = millis() - NTP_UPDATE_INTERVAL;         // Force NTP update soon

} 


void loop() {
  dnsServer.processNextRequest(); // 🚀 Handle captive portal DNS magic

  unsigned long currentMillis = millis();

  // ⏰ Periodically update the time (every 30s)
  static unsigned long lastTimeUpdate = 0;
  if (currentMillis - lastTimeUpdate >= 30000) {
    fetchTime(); // 🕒 Update time every 30 seconds
    lastTimeUpdate = currentMillis;
  }

  // 🌦️ Refresh weather every 30 minutes
  if (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
    Serial.println("🔁 Refreshing weather info...");
    fetchWeather();
    lastWeatherUpdate = currentMillis;
  }

  // 🚀 Periodically fetch other data
  if (currentMillis - lastApiCall > FETCH_INTERVAL) {
    Serial.println("🔄 Fetching updated data...");
    switch (fetchCycle) {
      case 0:
        fetchBitcoinData();
        break;
      case 1:
        fetchBlockHeight();
        break;
      case 2:
        fetchFeeRate();
        break;
      case 3:
        fetchMinerName();
        break;
      // ⛔ Removed time fetch here – it's now handled on its own timer
    }
    fetchCycle = (fetchCycle + 1) % 4;  // 0–3 only (we moved time out)
    lastApiCall = currentMillis;
    Serial.println("✅ Data fetch cycle complete.");
  }

  // 🖥️ Rotate screens
  if (P.displayAnimate()) {
    Serial.print("🖥️ Displaying screen: ");
    Serial.println(displayCycle);

    switch (displayCycle) {
      case 0:
        P.displayZoneText(ZONE_UPPER, "BLOCK", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, blockText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 1:
        P.displayZoneText(ZONE_UPPER, "MINED BY", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, minerName.c_str(), PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 2:
        P.displayZoneText(ZONE_UPPER, "MOSCOW TIME", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, satsText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 3:
        P.displayZoneText(ZONE_UPPER, "USD PRICE", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, btcText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 4:
        P.displayZoneText(ZONE_UPPER, "FEE RATE", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, feeText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 5:
        P.displayZoneText(ZONE_UPPER, "TIME", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, timeText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 6:
        P.displayZoneText(ZONE_UPPER, dayText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, dateText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      case 7: {
        static char tempDisplay[16];
        snprintf(tempDisplay, sizeof(tempDisplay), (temperature >= 0) ? "+%dC" : "%dC", temperature);
        P.displayZoneText(ZONE_UPPER, savedCity.c_str(), PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, tempDisplay, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      }
      case 8: {
        static char condDisplay[32];
        String cond = weatherCondition;
        cond.replace("_", " ");
        cond.toLowerCase();
        cond[0] = toupper(cond[0]);
        strncpy(condDisplay, cond.c_str(), sizeof(condDisplay));
        condDisplay[sizeof(condDisplay) - 1] = '\0';
        P.displayZoneText(ZONE_UPPER, "Weather is", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(ZONE_LOWER, condDisplay, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      }
    }

    Serial.println("✅ Screen update complete.");
    displayCycle = (displayCycle + 1) % 9;
    P.displayClear();
    P.synchZoneStart();
  }
}
