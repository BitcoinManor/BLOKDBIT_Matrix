// 🚀 BLOKDBIT Matrix: Dual-Row Static Display Version
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Font_Data.h"  // Optional, if using custom fonts
#include "time.h"

// 🌐 WiFi Credentials
const char* wifiSSID = "[ in-juh-noo-i-tee ]";
const char* wifiPASS = "notachance";

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

// Time Config
const char* ntpServer = "pool.ntp.org";
long gmtOffset_sec = -7 * 3600;
int daylightOffset_sec = 3600;

// Data Variables
int btcPrice = 0, blockHeight = 0, feeRate = 0, satsPerDollar = 0;
char btcText[16], blockText[16], feeText[16], satsText[16];

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
char timeText[16], dateText[16], dayText[16];

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
#define BUTTON_PIN 25



MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
int brightnessLevel = 6;  // 0 = dimmest, 15 = brightest
unsigned long lastFetchTime = 0;

// Fetch Functions
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
    sprintf(btcText, "$%s", formatWithCommas(btcPrice).c_str());
    sprintf(satsText, "%d sats/$", satsPerDollar);
  } else {
        Serial.println("❌ Error fetching BTC price");
        strcpy(btcText, "Loading...");
        strcpy(satsText, "Loading...");
    }
    http.end();
}

void fetchBlockHeight() {
  Serial.println("🔄 Fetching Block Height...");
  HTTPClient http;
  http.begin(BLOCK_API);
  if (http.GET() == 200) {
    blockHeight = http.getString().toInt();
    sprintf(blockText, "%d", blockHeight);
  }
  http.end();
}

void fetchFeeRate() {
  HTTPClient http;
  http.begin(FEES_API);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());
    feeRate = doc["fastestFee"];
    sprintf(feeText, "%d sat/vB", feeRate);
  }
  http.end();
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
    http.begin(BLOCKSTREAM_TX_API_BASE + blockHash + "/txs");
    if (http.GET() == 200) {
      DynamicJsonDocument txDoc(8192);
      deserializeJson(txDoc, http.getString());
      String scriptSig = txDoc[0]["vin"][0]["scriptsig"].as<String>();
      minerName = identifyMiner(hexToAscii(scriptSig));
    }
  }
  http.end();
}

void fetchTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("❌ Failed to get time!");
    strcpy(timeText, "Loading...");
    strcpy(dateText, "Loading...");
    strcpy(dayText, "Loading...");
    return;
  }

  strftime(timeText, sizeof(timeText), "%I:%M%p", &timeinfo);
  if (timeText[4] == 'M') timeText[4] = '\0';

  strftime(dateText, sizeof(dateText), "%b %d", &timeinfo);
  strftime(dayText, sizeof(dayText), "%A", &timeinfo);

  Serial.printf("✅ Time: %s\n", timeText);
  Serial.printf("✅ Date: %s\n", dateText);
  Serial.printf("✅ Day:  %s\n", dayText);
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


   // 🔠 Setup Matrix Display
  P.begin(MAX_ZONES);
  P.setIntensity(brightnessLevel);
  P.setZone(0, 0, ZONE_SIZE - 1);
  P.setZone(1, ZONE_SIZE, MAX_DEVICES - 1);
  P.setFont(nullptr);  // use default font
  P.setCharSpacing(1);



  // 📡 Fetch Initial Data
  fetchBitcoinData();
  fetchBlockHeight();
  fetchFeeRate();
  fetchMinerName();
  fetchTime();
  lastFetchTime = millis();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);  //added this for the Smash Buy Button!!!

}

void loop() {
  static uint8_t cycle = 0;

  // ⏳ Fetch New Data Every 2 Minutes
  if (millis() - lastFetchTime > FETCH_INTERVAL) {
    fetchBitcoinData(); fetchBlockHeight(); fetchFeeRate(); fetchMinerName(); fetchTime();
    lastFetchTime = millis();
  }

  if (P.displayAnimate()) {
    switch (cycle) {
      


      case 0:
        Serial.println("🔄 Updating Display: BlokdBit Matrix!");
        P.displayZoneText(1, "BLOKDBIT", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, "MATRIX", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
      
      case 1:
        Serial.println("🔄 Updating Display: Block Height");
        Serial.printf("🔄 Updating Display: %d\n", blockHeight);
        P.displayZoneText(1, "BLOCK", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, blockText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 2:  // Mined By Value
        Serial.println("🔄 Updating Display: Mined By");
        Serial.printf("🔄 Updating Display: Solved By %s\n", minerName.c_str());
        P.displayZoneText(1, "MINED BY", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, minerName.c_str(), PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 3:  // Sats Per Dollar Value
        Serial.println("🔄 Updating Display: Sats Per Dollar");
        Serial.printf("🔄 Updating Display: Sats/$ %s\n", satsText);
        P.displayZoneText(1, "MOSCOW TIME", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, satsText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 4:  // Bitcoin Price Value
        Serial.printf("🔄 Updating Display: 💰 %s\n", btcText);
        P.displayZoneText(1, "USD PRICE", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, btcText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 5:   // Fee Rate Value
        Serial.println("🔄 Updating Display: Fee Rate (sats/vB)");
        P.displayZoneText(1, "FEE RATE", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, feeText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 6:  // ⏰ Time (Centered, No Scroll, 5s Pause)
        Serial.printf("🔄 Updating Display: ⏰ %s\n", timeText);
        P.displayZoneText(1, "TIME", PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, timeText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
        
      case 7:  // 📅 Date
        Serial.printf("🔄 Updating Display: 📅 %s\n", dateText);
        P.displayZoneText(1, dayText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        P.displayZoneText(0, dateText, PA_CENTER, 0, 5000, PA_FADE, PA_FADE);
        break;
    }
    cycle = (cycle + 1) % 8;
    P.displayClear();
    P.synchZoneStart();
  }

 if (digitalRead(BUTTON_PIN) == LOW) {
  Serial.println("🚨 SMASH BUY Button Pressed!");

  // Message 1: SMASH BUY!
  P.displayClear();
  P.displayZoneText(1, "SMASH", PA_CENTER, 0, 3000, PA_FADE, PA_FADE);
  P.displayZoneText(0, "BUY!", PA_CENTER, 0, 3000, PA_FADE, PA_FADE);
  while (!P.displayAnimate()) delay(10);

   // Message 2: STACKED 
  P.displayClear();
  P.displayZoneText(1, "$100", PA_CENTER, 0, 1500, PA_FADE, PA_FADE);
  P.displayZoneText(0, "Sent To Your", PA_CENTER, 0, 1500, PA_FADE, PA_FADE);
  while (!P.displayAnimate()) delay(10);


   // Message 3: Lightning Wallet 
  P.displayClear();
  P.displayZoneText(1, "LIGHTNING", PA_CENTER, 0, 1500, PA_FADE, PA_FADE);
  P.displayZoneText(0, "WALLET", PA_CENTER, 0, 1500, PA_FADE, PA_FADE);
  while (!P.displayAnimate()) delay(10);

  
  P.displayClear();
  P.synchZoneStart();
  }

  
}
