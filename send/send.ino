#include <WiFi.h>       //import library sampai line8
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

#include "DHT.h"

#define DHTPIN 4        //dht pin 4/di GPIO 4
#define DHTTYPE DHT22   //tipe dht
String datatelur="init";
int val = 0;  
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

const char* ssid = "Polinema_hotspot_UKM";  //masuk ke hotspot mana
const char* password = "polinemaUKM";

String serverName = "47.254.244.187";   // server backend nya,mau dimasukkan kmn datanya
//String serverName = "example.com";   // OR REPLACE WITH YOUR DOMAIN NAME

String serverPath = "/file-upload";     //path dr backend,mengarah ke backend. kl upload gambar kesitu /file-upload

const int serverPort = 5000;

WiFiClient client;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const int timerInterval = 30000;    // time between each HTTP POST image
unsigned long previousMillis = 0;   // last time image was sent

DHT dht(DHTPIN, DHTTYPE);
int state = LOW;  
int mistmaker = 14;
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  //step utk menghubungkan ke wifi smpai line 73 dgn masukkan ssid&pass td
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

//    digitalWrite(0,LOW);
  
  pinMode(2, INPUT);
  timeClient.begin();
  timeClient.setTimeOffset(3600);
  dht.begin();
  delay(5000);
  sendPhoto();     //(didlm voidsetup)bakal ngeofoto pas awal dinyalain/ngecapture foto baru
  pinMode(0, OUTPUT);
  pinMode(mistmaker, OUTPUT);
}

void loop() {      //main programnya, ngambil data2nya dr sensor2
  float h = dht.readHumidity();    //membaca kelembaban
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();   //membaca suhu
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
   Serial.println(F("Failed to read from DHT sensor!"));
   return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);
  
  String fieldwaktu = getTimeNTP();   //ngambil waktu
  String fieldtelur = functelur();    //ngambil sttstelur detected/stopped
  if(fieldtelur=="Detected"){
    sendPhoto();
  }
//  
  updatesensor("http://47.254.244.187/updatesens.php?hum="+String(h)+"&temp="+String(t)+"&telur="+fieldtelur);  //update datanya utk dikirim ke server
  Serial.println(fieldwaktu);
  if (fieldwaktu == "14:00:00" ||fieldwaktu == "14:01:00" ||fieldwaktu == "14:02:00" ||fieldwaktu == "14:03:00"){
    sendPhoto(); 
  }
  if (t>40){              //lebih besar dr 40 lampunya mati, knp kebalik krn relay dipasang aktif low
    digitalWrite(0,HIGH); //bakal mati kl diksh input high
  }else if(t<36){         //lebih kecil dr 36 lampunya nyala
    digitalWrite(0,LOW);  //bakal nyala kl diksh input low
  }else{
    //nothing
  }

  if (h>65){              //lebih besar dr 40 lampunya mati, knp kebalik krn relay dipasang aktif low
    digitalWrite(mistmaker,HIGH); //bakal mati kl diksh input high
  }else if(h<55){         //lebih kecil dr 36 lampunya nyala
    digitalWrite(mistmaker ,LOW);  //bakal nyala kl diksh input low
  }else{
    //nothing
  }
  delay(2500);
}