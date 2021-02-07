/* 

   4pin IIC引脚，正面看，从左到右依次为GND、VCC、SCL、SDA
        ESP8266  ---  OLED
        3.3V     ---  VCC
        G (GND)  ---  GND
        D1(GPIO5)---  SCL
        D2(GPIO4)---  SDA
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <TimeLib.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

//#define LED D4
#define DEBUG //是否开启debug功能

#ifdef DEBUG
#define DebugPrintln(message)    Serial.println(message)
#else
#define DebugPrintln(message)
#endif

#ifdef DEBUG
#define DebugPrint(message)    Serial.print(message)
#else
#define DebugPrint(message)
#endif

#define WEATHER_CODE_DAY_SUN "0" //晴（国内城市白天晴）
#define WEATHER_CODE_NIGHT_SUN "1" //晴（国内城市夜晚晴）
#define WEATHER_CODE_DAY_SUN1 "2" //晴（国外城市白天晴）
#define WEATHER_CODE_NIGHT_SUN2 "3" //晴（国外城市夜晚晴）
#define WEATHER_CODE_CLOUDY "4" //多云
#define WEATHER_CODE_DAY_PARTLY_CLOUDY "5" //白天晴间多云
#define WEATHER_CODE_NIGHT_PARTLY_CLOUDY "6" //夜晚晴间多云
#define WEATHER_CODE_DAY_MOSTLY_CLOUDY "7" //白天大部多云
#define WEATHER_CODE_NIGHT_MOSTLY_CLOUDY "8" //夜晚大部多云
#define WEATHER_CODE_OVERCAST "9" //阴
#define WEATHER_CODE_SHOWER "10" //阵雨
#define WEATHER_CODE_THUNDERSHOWER "11" //雷阵雨
#define WEATHER_CODE_THUNDERSHOWER_WITH_HAIL "12" //雷阵雨伴有冰雹
#define WEATHER_CODE_LIGHT_RAIN "13" //小雨
#define WEATHER_CODE_MODERATE_RAIN "14" //中雨
#define WEATHER_CODE_HEAVY_RAIN "15" //大雨
#define WEATHER_CODE_STORM "16" //暴雨
#define WEATHER_CODE_HEAVY_STORM "17" //大暴雨
#define WEATHER_CODE_SEVERE_STORM "18" //特大暴雨
#define WEATHER_CODE_ICE_RAIN "19" //冻雨
#define WEATHER_CODE_SLEET "20" //雨夹雪
#define WEATHER_CODE_SNOW_FLURRY "21" //阵雪
#define WEATHER_CODE_LIGHT_SNOW "22" //小雪
#define WEATHER_CODE_MODERATE_SNOW "23" //中雪
#define WEATHER_CODE_HEAVY_SNOW "24" //大雪
#define WEATHER_CODE_SNOW_STORM "25" //暴雪

#define SUN_DAY 0
#define SUN_NIGHT 1
#define SUN_CLOUD 2
#define CLOUD 3
#define RAIN 4
#define THUNDER 5

const char ssid[] = "xiaomi4A-7926";                       //WiFi名
const char pass[] = "13228715245LJ";

String biliuid = "22774364";
static const char ntpServerName[] = "ntp1.aliyun.com"; //NTP服务器，阿里云
const int timeZone = 8;                                //时区，北京时间为+8

WiFiClient client;
HTTPClient http;
WiFiUDP Udp;
unsigned int localPort = 8888; // 用于侦听UDP数据包的本地端口

time_t getNtpTime();
void sendNTPpacket(IPAddress& address);
void oledClockDisplay();
void sendCommand(int command, int value);
void initdisplay();

bool getJson();
bool parseJson(String json);
void parseJson1(String json);

boolean isNTPConnected = false;
//声明方法
bool autoConfig();
void smartConfig();
bool sendRequest(const char* host, const char* cityid, const char* apiKey);
bool skipResponseHeaders();
void readReponseContent(char* content, size_t maxSize);
void stopConnect();
void clrEsp8266ResponseBuffer(void);
bool parseUserData(char* content, struct UserData* userData);
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol);
void drawWeather(uint8_t symbol, int degree);

const unsigned long BAUD_RATE = 115200;// serial connection speed
const unsigned long HTTP_TIMEOUT = 5000;               // max respone time from server
const size_t MAX_CONTENT_SIZE = 500;                   // max size of the HTTP response
const char* host = "api.seniverse.com";
const char* APIKEY = "Sjabgfvn-9MuORzVx";        //API KEY
const char* city = "tianjin";
const char* language = "zh-Hans";//zh-Hans 简体中文  会显示乱码

int flag = HIGH;//默认当前灭灯
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


char response[MAX_CONTENT_SIZE];
char endOfHeaders[] = "\r\n\r\n";

long lastTime = 0;
// 请求服务间隔
long Delay = 10;

String response1;
String responseview;
int follower = 0;
int view = 0;
const int slaveSelect = 5;
const int scanLimit = 7;

int times = 0;

// 我们要从此网页中提取的数据的类型
struct UserData {
  char city[16];//城市名称
  char weather_code[4];//天气现象code（多云...）
  char temp[5];//温度
};

const unsigned char xing[] U8X8_PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x80, 0x00, 0x88, 0x00,
  0xF8, 0x1F, 0x84, 0x00, 0x82, 0x00, 0xF8, 0x0F, 0x80, 0x00, 0x80, 0x00, 0xFE, 0x3F, 0x00, 0x00
};  /*星*/
const unsigned char liu[] U8X8_PROGMEM = {
  0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x10, 0x10, 0x08, 0x10, 0x04, 0x20, 0x02, 0x20, 0x00, 0x00
};  /*六*/


//24*24小电视
const unsigned char bilibilitv_24u[] U8X8_PROGMEM = {0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x30, 0x00, 0x01, 0xe0, 0x80, 0x01,
                                                     0x80, 0xc3, 0x00, 0x00, 0xef, 0x00, 0xff, 0xff, 0xff, 0x03, 0x00, 0xc0, 0xf9, 0xff, 0xdf, 0x09, 0x00, 0xd0, 0x09, 0x00, 0xd0, 0x89, 0xc1,
                                                     0xd1, 0xe9, 0x81, 0xd3, 0x69, 0x00, 0xd6, 0x09, 0x91, 0xd0, 0x09, 0xdb, 0xd0, 0x09, 0x7e, 0xd0, 0x0d, 0x00, 0xd0, 0x4d, 0x89, 0xdb, 0xfb,
                                                     0xff, 0xdf, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x78, 0x00, 0x1e, 0x30, 0x00, 0x0c
                                                    };

/*
   初始化操作
*/
void setup() {
  Serial.begin(BAUD_RATE);
  //  pinMode(LED,OUTPUT);
  //  digitalWrite(LED, HIGH);
  u8g2.begin();
  u8g2.enableUTF8Print();

  WiFi.disconnect();
  if (!autoConfig()) {
    smartConfig();
    DebugPrint("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      //这个函数是wifi连接状态，返回wifi链接状态
      delay(500);
      DebugPrint(".");
    }
  }

  delay(1000);
  //  digitalWrite(LED, LOW);
  DebugPrintln("IP address: ");
  DebugPrintln(WiFi.localIP());//WiFi.localIP()返回8266获得的ip地址
  lastTime = millis();
  //使能软件看门狗的触发间隔
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(10); //每300秒同步一次时间
  isNTPConnected = true;
  ESP.wdtEnable(5000);
}

/**
    主函数
*/
void loop() {
  if (times == 6 ) {
    while (!client.connected()) {
      if (!client.connect(host, 80)) {
        flag = !flag;
        //         digitalWrite(LED, flag);
        delay(500);
        //喂狗
        ESP.wdtFeed();
      }
    }

    if (millis() - lastTime >= Delay) {
      //每间隔20s左右调用一次
      lastTime = millis();
      if (sendRequest(host, city, APIKEY) && skipResponseHeaders()) {
        clrEsp8266ResponseBuffer();
        readReponseContent(response, sizeof(response));
        UserData userData;
        if (parseUserData(response, &userData)) {
          showWeather(&userData);
        }
      }
    }
    delay(3000);
  }
  if (times == 0 || times == 1 || times == 2 || times == 3 || times == 4 || times == 7 || times == 8 || times == 9 || times == 10) {
    //if (timeStatus() != timeNotSet)
    // {
    Serial.println("check2");
    DebugPrintln(times);
    oledClockDisplay();
    //}
  }

  if (times == 5) {
    if (WiFi.status() == WL_CONNECTED) {
      if (getJson()) {
        if (parseJson(response1)) {
          parseJson1(responseview);
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_unifont_t_chinese2);
          u8g2.drawXBMP( 52 , 0 , 24 , 24 , bilibilitv_24u ); 
          u8g2.setCursor(30, 40);
          u8g2.print("fans:");
          u8g2.setFont(u8g2_font_unifont_t_chinese2);
          u8g2.setCursor(75, 40);
          u8g2.println(follower);
          u8g2.setFont(u8g2_font_unifont_t_chinese2);
          u8g2.setCursor(30, 60);
          u8g2.print("view:");
          u8g2.setFont(u8g2_font_unifont_t_chinese2);
          u8g2.setCursor(75, 60);
          u8g2.println(view);
          u8g2.sendBuffer();
        }
      }
      delay(1000);
    }
  }

  times += 1;
  DebugPrintln(times);
  if (times >= 10) {
    times = 0;
  }
  delay(1000);
  //喂狗
  ESP.wdtFeed();
}

/*
  自动连接20s 超过之后自动进入SmartConfig模式
*/
bool autoConfig() {
  WiFi.mode(WIFI_STA);     //设置esp8266 工作模式
  WiFi.begin();
  DebugPrintln("AutoConfiging ......");
  delay(2000);//刚启动模块的话 延时稳定一下
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Auto Configing ......");
  u8g2.sendBuffer();
  WiFi.begin(ssid, pass);
  int wstatus = WiFi.status();
  if (wstatus != WL_CONNECTED) {
    DebugPrintln("AutoConfig Success");
    DebugPrint("SSID:");
    DebugPrintln(WiFi.SSID().c_str());
    DebugPrint("PSW:");
    DebugPrintln(WiFi.psk().c_str());
    return true;
  } else {
    DebugPrint(".");
    delay(500);
    flag = !flag;
    //      digitalWrite(LED, flag);
  }

  DebugPrintln("AutoConfig Faild!");
  return false;
}

/**
  开启SmartConfig功能
*/
void smartConfig()
{
  WiFi.mode(WIFI_STA);
  delay(1000);
  DebugPrintln("Wait for Smartconfig");
  // 等待配网
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting wifi...");
  u8g2.sendBuffer();

  WiFi.beginSmartConfig();
  while (1) {
    DebugPrint(".");
    delay(200);
    flag = !flag;
    //    digitalWrite(LED, flag);

    if (WiFi.smartConfigDone()) {
      //smartconfig配置完毕
      DebugPrintln("SmartConfig Success");
      DebugPrint("SSID:");
      DebugPrintln(WiFi.SSID().c_str());
      DebugPrint("PSW:");
      DebugPrintln(WiFi.psk().c_str());
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_unifont_t_chinese2);
      u8g2.setCursor(0, 14);
      u8g2.print("Connecting...");
      u8g2.sendBuffer();
      WiFi.mode(WIFI_AP_STA);     //设置esp8266 工作模式
      WiFi.setAutoConnect(true);  // 设置自动连接
      break;
    }
  }
}

/**
  @发送请求指令
*/
bool sendRequest(const char* host, const char* cityid, const char* apiKey) {
  // We now create a URI for the request
  //心知天气
  String GetUrl = "/v3/weather/now.json?key=";
  GetUrl += apiKey;
  GetUrl += "&location=";
  GetUrl += city;
  GetUrl += "&language=";
  GetUrl += language;
  // This will send the request to the server
  client.print(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  DebugPrintln("create a request:");
  DebugPrintln(String("GET ") + GetUrl + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n");
  delay(10);
  return true;
}

/**
  跳过 HTTP 头，使我们在响应正文的开头
*/
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  bool ok = client.find(endOfHeaders);
  if (!ok) {
    DebugPrintln("No response or invalid response!");
  }
  return ok;
}

/**
   从HTTP服务器响应中读取正文
*/
void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.readBytes(content, maxSize);
  delay(100);
  DebugPrintln("Get the data from Internet!");
  content[length] = 0;
  DebugPrintln(content);
  DebugPrintln("Read data Over!");
  client.flush();//这句代码需要加上  不然会发现每隔一次client.find会失败
}

// 关闭与HTTP服务器连接
void stopConnect() {
  client.stop();
}

void clrEsp8266ResponseBuffer(void) {
  memset(response, 0, MAX_CONTENT_SIZE);      //清空
}

bool parseUserData(char* content, struct UserData* userData) {
  //    -- 根据我们需要解析的数据来计算JSON缓冲区最佳大小
  //   如果你使用StaticJsonBuffer时才需要
  //    const size_t BUFFER_SIZE = 1024;
  //   在堆栈上分配一个临时内存池
  //    StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  //    -- 如果堆栈的内存池太大，使用 DynamicJsonBuffer jsonBuffer 代替
  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  //复制我们感兴趣的字符串
  strcpy(userData->city, root["results"][0]["location"]["name"]);
  strcpy(userData->weather_code, root["results"][0]["now"]["code"]);
  strcpy(userData->temp, root["results"][0]["now"]["temperature"]);
  //  -- 这不是强制复制，你可以使用指针，因为他们是指向“内容”缓冲区内，所以你需要确保
  //   当你读取字符串时它仍在内存中
  return true;
}

/**
   根据天气接口返回的数据判断显示
*/
void showWeather(struct UserData* userData) {
  if (strcmp(userData->weather_code, WEATHER_CODE_DAY_SUN) == 0
      || strcmp(userData->weather_code, WEATHER_CODE_DAY_SUN1) == 0) {
    drawWeather(SUN_DAY, userData->temp, userData->city);
  } else if (strcmp(userData->weather_code, WEATHER_CODE_NIGHT_SUN) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_NIGHT_SUN2) == 0 ) {
    drawWeather(SUN_NIGHT, userData->temp, userData->city);
  } else if (strcmp(userData->weather_code, WEATHER_CODE_DAY_PARTLY_CLOUDY) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_NIGHT_PARTLY_CLOUDY) == 0 ) {
    drawWeather(SUN_CLOUD, userData->temp, userData->city);
  } else if (strcmp(userData->weather_code, WEATHER_CODE_CLOUDY) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_DAY_MOSTLY_CLOUDY) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_NIGHT_MOSTLY_CLOUDY) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_OVERCAST) == 0) {
    drawWeather(CLOUD, userData->temp, userData->city);
  } else if (strcmp(userData->weather_code, WEATHER_CODE_SHOWER) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_LIGHT_RAIN) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_MODERATE_RAIN) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_HEAVY_RAIN) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_STORM) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_HEAVY_STORM) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_SEVERE_STORM) == 0) {
    drawWeather(RAIN, userData->temp, userData->city);
  } else if (strcmp(userData->weather_code, WEATHER_CODE_THUNDERSHOWER) == 0
             || strcmp(userData->weather_code, WEATHER_CODE_THUNDERSHOWER_WITH_HAIL) == 0) {
    drawWeather(THUNDER, userData->temp, userData->city);
  } else {
    drawWeather(CLOUD, userData->temp, userData->city);
  }
}

void drawWeather(uint8_t symbol, char* degree, char* city1)
{
  DebugPrintln(city);
  u8g2.clearBuffer();         // clear the internal memory
  //绘制天气符号
  drawWeatherSymbol(0, 48, symbol);
  //绘制温度
  u8g2.setFont(u8g2_font_logisoso32_tf);
  u8g2.setCursor(48 + 3, 42);
  u8g2.print(degree);
  u8g2.print("°C");   // requires enableUTF8Print()
  u8g2.setFont(u8g2_font_unifont_t_chinese3);

  u8g2_uint_t strWidth = u8g2.getUTF8Width(city);
  u8g2_uint_t displayWidth = u8g2.getDisplayWidth();

  u8g2.setCursor(displayWidth - strWidth - 5, 60);
  u8g2.print("TianJin");
  u8g2.sendBuffer();        // transfer internal memory to the display
}

/**
   绘制天气符号
*/
void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  // fonts used:
  // u8g2_font_open_iconic_embedded_6x_t
  // u8g2_font_open_iconic_weather_6x_t
  // encoding values, see: https://github.com/olikraus/u8g2/wiki/fntgrpiconic
  switch (symbol)
  {
    case SUN_DAY://太阳
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 69);
      break;
    case SUN_NIGHT://太阳
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 66);
      break;
    case SUN_CLOUD://晴间多云
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 65);
      break;
    case CLOUD://多云
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 64);
      break;
    case RAIN://下雨
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;
    case THUNDER://打雷
      u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;
  }
}
bool getJson()
{
  bool r = false;
  http.setTimeout(HTTP_TIMEOUT);

  //followe
  http.begin("http://api.bilibili.com/x/relation/stat?vmid=" + biliuid);
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      response1 = http.getString();
      Serial.println(response1);
      r = true;
    }
  } else {
    Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
    r = false;
  }
  http.end();
  //return r;

  //view
  http.begin("http://api.bilibili.com/x/space/upstat?mid=" + biliuid);
  int httpCodeview = http.GET();
  if (httpCodeview > 0) {
    if (httpCodeview == HTTP_CODE_OK) {
      responseview = http.getString();
      Serial.println(responseview);
      r = true;
    }
  } else {
    Serial.printf("[HTTP] GET JSON failed, error: %s\n", http.errorToString(httpCode).c_str());
    r = false;
  }
  http.end();
  return r;
}

bool parseJson(String json)
{
  const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 100;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(response1);

  int code = root["code"]; // 0
  const char* message = root["message"]; // "0"
  int ttl = root["ttl"]; // 1

  JsonObject& data = root["data"];
  long data_mid = data["mid"]; // 22774364
  int data_following = data["following"]; // 208
  int data_whisper = data["whisper"]; // 0
  int data_black = data["black"]; // 0
  int data_follower = data["follower"]; // 29

  root.printTo(Serial);

  if (code != 0) {
    Serial.print("[API]Code:");
    Serial.print(code);
    Serial.print(" Message:");
    Serial.println(message);

    return false;
  }

  if (data_mid == 0) {
    Serial.println("[JSON] FORMAT ERROR");
    return false;
  }
  Serial.print("UID: ");
  Serial.print(data_mid);
  Serial.print(" follower: ");
  Serial.println(data_follower);

  follower = data_follower;
  return true;
}

void parseJson1(String json)
{
  const size_t capacity = 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 80;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(responseview);

  int code = root["code"]; // 0
  const char* message = root["message"]; // "0"
  int ttl = root["ttl"]; // 1

  JsonObject& data = root["data"];

  int data_archive_view = data["archive"]["view"]; // 275

  int data_article_view = data["article"]["view"]; // 0

  int data_likes = data["likes"]; // 426
  Serial.print("view: ");
  Serial.println(data_archive_view);

  view = data_archive_view;
}

void oledClockDisplay()
{
  Serial.println("check1");
  int years, months, days, hours, minutes, seconds, weekdays;
  years = year();
  months = month();
  days = day();
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  if (isNTPConnected)
    u8g2.print("当前时间 (UTC+8)");
  else
    u8g2.print("无网络!"); //如果上次对时失败，则会显示无网络
  String currentTime = "";
  if (hours < 10)
    currentTime += 0;
  currentTime += hours;
  currentTime += ":";
  if (minutes < 10)
    currentTime += 0;
  currentTime += minutes;
  currentTime += ":";
  if (seconds < 10)
    currentTime += 0;
  currentTime += seconds;
  String currentDay = "";
  currentDay += years;
  currentDay += "/";
  if (months < 10)
    currentDay += 0;
  currentDay += months;
  currentDay += "/";
  if (days < 10)
    currentDay += 0;
  currentDay += days;

  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(0, 44);
  u8g2.print(currentTime);
  u8g2.setCursor(0, 61);
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.print(currentDay);
  u8g2.drawXBM(80, 48, 16, 16, xing);
  u8g2.setCursor(95, 62);
  u8g2.print("期");
  if (weekdays == 1)
    u8g2.print("日");
  else if (weekdays == 2)
    u8g2.print("一");
  else if (weekdays == 3)
    u8g2.print("二");
  else if (weekdays == 4)
    u8g2.print("三");
  else if (weekdays == 5)
    u8g2.print("四");
  else if (weekdays == 6)
    u8g2.print("五");
  else if (weekdays == 7)
    u8g2.drawXBM(111, 49, 16, 16, liu);
  u8g2.sendBuffer();
}

/*-------- NTP 代码 ----------*/

const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE]; // 输入输出包的缓冲区


time_t getNtpTime()
{
  IPAddress ntpServerIP;          // NTP服务器的地址

  while (Udp.parsePacket() > 0);  // 丢弃以前接收的任何数据包
  Serial.println("Transmit NTP Request");
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-("); //无NTP响应
  isNTPConnected = false;
  return 0; //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
