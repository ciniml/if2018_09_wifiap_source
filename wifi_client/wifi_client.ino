#include <WiFi.h>
#include <ESPmDNS.h>
#include <stdint.h>
#include <command.hpp>

static const int GPIO_LED = 32;
static const int GPIO_SWITCH = 13;

#define USE_MDNS

static const char* CLIENT_NAME = "test_client";
static const char* SERVICE_NAME = "esp32_command";

static const char* AP_SSID = "your_ssid";
static const char* AP_PASSPHRASE = "your_passphrase";

static IPAddress serverAddress(192, 168, 0, 1);
static uint16_t serverPort = 80;
#ifdef USE_MDNS
static bool serviceFound = false;
#endif

static WiFiClient client;

// コマンド・レシーバのインスタンス
static CommandReceiver receiver;
// コマンド・トランスミッタのインスタンス
static CommandTransmitter transmitter;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to the access point...");

  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);
  pinMode(GPIO_SWITCH, INPUT);
  digitalWrite(GPIO_SWITCH, 0);
  
  // 接続先アクセスポイントを設定
  WiFi.begin(AP_SSID, AP_PASSPHRASE);
  WiFi.setAutoConnect(true);
  // mDNSを初期化
  MDNS.begin(CLIENT_NAME);
}

static uint8_t led = 0;
static bool last_sw_pressed = false;
void loop() {
  if( !client.connected() ) { // クライアント未接続？
    auto status = WiFi.status();
    if( status != WL_CONNECTED ) {  // アクセスポイントに接続されていない？
#ifdef USE_MDNS
      serviceFound = false;
#endif
      if( status == WL_DISCONNECTED ) { // アクセスポイントから切断している？
        WiFi.reconnect(); // 再接続する。
      }
      Serial.println("Connecting to the AP...");
      delay(500);
      return;
    }
    // mDNSを使う場合、サービスを検索する
#ifdef USE_MDNS
    if( !serviceFound ) {
      Serial.println("Querying service...");
      int serviceCount = MDNS.queryService(SERVICE_NAME, "tcp");
      if( serviceCount == 0 ) {
        // サービスが見つからなかった。
        Serial.println("No services found.");
        delay(1000);
        return;
      }
      Serial.print("Service: ");
      Serial.print(MDNS.hostname(0));
      Serial.print(" - ");
      Serial.print(MDNS.IP(0));
      Serial.print(":");
      Serial.print(MDNS.port(0));
      Serial.println();
      serverAddress = MDNS.IP(0);
      serverPort = MDNS.port(0);
      serviceFound = true;  // サービス検出済みとする。
    }
#endif // USE_MDNS

    receiver.clear();    // 接続されていないならコマンド送受信状態を初期化する。
    transmitter.clear(); // /
    if( client.connect(serverAddress, serverPort) ) {
      // 接続した
      client.setTimeout(1); // タイムアウトを1秒に設定
      Serial.println("Connected to the server");
    }
    else {
      // 接続できなかった。
      delay(1000);
      return;
    }
  }

  if( transmitter.transmitting() ) {  // コマンド送信中？
    Serial.println("Transmit");
    transmitter.transmit(client); // 送信処理を行う
  }
  else if( receiver.receive(client) ) {  // コマンド受信した？
    Serial.print("command ");
    Serial.print(receiver.number());
    Serial.println();
    if( receiver.number() == 0x00 ) { // コマンド0x00
      if( receiver.length() >= 1 ) {
        const uint8_t* body = receiver.body();
        if( body[0] == 0x00 )  {
          Serial.println("Response: LED off");
        }
        else if( body[0] == 0x01 )  {
          Serial.println("Response: LED on");
        }
      }
    }
  }
  else {
    // 次回送信コマンドをセット
    bool sw_pressed = digitalRead(GPIO_SWITCH) == LOW;
    if( sw_pressed && !last_sw_pressed ) {
      Serial.println("Set command");
      transmitter.set(0x00, &led, 1); // コマンド0x00
      digitalWrite(GPIO_LED, led);
      led ^= 1;
    }
    last_sw_pressed = sw_pressed;
  }
}
