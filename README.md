# ESP32-Cam WIFI 🎥

My aim for this project is to make a fully self-contained system allowing live video transmission as well as giving the user the ability to takes pictures that are saved to a MicroSD Card plugged directly into the device. The module works thanks to a 5V battery, a switch can be used to turn on/off the system, finally, a USB-C port makes charging very convenient.

## Hardware Required :

| **Part** | Image | Usual price |
| --- | :---: | :---: |
| **ESP32-CAM Module** | <img width="30%" src="https://ae-pic-a1.aliexpress-media.com/kf/S875c7b2345a04d6782ac048bc152614eP.jpg_960x960q75.jpg_.avif"> | 4.50€ |
| **OV2640 Camera** | <img width="30%" src="https://m.media-amazon.com/images/I/61dHzX3midL.jpg">  | 2.80€ |
| **ESP32-CAM-MB Programmer Shield** | <img width="30%" src="ESP32-CAM-MB.jpg"> | 1.60€ |
| **Battery (5V, 3.7V, …)** |  <img width="30%" src="https://m.media-amazon.com/images/I/41mCtS5yq0L.jpg">| * |
| **Charging module (TP4056)** | <img width="30%" src="https://m.media-amazon.com/images/I/81kZztRcAFL._AC_SL1500_.jpg"> | ~1€ |
| **A switch** | <img width="30%" src="https://m.media-amazon.com/images/I/61HYwLtKBRL._AC_SL1500_.jpg"> | * |
| **DC-DC power supply adapter (if your battery ≠ 5V)** | <img width="30%" src="https://www.az-delivery.de/cdn/shop/products/mt3608-dc-dc-netzteil-adapter-step-up-modul-932676.jpg?v=1679399025&width=1200"> | ~1.5€ |
| **Low capacity MicroSD card (in FAT32 format)**  | <img width="30%" src="https://wonderphone.fr/wp-content/uploads/2017/08/product7-2.jpg"> | * |

*\* depends on capacity/quality*

## Installation

Download this ZIP archive, change the credentials of the WIFI Access Point defined in the code :

```cpp
const char *ssid = "ESP32-CAM";
const char *password = "12345678";
```

Upload the code on your ESP32, using the micro-usb programmer. *If you use the Arduino IDE select “AI Thinker ESP32-CAM” as the targeted board.*

## Usage

Insert a microSD card in the slot on top of the module, then turn it on, you should now see a new WIFI available on your computer, just like this :

<p align="center">
	<img width="30%" src="AP_screenshoot.png">
</p>

Connect to it. Finally, open the HTML file with your web browser. You can take pictures or change the frame size of the stream in real time (high resolution can slow down the rate).

<p align="center">
	<img width="40%" src="web-interface.png">
</p>

## 3D printed case

<p align="center">
	<img width="40%" src="inside.jpg">
    <img width="40%" src="hood.jpg">
</p>

I provide you with the files allowing you to print a case suitable for a standard size cylindrical battery. There is also the hood to close it.

- Download the case.
- Download the hood.
