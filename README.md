# GeophoneDuino
This repository contains everything related to our Arduino ESP8266 based Geophone sensing node (although it could be used to sample any other sensor).

## Hardware
The code assumes the following hardware components are stacked on top of each other:
 * Our custom protoshield with an MCP3201 (SPI 12bit ADC), an _optional_ opamp to amplify the geophone signal and the geophone sensor
 * WeMos D1 mini (the Arduino ESP8266 itself)
 * (_Optional_) Battery shield

<p align="center"><img src="images/HardwareExample.jpg" alt="Sample GeophoneDuino sensing node" style="width: 200px;"></p>

## Instructions

### How to flash the firmware?
 1. Open the file `GeophoneDuino.ino` in the Arduino IDE.
 2. Plug in the ESP8266 through USB, select the right port on `Tools > Port` (_eg_: `\dev\cu.wchusbserial14440`) and configure the board as:
   - Board: WeMos D1 R2 & mini
   - CPU frequency: 160 MHz
   - Flash size: 4M (3M SPIFFS)
   - Upload speed: 921600
 3. Upload the sketch (`Sketch > Upload`)
 4. Copy SPIFFS (filesystem) files by clicking on `Tools > ESP8266 Sketch Data Upload`. This step will allow you to:
   - Connect to the Arduino's own hotspot (which is automatically created whenever it is unable to connect to the default WiFi network), perform a network scan and [configure which network it should connect to](#how-to-configure-which-network-to-join).
   - [Visualize sensor data in real-time wirelessly](#how-to-see-real-time-sensor-data) (doesn't even need an Internet connection)

### How to configure which network to join?
In order to wirelessly interact with the Arduino, both devices need to be in the same network. One could change these settings in the code and reflash the firmware every time the testing/deployment environment changes, but sometimes this isn't ideal.

Instead, the Arduino will create its own hotspot if it is unable to connect to the preconfigured network within 10s, so that we can connect to it and update its settings.
Note that by default, the network name will be `Geophone_ABCDEF` (where `ABCDEF` will be replaced by its hex serial number) and password will be `geophone`.

Once connected, its IP should be `192.168.0.1`. We can access its wireless settings by opening a web browser and navigating to `192.168.0.1/WiFi`. The webpage will automatically trigger a network scan and all available networks will be displayed within a few seconds. Then, we can configure which network to join, the password, desired IP, etc. and click Connect.
Note that, upon successful network connection, the hotspot will be turned off and therefore our laptop/device will also need to connect to a different network.

### How to see real-time sensor data?
Once both the Arduino and our laptop/phone/device are connected to the same network, we can simply open a web browser and navigate to `<Arduino IP>/` or, equivalently, `<Arduino IP>/index.html` and a live stream of data will show up. Click `Close socket` to stop streaming or `Save data` to download a `csv` file with all data recorded.
