# GeophoneDuino
This repository contains everything related to our Arduino ESP8266 based Geophone sensing node (although it could be used to sample any other sensor).

## Hardware
The code assumes the following hardware components are stacked on top of each other:
 * Our custom protoshield with an MCP3201 (SPI 12bit ADC), an _optional_ opamp to amplify the geophone signal and the geophone sensor
 * WeMos D1 mini (the Arduino ESP8266 itself)
 * (_Optional_) Battery shield

<p align="center"><img src="images/HardwareExample.jpg" alt="Sample GeophoneDuino sensing node" style="width: 200px;"></p>

## Instructions

### Cloning the repo
For the scripts to work, it is important to **clone** the repo instead of simply downloading it.
 1. Locate your Arduino home folder (usually under `<home_dir>/Documents/Arduino` or `<home_dir>/Arduino`)
 2. Open a terminal and navigate to that folder. _E.g._ `cd ~/Documents/Arduino`
 3. Clone the repo:
 ```sh
 git clone https://github.com/CarlosRDomin/GeophoneDuino.git
 ```

### Installing the drivers
The Arduino ESP8266 uses a CH34x chip instead of the more common FTDI. Therefore, the right drivers need to be installed in order for the board to show up when you plug it in through USB. Driver file (for Mac) is available under `Datasheets and useful info/CH34x_Install_V1.4.pkg`. Simply double click and follow the on-screen instructions (might need to reboot).

#### Filesystem plugin
 The filesystem plugin allows you to save files in the Esp8266's Flash memory (so we can host webpages, etc.). The plugin is [here](https://github.com/esp8266/arduino-esp8266fs-plugin). Steps to install:
  1. Download the latest `zip` file (no need to download the source) from the [Releases tab](https://github.com/esp8266/arduino-esp8266fs-plugin/releases) (eg: latest version as of Feb 19th 2018 is 0.3.0)
  2. Locate your Arduino home folder (usually under `<home_dir>/Documents/Arduino` or `<home_dir>/Arduino`), which is where you should have cloned this repository
  3. If it doesn't exist, create a folder called `tools` inside the `Arduino` folder
  4. Extract the `zip` file you downloaded, so now there should be a file named `...Arduino/tools/ESP8266FS/tool/esp8266fs.jar`
  5. Restart the Arduino IDE

#### Installing 3rd-party libraries
 In order to make it easier to install all libraries and the Esp8266 core, I added a Python script under the `submodules` folder (which uses `git submodule` to fetch the right version of each library). So the steps simply are:
  1. Open a Terminal console and navigate to the `submodules` folder in this repo
  2. Execute this command:
  ```sh
  python symlinks.py -i
  ```
  NOTE: this would install the libraries and the board into the **default** Arduino folder (_eg_ for Mac: `<home_dir>/Documents/Arduino`). If your `Arduino` folder is somewhere else, specify such path as an argument to the Python script like this: `python symlinks.py -i -p <PATH>`

### How to flash the firmware?
 1. Open the file `GeophoneDuino.ino` in the Arduino IDE.
 2. Plug in the ESP8266 through USB, select the right port on `Tools > Port` (_eg_: `\dev\cu.wchusbserial14440`) and configure the board as:
   - Board: WeMos D1 R2 & mini
   - Flash size: 4M (1M SPIFFS)
   - Debug port: Disabled
   - Debug level: None
   - lwIP variant: v2 higher bandwidth
   - CPU frequency: 160 MHz
   - Upload speed: 921600
   - Erase Flash: only Sketch
 3. Copy SPIFFS (filesystem) files by clicking on `Tools > ESP8266 Sketch Data Upload`. This step will allow you to:
       - Connect to the Arduino's own hotspot (which is automatically created whenever it is unable to connect to the default WiFi network), perform a network scan and [configure which network it should connect to](#how-to-configure-which-network-to-join).
       - [Visualize sensor data in real-time wirelessly](#how-to-see-real-time-sensor-data) (doesn't even need an Internet connection)
 4. Upload the sketch (`Sketch > Upload`)

### How to configure which network to join?
In order to wirelessly interact with the Arduino, both devices need to be in the same network. One could change these settings in the code and reflash the firmware every time the testing/deployment environment changes, but sometimes this isn't ideal.

Instead, the Arduino will create its own hotspot if it is unable to connect to the preconfigured network within 10s, so that we can connect to it and update its settings.
Note that by default, the network name will be `Geophone_ABCDEF` (where `ABCDEF` will be replaced by its hex serial number) and password will be `geophone`.

Once connected, its IP should be `192.168.0.1`. We can access its wireless settings by opening a web browser and navigating to `192.168.0.1/WiFi`. The webpage will automatically trigger a network scan and all available networks will be displayed within a few seconds. Then, we can configure which network to join, the password, desired IP, etc. and click Connect.
Note that, upon successful network connection, the hotspot will be turned off and therefore our laptop/device will also need to connect to a different network.

### How to see real-time sensor data?
Once both the Arduino and our laptop/phone/device are connected to the same network, we can simply open a web browser and navigate to `<Arduino IP>/` or, equivalently, `<Arduino IP>/index.html` and a live stream of data will show up. Click `Close socket` to stop streaming or `Save data` to download a `csv` file with all data recorded.

### How to collect (store to file) data?
The Python script to collect data relies on a WebSocket implementation that needs to be installed before the first use. Simply open a terminal and execute
```sh
pip install ws4py
```

Then, collect data by running the script:
```sh
python data_collection.py
```

(To stop collecting data, just press `Ctrl + C`)
