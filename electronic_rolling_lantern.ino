/*

Project Name: Electronic Rolling Lantern

Function Description: When the lantern rolls, the lighting changes 
color according to the lantern's rolling angle and acceleration. 
Additionally, it can connect to a smartphone via Bluetooth, 
allowing users to control the light's on/off state and brightness with their phone.

Author: Carla Guo, Created on August 30, 2024

Usage Risk: The code and documentation provided in this project are 
for reference and educational purposes only. We are not responsible 
for any damages, losses, or other negative consequences that may arise 
from using this project, including but not limited to data loss, 
hardware damage, or other property damage.

*/

#include <ArduinoBLE.h>
#include <Arduino.h>
#include "LSM6DS3.h"
#include <Adafruit_NeoPixel.h>
#include "Wire.h"
#include "math.h"

BLEService ledService("180A"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("2A56", BLERead | BLEWrite);
BLEByteCharacteristic BrightCharacteristic("2A5E", BLERead | BLEWrite);

#define PIN A0
#define NUMPIXELS 60 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

float ax = 0;
float ay = 0;
float az = 0;

//Create a instance of class LSM6DS3
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A

bool flag = false;
uint8_t bright = 10;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (myIMU.begin() != 0) {
      Serial.println("Device error");
  } else {
      Serial.println("Device OK!");
  }
  pixels.begin();
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("nRF52840_Sense");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);
  ledService.addCharacteristic(BrightCharacteristic);

  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Peripheral");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      if (switchCharacteristic.written()) {
        flag = switchCharacteristic.value(); // 根据特征值设置 flag
      }
      if (BrightCharacteristic.written()) {
        bright = BrightCharacteristic.value(); // 根据特征值设置 bright
      }
      // 插入一个短暂的延迟，以避免占用过多 CPU 时间
      delay(50);
    }
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
  // 主核心处理 IMU 数据
  if (flag) {
    float ax = myIMU.readFloatAccelX();
    float ay = myIMU.readFloatAccelY();
    float az = myIMU.readFloatAccelZ();
    uint8_t r = constrain(ax * 100 + 128, 0, 255);
    uint8_t g = constrain(ay * 100 + 128, 0, 255);
    uint8_t b = constrain(az * 100 + 128, 0, 255);
    uint32_t packedRGB = (r << 16) | (g << 8) | b;

    Serial.print("ax =");
    Serial.print(ax);
    Serial.print(String("      ")+"ay =");
    Serial.print(ay);
    Serial.print(String("      ")+"az =");
    Serial.println(az);
    Serial.print("r = ");
    Serial.print(r);
    Serial.print(", g = ");
    Serial.print(g);
    Serial.print(", b = ");
    Serial.println(b);

    pixels.clear();
    pixels.fill(packedRGB, 0, 60);
    pixels.setBrightness(bright);
    pixels.show();   // 更新LED颜色
  } else {
    pixels.clear();
    pixels.show();   // 清除LED颜色
  }
}