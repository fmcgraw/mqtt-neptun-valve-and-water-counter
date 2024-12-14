#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define STR_ADDR 0 

const char* ssid = "YOUR-WI-FI-SSID";
const char* password = "YOUR-WI-FI-PASSWORD";

const char* mqtt_server = "YOUR-MQTT-BROKER-IP-ADDRESS";
const char* mqtt_user = "YOUR-MQTT-BROKER-USER";
const char* mqtt_password = "YOUR-MQTT-BROKER-PASSWORD";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
String stringtopic;

const char* inTopic = "waterControl/commands/#";
const char* inTopicRelay = "waterControl/commands/relay";
const char* inTopicSetCounter = "waterControl/commands/setCounter";
const char* outTopicOnline = "waterControl/state/online";
const char* outTopicRelayState = "waterControl/state/relay";
const char* outTopicCounter = "waterControl/state/counter";

int relay_pin = D2;
int led_connect = D0;
int led_open = D1;

int button_open = D5;
int button_close = D6;
int button_counter = D7;

int eeprom_state_address = 0;

Bounce b_open = Bounce();
Bounce b_close = Bounce();
Bounce b_counter = Bounce();

void extButtonOpen() {
  b_open.update();

  if ( b_open.fell() ) {
    Serial.print("Нажали кнопку Open");
    Serial.print("\n");

    digitalWrite(relay_pin, LOW);
    digitalWrite(led_open, HIGH);

    digitalWrite(led_connect, LOW);
    delay(100);
    digitalWrite(led_connect, HIGH);

    client.publish(outTopicRelayState, "1");
   }
}

void extButtonClose() {
  b_close.update();

  if ( b_close.fell() ) {
    Serial.print("Нажали кнопку Close");
    Serial.print("\n");

    digitalWrite(relay_pin, HIGH);
    digitalWrite(led_open, LOW);

    digitalWrite(led_connect, LOW);
    delay(100);
    digitalWrite(led_connect, HIGH);

    client.publish(outTopicRelayState, "0");
   }
}

void extButtonCounter() {
  b_counter.update();

  if ( b_counter.fell() ) {
    Serial.println("Пришли данные от счетчика:");
    
    String str;
    int len = EEPROM.read(STR_ADDR);
    str.reserve(len);

    for (int i = 0; i < len; i++) {
      str += (char)EEPROM.read(STR_ADDR + 1 + i);
    }

    int counter_int = str.toInt();
    counter_int = counter_int + 1;

    Serial.println(counter_int);

    char cstr[16];
    client.publish(outTopicCounter, itoa(counter_int, cstr, 10));

    String counter_st = String(counter_int);

    int len_new = counter_st.length();
     
    EEPROM.write(STR_ADDR, len_new);
    EEPROM.commit();
     
    for (int i = 0; i < len_new; i++) {
      EEPROM.write(STR_ADDR + 1 + i, counter_st[i]);
      EEPROM.commit();
    }
   }
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.hostname("waterControl");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for(int i = 0; i<500; i++){
      delay(1);
    }
    Serial.print(".");
  }
  
  digitalWrite(led_connect, HIGH);
  delay(500);
  digitalWrite(led_connect, LOW);
  delay(500);
  digitalWrite(led_connect, HIGH);
  delay(500);
  digitalWrite(led_connect, LOW);
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void callback(char* topic, byte* payload, unsigned int length) {    
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");  
  Serial.println("");

  digitalWrite(led_connect, LOW);
  delay(100);
  digitalWrite(led_connect, HIGH);

  stringtopic =  String(topic);

  if (stringtopic == inTopicRelay) {
    Serial.println("Received command for Valve");
    
    if ((char)payload[0] == '0') {
      Serial.println("Close");
      digitalWrite(relay_pin, HIGH);
      digitalWrite(led_open, LOW);
    }
    
    if ((char)payload[0] == '1') {
      Serial.println("Open");
      digitalWrite(relay_pin, LOW);
      digitalWrite(led_open, HIGH);
    }
  }

  if (stringtopic == inTopicSetCounter) {
    String counter_payload = "";

    for (int i = 0; i < length; i++) {
      counter_payload = counter_payload + (char)payload[i];
    } 

    Serial.println("");
    Serial.println(counter_payload);

    int len = counter_payload.length();
     
    EEPROM.write(STR_ADDR, len);
    EEPROM.commit();
     
    for (int i = 0; i < len; i++) {
      EEPROM.write(STR_ADDR + 1 + i, counter_payload[i]);
      EEPROM.commit();
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("waterControl", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopicOnline, "1");
      // ... and resubscribe
      client.subscribe(inTopic);

      delay(1000);
      digitalWrite(led_connect, HIGH);
      delay(500);
      digitalWrite(led_connect, LOW);
      
      delay(500);
      digitalWrite(led_connect, HIGH);
      delay(500);
      digitalWrite(led_connect, LOW);
      
      delay(500);
      digitalWrite(led_connect, HIGH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for(int i = 0; i<5000; i++){
        delay(1);
      }
    }
  }
}

void setup() {
  EEPROM.begin(512);
  pinMode(relay_pin, OUTPUT); 
  pinMode(led_connect, OUTPUT);
  pinMode(led_open, OUTPUT);
  pinMode(button_open, INPUT_PULLUP);
  pinMode(button_close, INPUT_PULLUP);
  pinMode(button_counter, INPUT_PULLUP);

  b_open.attach(button_open);
  b_open.interval(25);

  b_close.attach(button_close);
  b_close.interval(25);

  b_counter.attach(button_counter);
  b_counter.interval(25);
  
  digitalWrite(relay_pin, LOW);
  
  digitalWrite(led_connect, HIGH);
  delay(500);
  digitalWrite(led_connect, LOW);

  digitalWrite(led_open, HIGH);

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  extButtonOpen();
  extButtonClose();
  extButtonCounter();
}
