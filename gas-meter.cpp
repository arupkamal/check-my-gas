#include <WiFi.h>
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60 /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

#include "camera_pins.h"


WiFiClient client;

const char * ssid = "Arup.WiFi";
const char * password = "000";


String serverName = "us-central1-xxx.cloudfunctions.net";
String serverPath = "/xxx";
const int serverPort = 80;

#define LED_BUILTIN 4
#define LED_EXT 12

const unsigned long timerInterval = 60000; // time between each HTTP POST image
unsigned long previousMillis = 0; // last time image was sent

void onLED(int delayMilSec) {
    digitalWrite(LED_EXT, HIGH);
    delay(delayMilSec);  
}

void offLED(int delayMilSec) {
    digitalWrite(LED_EXT, LOW);
    delay(delayMilSec);  
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    pinMode(12, OUTPUT);

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

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

    config.frame_size = FRAMESIZE_UXGA;
    //config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;

    config.fb_count = 2;
    //config.fb_count = 1;

    // camera init
    esp_err_t err = esp_camera_init( & config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t * s = esp_camera_sensor_get();
    s -> set_exposure_ctrl(s, 1);
    s -> set_ae_level(s, -2);
    s -> set_whitebal(s, 1);
    s -> set_wb_mode(s, 2);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        onLED(350);
        offLED(150);
        Serial.print(".");
    }

    onLED(0);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("Camera Ready! IP: ");
    Serial.println(WiFi.localIP());
    sendPhoto();
}

void loop() {
    /*
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= timerInterval) {
      sendPhoto();
      previousMillis = currentMillis;
    }
    */
}

String sendPhoto() {
    String getAll;
    String getBody;

    camera_fb_t * fb = NULL;
    onLED(900);
    
    fb = esp_camera_fb_get();
    delay(500);
    
    offLED(0);
    
    if (!fb) {
        Serial.println("Camera capture failed");
        delay(1000);
        ESP.restart();
    }

    Serial.println("Connecting to server: " + serverName);

    if (client.connect(serverName.c_str(), serverPort)) {
        Serial.println("Connection successful!");
        String head = "--AhmedArupKamal\r\nContent-Disposition: form-data; name=\"img\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
        String tail = "\r\n--AhmedArupKamal--\r\n";

        uint32_t imageLen = fb -> len;
        uint32_t extraLen = head.length() + tail.length();
        uint32_t totalLen = imageLen + extraLen;

        client.println("POST " + serverPath + " HTTP/1.1");
        client.println("Host: " + serverName);
        client.println("Content-Length: " + String(totalLen));
        client.println("Content-Type: multipart/form-data; boundary=AhmedArupKamal");
        client.println();
        client.print(head);

        uint8_t * fbBuf = fb -> buf;
        size_t fbLen = fb -> len;
        for (size_t n = 0; n < fbLen; n = n + 1024) {
            if (n + 1024 < fbLen) {
                client.write(fbBuf, 1024);
                fbBuf += 1024;
            } else if (fbLen % 1024 > 0) {
                size_t remainder = fbLen % 1024;
                client.write(fbBuf, remainder);
            }
        }
        client.print(tail);

        esp_camera_fb_return(fb);

        int timoutTimer = 10000;
        long startTimer = millis();
        boolean state = false;

        while ((startTimer + timoutTimer) > millis()) {
            Serial.print(".");
            delay(100);
            while (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    if (getAll.length() == 0) {
                        state = true;
                    }
                    getAll = "";
                } else if (c != '\r') {
                    getAll += String(c);
                }
                if (state == true) {
                    getBody += String(c);
                }
                startTimer = millis();
            }
            if (getBody.length() > 0) {
                break;
            }
        }
        Serial.println();
        client.stop();

        Serial.println(getBody);
    } else {

        onLED(350);offLED(350);
        onLED(350);offLED(350);
        onLED(350);offLED(350);
        onLED(350);offLED(350);
        onLED(350);offLED(350);
        onLED(350);offLED(350);
        
        getBody = "Connection to " + serverName + " failed.";
        Serial.println(getBody);
    }

    Serial.println("Going to sleep now...");
        onLED(400);offLED(350);
        onLED(400);offLED(350);

    Serial.flush();
    esp_deep_sleep_start();

    return getBody;
}
