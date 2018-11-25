/**
 * wifi_server.ino
 */
#include <WiFi.h>
#include <stdint.h>
#include <command.hpp>

static const char* AP_SSID = "test_ap";
static const char* AP_PASSPHRASE = "passphrase";
static const IPAddress SERVER_ADDRESS(192, 168, 0, 1);
static const uint16_t SERVER_PORT = 80;

static WiFiServer server(SERVER_PORT);
static WiFiClient client;


// コマンド・レシーバのインスタンス
static CommandReceiver receiver;
// コマンド・トランスミッタのインスタンス
static CommandTransmitter transmitter;

static const int GPIO_LED = 12;
static const int GPIO_CONNECTION_LED = 32;
void setup() {
  // GPIO初期化
  pinMode(GPIO_LED, OUTPUT);
  digitalWrite(GPIO_LED, LOW);
  pinMode(GPIO_CONNECTION_LED, OUTPUT);
  digitalWrite(GPIO_CONNECTION_LED, LOW);
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  // アクセスポイントを初期化
  WiFi.softAPConfig(SERVER_ADDRESS, SERVER_ADDRESS, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASSPHRASE, 1, 0, 1);
  
  server.begin();
}

static uint8_t connectionLed = 0;

void loop() {
  WiFiClient newClient = server.available();  // クライアントからの接続を確認
  if( newClient ) { // 新しい接続がある？
    digitalWrite(GPIO_CONNECTION_LED, LOW);
    receiver.clear();    // 接続されていないならコマンド送受信状態を初期化する。
    transmitter.clear(); // /
    client = newClient;
  }

  if( client.connected() ) {  // クライアント接続済み？
    digitalWrite(GPIO_CONNECTION_LED, ((connectionLed + 1) & 0x80) != 0 );
    connectionLed = (connectionLed + 1) & 0x7f;
    client.available();
    
    if( transmitter.transmitting() ) {  // コマンド送信中？
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
            Serial.println("LED off");
            digitalWrite(GPIO_LED, LOW);
          }
          else if( body[0] == 0x01 )  {
            Serial.println("LED on");
            digitalWrite(GPIO_LED, HIGH);
          }
          uint8_t response_body = body[0];
          transmitter.set(0x00, &response_body, 1);  // 本体1バイトのレスポンスを送信
        }
      }
    }
  }
  else {
    // 未接続
    delay(1000);
  }
}
