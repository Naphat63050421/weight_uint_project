#include "Arduino.h"
// #include <WiFiClientSecure.h>
// #include <SPIFFS.h>
// #include "at_client.h"
#include "BluetoothSerial.h"
//#include "constants.h"
#include "M5Stack.h"
#include "M5GFX.h"
#include "HX711.h"

#define LOADCELL_DOUT_PIN 33
#define LOADCELL_SCK_PIN 32
#define readingsToAverage 70

const char *pin = "1234";
String device_name = "ESP32-BT-load-cell";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

HX711 scale;
M5GFX display;
M5Canvas canvas(&display);
BluetoothSerial SerialBT;

void setup()
{
  Serial.begin(115200);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  //อาจจะต้องเอาโชว์ขึ้นจอให้ USER  รู้ว่า bluetooth เชื่อมต่อหรือยัง 
  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.println("Using PIN");
  #endif
  M5.begin();
  M5.Power.begin();
  display.begin();
  canvas.setColorDepth(1);
  canvas.createSprite(display.width(), display.height());
  canvas.setTextDatum(MC_DATUM);
  canvas.setPaletteColor(1, YELLOW);

  canvas.drawString("Calibration sensor....", 160, 80);
  canvas.pushSprite(0, 0);

  //config Freq
  rtc_cpu_freq_config_t config;
  Serial.println("Initializing the scale");
  rtc_clk_cpu_freq_get_config(&config);
  rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &config);
  rtc_clk_cpu_freq_set_config_fast(&config);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(11.2); // factor scale จะต้องเอาเข้าสูตรคำนวนก่อนถึงจะได้ค่าออกมา
  scale.tare(); //set scale ให้เป็น 0
}

void loop()
{
  M5.update();
  float sum_weight = 0;
  float weight = 0;
  float reading_averg[readingsToAverage];
  canvas.fillSprite(BLACK); // back ground สีดำ
  canvas.setTextSize(1);  //text size 1
  canvas.drawString("Connect the Weight Unit to PortB(G33,G32)", 160, 40); // text
  canvas.drawString("Click Btn A for Calibration", 160, 80); // text

  weight = scale.get_units();
  weight = weight / 1000;
  //เมื่อมีค่าน้ำหนักจะเริ่มการคำนวน
  if (weight > 0.50) // +- load cell เวลาขยับอาจจะเกิดค่าน้ำหนักทำให้เกิดการคำนวนได้ 
  {
    canvas.setTextSize(3);
    canvas.drawString("Calculated..", 160, 150);
    //ตอนคำนวนอาจจะมีstatusบอกว่าอีกกี่วินาทีจะได้น้ำหนักตอนคำนวน (แต่ยังไม่ได้ทำ)
    canvas.pushSprite(0, 0);
    for (int i = 0; i < readingsToAverage; i++) //for loop คำนวนค่าน้ำหนัก AVERG  70 ตัว loop time เวลาประมาณ 5 วินาที ของ 70 ตัว 
                                                //(แต่จะเปลี่ยนเป็น Timer function "millis()" จับเวลาแทน)
    {
      reading_averg[i] = scale.get_units();
      sum_weight += reading_averg[i];
    }
    sum_weight = sum_weight / readingsToAverage; //คำนวน Averange 
    sum_weight = sum_weight / 1000; //แปลงหน่วย
    if (sum_weight < 0.00) // เผื่อถ้าค่าข้อมูลติดลบตอนยังไม่เจอน้ำหนักจะให้ Fillค่า 0.0 kg เอาไว้ (อาจจะไม่มีก็ได้)
    {
      sum_weight = 0.00;
    }
    canvas.drawString("Weight " + String(sum_weight) + "Kg", 160, 150);
    canvas.pushSprite(0, 0);
    //ส่งค่าให้ Bluetooth
    SerialBT.print("Weight: "); 
    SerialBT.print(sum_weight); 
    SerialBT.println("Kg"); 

    delay(5000); //ให้ดูผลการชั่งน้ำหนัก 5 วินาที
    //ตอนได้ผลลัพของการชั่งน้ำหนักจะให้มี สีบอกว่าอันนี้คือค่าผลลัพธ์ (แต่ยังไม่ได้ทำ)
  }
  canvas.setTextSize(3); // config text size
  canvas.drawString(">>Intput Weight<<", 160, 150); // text
  Serial.println(weight); // text
  canvas.pushSprite(0, 0); // pushSprite ออกหน้าจอ
}