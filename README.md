#  BLOKDBIT: Open-Source Bitcoin Dashboards Built for Sovereignty

**Open Source. Web Flashable. Self-Sovereign.**

![BLOKDBIT Banner](https://github.com/BitcoinManor/BlokdBit-Matrix/raw/main/assets/blokdbit_banner.png)

Welcome to **BLOKDBIT**, a family of beautifully designed, plug-and-play Bitcoin dashboards that combine open source power with elegant hardware and blazing-fast setup. Whether you're tracking the price of Bitcoin, monitoring network fees, or running a full node—**BLOKDBIT** keeps you in sync with the signal all at a glance!!

---

## 🚀 Quick Links

- **🔌 Flash a Device Instantly:**  
  [BLOKDBIT Web Flasher →](https://bitcoinmanor.github.io/BlokdBit-Matrix/)

- **📦 Order Devices & Kits:**  
  [blokdbit.com](https://blokdbit.com)

- **🛠 Developer Resources & Code:**  
  [GitHub Repos](https://github.com/BitcoinManor)

- **🎉 Follow Us:**  
  [Twitter/X](https://x.com/blokdbit) | [Instagram](https://www.instagram.com/blokdbit/)

---

## 💡 What Is BLOKDBIT?

**BLOKDBIT** is a hardware+software project built for Bitcoiners who want:

- Real-time Bitcoin stats on a clean, dedicated display
- Lightning-fast Web Flasher setup (no coding required)
- Beautiful self-contained devices: no keyboard, no apps, no noise
- Open source control over everything you own

---

## 🌟 Product Lineup

### **BLOKDBIT Matrix**
- 16x64 LED Ticker powered by ESP32
- Displays: BTC Price, Block Height, Fee Rate, Time, Date, Sats Per Dollar
- Affordable & sleek — plug it in, flash it, done
- 🔥 Powered by `BlokdBit_Matrix_FINAL_SKETCH4Flasher.ino.nodemcu-32s.bin`

### **BLOKDBIT Spark** *(Beta)*
- 7" touch display powered by ESP32-S3
- Wi-Fi & city config via Web Flasher
- Slim UI showing Bitcoin stats with swipe navigation
- Designed for ultra-low power & reliability

### **BLOKDBIT Pulse / Edge / Infinity** *(Coming Soon)*
- Raspberry Pi-based touchscreen dashboards
- Pulse = 5", Edge = 7", Infinity = 10"
- Custom cases, advanced node integrations, Bitaxe Hub-ready

---

## 🔥 Web Flashing Made Easy

The **BLOKDBIT Web Flasher** is a zero-install, browser-based tool that:

- Connects to your ESP32 device directly via USB
- Lets you enter Wi-Fi, city, timezone settings
- Flashes the correct firmware with a single click
- Displays live flashing logs and error output

👉 **Start flashing here:** [bitcoinmanor.github.io/BLOKDBIT_WebFlasher](https://bitcoinmanor.github.io/BLOKDBIT_WebFlasher/)
---

## 💻 Tech Stack

- **Firmware:** C++ / Arduino IDE
- **Web Flasher:** Custom HTML + JavaScript (no third-party tools)
- **Hardware:** ESP32, Raspberry Pi 4, MAX7219, Waveshare displays
- **APIs Used:**
  - `mempool.space` for fees
  - `blockchain.info` for block height
  - `CoinGecko` for BTC/USD price

---

## 🛠 Getting Started (Dev Edition)

```bash
# 1. Clone the repo
https://github.com/BitcoinManor/BlokdBit_Matrix.git

# 2. Open the .ino file in Arduino IDE
BlokdBit_Matrix_FINAL_SKETCH4Flasher.ino

# 3. Install libraries:
MD_Parola
MD_MAX72xx
ArduinoJson
Preferences
WiFi
HTTPClient

# 4. Connect your ESP32
# 5. Export compiled binary (Sketch > Export Binary)
# 6. Flash using the Web Flasher (or manually)
```

---

## 🙏 Data Sources & Attribution
BLOKDBIT proudly leverages the power of open APIs to deliver real-time Bitcoin data:

💱 CoinGecko – for reliable Bitcoin price data

🧱 blockchain.info – for current block height

🚦 mempool.space – for accurate fee estimates

Huge thanks to these projects for their dedication to open data and transparency in the Bitcoin ecosystem.
We encourage supporting them by contributing, using their services respectfully, or self-hosting when possible.

Built by [Bitcoin Manor](https://bitcoinmanor.com) for the sovereign individual.

- **Inspired by Hal Finney Ticker by Ben (arcbtc)** from the original ticker.
- **Bitcoin data powered by CoinGecko & Mempool.space & blockchain.info**.
- Original Ticker Fork: `arcbtc / hal-finney-ticker`
- LED Matrix Libraries: `MajicDesigns`
- Thank you to the Bitcoin community

---

## 📜 License

MIT License — use freely, fork often, flash everything.
⚖️ 

**Powered by Bitcoin.** Not just another gadget. This is **signal**.

```
BLOKDBIT Matrix v1.1 | Bitcoin Manor
Open Source | Web Flashable | Self-Sovereign
https://github.com/BitcoinManor/BlokdBit_Matrix
```

