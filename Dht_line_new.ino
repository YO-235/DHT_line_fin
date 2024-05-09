#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DHT.h>
#include <TridentTD_LineNotify.h>

// test web server

#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>

// 將網頁設定在 tcp 80 port
AsyncWebServer server(80);

// 資料
String data = "";
String data2 = "";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
  <meta http-equiv="refresh" content="1" />

  <style>
    body{font-size: 36px; }
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori{ margin-bottom: 0%; }
  </style>
</head>
<body>
    <p>%DATA%</p>
    <p>%DATA2%</p>
</body>
</html>)rawliteral";


// 網頁請求的資料回傳
String processor(const String& var){
  if (var == "DATA"){ // DATA = 溫度
    return String(data);
  }
  if (var == "DATA2") { // DATA2 = 濕度
    return String(data2);
  }
  return String();
}

#define DHTTYPE DHT11
#define DHTPIN D2 
// 修改成上述寄到登入郵箱的 Token號碼
#define LINE_TOKEN "f58UOcxWPW56zixG8bg02VzRvZUvbVk0P0y9TVm87Kk"

// 設定無線基地台SSID跟密碼
const char* ssid     = "米糠醃小黃瓜";
const char* password = "defense0230";
const char* ntpServer = "pool.ntp.org";
DHT dht(DHTPIN, DHTTYPE, 11);    // 11 works fine for ESP8266
 
float humidity, temp_f;   // 從 DHT-11 讀取的值


unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor
bool tempMsgSent = false;
bool humidMsgSent = false;
unsigned long tempStartTime = 0;
unsigned long humidStartTime = 0;
void setup(void)
{
 
  //LINE.begin();
  Serial.begin(9600);  // 設定速率 感測器
  dht.begin();           // 初始化

  WiFi.mode(WIFI_STA);
  // 連接無線基地台
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

  // 等待連線，並從 Console顯示 IP
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Weather Reading Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(5000); // yuki_watchIP

  // 啟動網頁伺服器
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html,processor);
  });

  // 開始運行網頁伺服器
  server.begin();
}

void loop(void)
{
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // 將最後讀取感測值的時間紀錄下來 
    previousMillis = currentMillis;   

    // 讀取溫度大約 250 微秒!
    humidity = dht.readHumidity();          // 讀取濕度(百分比)
    temp_f = dht.readTemperature(true);     // 讀取溫度(華氏)
    
 
    // 檢查兩個值是否為空值
    if (isnan(humidity) || isnan(temp_f)) {
       Serial.println("Failed to read from DHT sensor!");
       return;
    }
  }
  int temp = (temp_f-32)*5/9;
  String tempe="溫度:"+String(temp)+"℃";   
  String humid="濕度:"+String((int)humidity)+"％";

  // 在 serial print 出來
  Serial.println(tempe);
  Serial.println(humid);

  // 更新網頁資料
  data = tempe; // 溫度
  data2 = humid; // 濕度

  // 顯示 Line版本
  Serial.println(LINE.getVersion());
  LINE.setToken(LINE_TOKEN);
    
  if (temp > 45) {
    if (!tempMsgSent) {
        tempStartTime = currentMillis; // 開始計時
        LINE.setToken(LINE_TOKEN);
        LINE.notify("警告：溫度異常！溫度:" + String(temp) + "℃");
        delay(1000);
        tempMsgSent = true; // 設置溫度警報標誌為真
    }
  } else {
    if (tempMsgSent && currentMillis - tempStartTime >= 600000) { // 如果溫度超過閾值且已經十分鐘
      LINE.setToken(LINE_TOKEN);
      LINE.notify("溫度已恢復正常 溫度:" + String(temp) + "℃");
      delay(1000);
      tempMsgSent = false; // 重置溫度警報標誌為假
    }
  }

  if (humidity > 75) {
    if (!humidMsgSent) {
        humidStartTime = currentMillis; // 開始計時
        LINE.setToken(LINE_TOKEN);
        LINE.notify("警告：濕度異常！濕度:" + String((int)humidity) +"%");
        delay(1000);
        humidMsgSent = true; // 設置濕度警報標誌為真
    }
  } else {
    if (humidMsgSent && currentMillis - humidStartTime >= 600000) { // 如果濕度超過閾值且已經十分鐘
      LINE.setToken(LINE_TOKEN);
      LINE.notify("濕度已恢復正常 濕度:" + String((int)humidity) + "%");
      delay(1000);
      humidMsgSent = false; // 重置濕度警報標誌為假
    }
  }
}
  

 
