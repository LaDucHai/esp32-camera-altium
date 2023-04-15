#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include <driver/i2s.h>

//#include "SoundData.h";
//#include "XT_DAC_Audio.h";
//XT_Wav_Class ForceWithYou(Force);
//XT_DAC_Audio_Class DacAudio(25,0);  

const char* ssid = "TP-Link_A3DC";
const char* password = "00000000";
const char* websocket_server_host = "192.168.1.20";
const uint16_t websocket_server_port = 8888;

using namespace websockets;
WebsocketsClient client;
bool isWebSocketConnected;

void onEventsCallback(WebsocketsEvent event, String data){
  if(event == WebsocketsEvent::ConnectionOpened){
    Serial.println("Connection Opened");
    isWebSocketConnected = true;
  }else if(event == WebsocketsEvent::ConnectionClosed){
    Serial.println("Connection Closed");
    isWebSocketConnected = false;
    webSocketConnect();
  }
}

void setup() {
  isWebSocketConnected = false;
  Serial.begin(115200);
  Serial.println();

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
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 40;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  client.onEvent(onEventsCallback);
  webSocketConnect();
  //i2s_setup();
}

void i2s_setup()
{
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 40000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

  //install and start i2s driver
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  
  //init ADC pad
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_5);
  
  // enable the ADC
  i2s_adc_enable(I2S_NUM_0);
}

void webSocketConnect(){
   while(!client.connect(websocket_server_host, websocket_server_port, "/")){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Websocket Connected!");
}

void loop() {

  if(client.available()){
    client.poll();
  }
  
  if(!isWebSocketConnected) return;
  
  camera_fb_t *fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }

  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Non-JPEG data not implemented");
    return;
  }
  
  //fb->buf[12] = 0x01; //FIRST CAM
  fb->buf[12] = 0x02; //SECOND CAM

  client.sendBinary((const char*) fb->buf, fb->len);
  esp_camera_fb_return(fb);
//  static uint32_t i=0;                // simple counter to output
//  if(ForceWithYou.Completed)          // if completed playing, play again
//    DacAudio.PlayWav(&ForceWithYou);  // play the wav (pass the wav class object created at top of code
//  Serial.println(i);                  // print out the value of i
//  i++;
//  delay(1);
}
