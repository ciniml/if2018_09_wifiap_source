/**
 * wifi_client.ino
 */
#include <WiFi.h>
#include <stdint.h>
#include <command.hpp>

static const char* AP_SSID = "test_ap";
static const char* AP_PASSPHRASE = "passphrase";
static const IPAddress SERVER_ADDRESS(192, 168, 0, 1);
static const uint16_t SERVER_PORT = 80;

static WiFiClient client;

// コマンド・レシーバのインスタンス
static CommandReceiver receiver;
// コマンド・トランスミッタのインスタンス
static CommandTransmitter transmitter;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to the access point...");

  // 接続先アクセスポイントを設定
  WiFi.begin(AP_SSID, AP_PASSPHRASE);
  WiFi.setAutoConnect(true);
}

static uint8_t led = 0;

void loop() {
  if( !client.connected() ) { // クライアント未接続？
    auto status = WiFi.status();
    if( status != WL_CONNECTED ) {  // アクセスポイントに接続されていない？
      if( status == WL_DISCONNECTED ) { // アクセスポイントから切断している？
        WiFi.reconnect(); // 再接続する。
      }
      Serial.println("Connecting to the AP...");
      delay(500);
      return;
    }
    receiver.clear();    // 接続されていないならコマンド送受信状態を初期化する。
    transmitter.clear(); // /
    if( client.connect(SERVER_ADDRESS, SERVER_PORT) ) {
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
    Serial.println("Set command");
    transmitter.set(0x00, &led, 1); // コマンド0x00
    led ^= 1;
    delay(200);
  }
}
