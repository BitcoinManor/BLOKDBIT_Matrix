// 🚀 BlokdBit Matrix - Adding Date & Time Display
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Font_Data.h"
#include "time.h"  // ⏳ Time library for NTP

// 🌐 WiFi Credentials
const char* wifiSSID = "[ in-juh-noo-i-tee ]"; //FILL in your wifiSSID between the "" eg. "myhouse69"
const char* wifiPASS = "notachance";  //FILL in your wifiPASS between the ""
//const char* wifiSSID = "[SM-S918W0853]"; //For HotSpot
//const char* wifiPASS = "MySamsungPhone!!!";  //For Hot Spot

// 🌍 API Endpoints
const char* BTC_API = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";
const char* BLOCK_API = "https://blockchain.info/q/getblockcount";
const char* FEES_API = "https://mempool.space/api/v1/fees/recommended";
const char* MEMPOOL_BLOCKS_API = "https://mempool.space/api/blocks";
const char* BLOCKSTREAM_TX_API_BASE = "https://blockstream.info/api/block/";


//Fetching Miner
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
    String byteString = hex.substring(i, i + 2);
    char c = (char) strtol(byteString.c_str(), nullptr, 16);
    if (isPrintable(c)) ascii += c;
  }
  return ascii;
}

String identifyMiner(String scriptSig) {
  scriptSig.toLowerCase();
  for (const auto& tag : knownTags) {
    if (scriptSig.indexOf(tag.tag) != -1) return tag.name;
  }
  if (scriptSig.length() > 0) {
    int maxLen = 24;
    String preview = scriptSig.substring(0, min((int)scriptSig.length(), maxLen));
    return "Unknown (" + preview + ")";
  }
  return "Unknown";
}





// ⏳ NTP Server for Time Sync (Mountain Standard Time)
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = -7 * 3600;   // MST (UTC -7:00)
int daylightOffset_sec = 3600;        // Adds 1 hour for MDT during daylight savings

// 🔢 Global Variables for Data
int btcPrice = 0, blockHeight = 0, feeRate = 0, satsPerDollar = 0;
char btcText[16], blockHeightText[16], feeText[16], satsText[16], timeText[16], dateText[16];

// ⚙️ Hardware Setup
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_ZONES 2
#define ZONE_SIZE 8
#define MAX_DEVICES (MAX_ZONES * ZONE_SIZE)
#define SCROLL_SPEED 30
#define FETCH_INTERVAL 120000  // 2 minutes

#define ZONE_UPPER 1
#define ZONE_LOWER 0
#define CLK_PIN 18
#define DATA_PIN 23
#define CS_PIN 5

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
unsigned long lastFetchTime = 0;
int brightnessLevel = 3;  // 0 = dimmest, 15 = brightest




// 🎯 Messages for Display
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
char *msg[] = {
  "BlokdBit Matrix!",
  "Bitcoin Price ",
  //"Sats Per Dollar",
  "Moscow   Time ",
  "Block   Height",
  "Mined By Minor ",
  "Fee Rate  ",
  "Current Time",
  "Current Date"
};

// 📡 Fetch Bitcoin Price
void fetchBitcoinData() {
    Serial.println("🔄 Fetching BTC price...");
    HTTPClient http;
    http.begin(BTC_API);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        btcPrice = doc["bitcoin"]["usd"];
        satsPerDollar = 100000000 / btcPrice;
        Serial.printf("✅ BTC Price: $%d\n", btcPrice);
        Serial.printf("✅ Sats Per Dollar: %d\n", satsPerDollar);
        sprintf(btcText, "$%d", btcPrice);
        sprintf(satsText, "%d Sats/$", satsPerDollar);
    } else {
        Serial.println("❌ Error fetching BTC price");
        strcpy(btcText, "Loading...");
        strcpy(satsText, "Loading...");
    }
    http.end();
}

// 📡 Fetch Block Height
void fetchBlockHeight() {
    Serial.println("🔄 Fetching Block Height...");
    HTTPClient http;
    http.begin(BLOCK_API);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        blockHeight = http.getString().toInt();
        Serial.printf("✅ Block Height: %d\n", blockHeight);
        sprintf(blockHeightText, "%d", blockHeight);
    } else {
        Serial.println("❌ Error fetching Block Height!");
        strcpy(blockHeightText, "Loading...");
    }
    http.end();
}

//  FETCH MINER
void fetchMinerName() {
  HTTPClient http;
  Serial.println("🔄 Fetching latest block hash...");
  http.begin(MEMPOOL_BLOCKS_API);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    String blockHash = doc[0]["id"];
    http.end();

    Serial.printf("✅ Latest Block Hash: %s\n", blockHash.c_str());
    String txEndpoint = BLOCKSTREAM_TX_API_BASE + blockHash + "/txs";
    http.begin(txEndpoint);
    httpCode = http.GET();
    if (httpCode == 200) {
      String txPayload = http.getString();
      DynamicJsonDocument txDoc(8192);
      deserializeJson(txDoc, txPayload);
      String scriptSig = txDoc[0]["vin"][0]["scriptsig"].as<String>();
      String asciiTag = hexToAscii(scriptSig);
      minerName = identifyMiner(asciiTag);
      Serial.printf("✅ Mined By: %s\n", minerName.c_str());
    } else {
      Serial.println("❌ Failed to fetch coinbase TX");
      minerName = "Loading...";
    }
  } else {
    Serial.println("❌ Failed to fetch block hash");
    minerName = "Loading...";
  }
  http.end();
}




// 📡 Fetch Fee Rate
void fetchFeeRate() {
    Serial.println("🔄 Fetching Fee Rate...");
    HTTPClient http;
    http.begin(FEES_API);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        feeRate = doc["fastestFee"];
        Serial.printf("✅ Fee Rate: %d sat/vB\n", feeRate);
        sprintf(feeText, "%d sat/vB", feeRate);
    } else {
        Serial.println("❌ Error fetching Fee Rate!");
        strcpy(feeText, "Loading...");
    }
    http.end();
}

// ⏳ Fetch & Format Time
void fetchTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("❌ Failed to get time!");
        strcpy(timeText, "Loading...");
        strcpy(dateText, "Loading...");
        return;
    }
    
    // ⏰ Format Time: 12:30a (Shortened AM/PM)
    strftime(timeText, sizeof(timeText), "%I:%M%p", &timeinfo);
    if (timeText[4] == 'M') timeText[4] = '\0';  // Remove space before AM/PM

    /// 📅 Format Date: "Mar 12" (Month + Day, properly spaced)
    strftime(dateText, sizeof(dateText), "%b %d", &timeinfo);
        Serial.printf("✅ Current Time: %s\n", timeText);
        Serial.printf("✅ Current Date: %s\n", dateText);
}

void setup() {
    Serial.begin(115200);
    Serial.println("🔵 BlokdBit Matrix - Starting...");

    // 🌐 Connect to WiFi
    WiFi.begin(wifiSSID, wifiPASS);
    Serial.print("🌐 Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");

    // ⏳ Initialize Time Sync
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    fetchTime();

    // 📡 Fetch Initial Data
    fetchBitcoinData();
    fetchBlockHeight();
    fetchMinerName();
    fetchFeeRate();
    lastFetchTime = millis();

    // 🔠 Setup Matrix Display
    P.begin(MAX_ZONES);
    P.setIntensity(brightnessLevel);  // 💡 Brightness set via variable 
    P.setZone(ZONE_LOWER, 0, ZONE_SIZE - 1);
    P.setFont(ZONE_LOWER, BigFontLower);
    P.setZone(ZONE_UPPER, ZONE_SIZE, MAX_DEVICES - 1);
    P.setFont(ZONE_UPPER, BigFontUpper);
    P.setCharSpacing(P.getCharSpacing() * 2);
}

void loop() {
    static uint8_t cycle = 0;

    // ⏳ Fetch New Data Every 2 Minutes
    if (millis() - lastFetchTime > FETCH_INTERVAL) {
        fetchBitcoinData();
        fetchBlockHeight();
        fetchMinerName();
        fetchFeeRate();
        fetchTime();
        lastFetchTime = millis();
    }

    // 🔄 Run Display Animation
    P.displayAnimate();

    // ✅ Ensure Display Cycles Through Each Data Point
if (P.getZoneStatus(ZONE_LOWER) && P.getZoneStatus(ZONE_UPPER)) {
    switch (cycle) {
        case 0:  // BlokdBit Matrix (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: BlokdBit Matrix!");
            P.displayZoneText(ZONE_LOWER, msg[0], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[0], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 1:  // Bitcoin Price Label (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: Bitcoin Price ");
            P.displayZoneText(ZONE_LOWER, msg[1], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[1], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 2:  // Bitcoin Price Value (scrolls continuously)
            Serial.printf("🔄 Updating Display: 💰 %s\n", btcText);
            P.displayZoneText(ZONE_LOWER, btcText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, btcText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 3:  // Sats Per Dollar Label (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: Sats Per Dollar");
            P.displayZoneText(ZONE_LOWER, msg[2], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[2], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 4:  // Sats Per Dollar Value (scrolls continuously)
            Serial.printf("🔄 Updating Display: Sats/$ %s\n", satsText);
            P.displayZoneText(ZONE_LOWER, satsText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, satsText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 5:  // Block Height Label (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: Block Height");
            P.displayZoneText(ZONE_LOWER, msg[3], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[3], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 6:  // Block Height Value (scrolls continuously)
            Serial.printf("🔄 Updating Display: %d\n", blockHeight);
            P.displayZoneText(ZONE_LOWER, blockHeightText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, blockHeightText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 7:  // Block Solved By Miner (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: Miner");
            P.displayZoneText(ZONE_LOWER, msg[4], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[4], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;
            

        case 8: // Miners name
            Serial.printf("🔄 Updating Display: Solved By %s\n", minerName.c_str());
            P.displayZoneText(ZONE_LOWER, minerName.c_str(), PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, minerName.c_str(), PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;          

        case 9:  // Fee Rate Label (scrolls + pauses centered)
            Serial.println("🔄 Updating Display: Fee Rate (sats/vB)");
            P.displayZoneText(ZONE_LOWER, msg[5], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, msg[5], PA_CENTER, SCROLL_SPEED, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;

        case 10:  // Fee Rate Value (scrolls continuously)
            Serial.printf("🔄 Updating Display: Fee Rate %s\n", feeText);
            P.displayZoneText(ZONE_LOWER, feeText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            P.displayZoneText(ZONE_UPPER, feeText, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
            break;


        case 11:  // 📅 Date (Pause Centered)
            Serial.printf("🔄 Updating Display: 📅 %s\n", dateText);
            P.displayZoneText(ZONE_LOWER, dateText, PA_CENTER, SCROLL_SPEED, 5000, PA_PRINT, PA_NO_EFFECT);
            P.displayZoneText(ZONE_UPPER, dateText, PA_CENTER, SCROLL_SPEED, 5000, PA_PRINT, PA_NO_EFFECT);
            break;

    
        case 12:  // ⏰ Time (Centered, No Scroll, 5s Pause)
                Serial.printf("🔄 Updating Display: ⏰ %s\n", timeText);
                P.displayZoneText(ZONE_LOWER, timeText, PA_CENTER, SCROLL_SPEED, 5000, PA_PRINT, PA_NO_EFFECT);
                P.displayZoneText(ZONE_UPPER, timeText, PA_CENTER, SCROLL_SPEED, 5000, PA_PRINT, PA_NO_EFFECT);
                break;

    }

    // 🔄 Cycle through messages
    cycle = (cycle + 1) % 13;  // Now cycles through all 13 steps
    P.displayClear();
    P.synchZoneStart();
  }
}
