Tested with:
1. Arduino IDE 1.8.10 on Windows 10
2. Sonoff TH10
3. Sonoff AM2301 sensor

Pre-requisite:
1. You must have an account with TruIO platform and have created a device.

How to use:
1. Download this repo then copy and paste all the folders of "libraries" into your local Arduino libraries folder (usually located at C:\Users\<user>\Documents\Arduino\libraries)
2. Open the "truio-Sonoff-TH10-demo.ino" and insert the relevant details into:

			const char* ssid = "Insert your WiFi SSID";
			const char* password = "Insert your WiFi password";
			#define TRUIO_USER "Insert your TruIO Email address"
			#define TRUIO_PW "Insert your TruIO API Key"
			#define DEVICE_KEY "Insert your Device Key"

3. Compile and upload your sketch to ESP8266.
4. Open serial monitor to see the info.
5. Log in platform.truio.live and view your device tags data.
