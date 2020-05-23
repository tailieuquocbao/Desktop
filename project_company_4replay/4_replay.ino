#include <FS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266TimeAlarms.h>
#include <string.h>
#include <stdlib.h>

#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <driver/adc.h>

#include "digitalPin.h"
#include "Button.h"
#include "Relay.h"

byte willQoS = 1; // gui tin hieu len sever khi mat ket noi
boolean willRetain = false;
const char *willTopic = "connected/192";
const char *device_serial_number = "192";
const char *device_relay_number[] = {"0", "1", "2", "3"};

const char *willMessage = "0";


#define MSG_BUFFER_SIZE (50) // khai bao bo nho so luong ki tu gui len cho sever
char msg[MSG_BUFFER_SIZE];
int relayNo = 0;
int relayNoValue = 1;
const int MODE_MANUAL = 1;
const int MODE_AUTO_BALANCE = 2;
const int MODE_CYCLE = 3;
const int MODE_DATETIME = 4;

// DigitalPin relayPin[] = {25, 18, 19, 21};
// DigitalPin relayPin[] = {14, 27, 26, 25};
Relay relayPin[] = {14, 27, 26, 25};

int  rqqqq = 19 ;

Button button1(5);

// DigitalPin ledMode1(19), ledMode2(21), ledMode3(33), ledMode4(23), ledWifi(18); 
DigitalPin  ledWifi(18);

String ssid = WiFi.SSID();
String password = WiFi.psk();
char saved_ssid[100] = "";
char saved_psk[100] = "";
const char *mqtt_server = "farm-dev.affoz.com";

AlarmId id; // setup cho ham free alarm dung de reset bo nho alarm
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

const int tempPin = 32;
double degreesC;
double tempReading = 0;

int mode = 0;


/*
{"mode":1,"setting":{"min":20,"max":33,"mode_on":true}}
{"mode":2,"setting":{"min":20,"max":33,"mode_on":true}}
{"mode":3,"setting":{"run_time":4,"rest_time":2,"apply_datetime":1587124290,"stop_datetime":1587124290,"repeat":100}}
{"mode":4,"setting":{"mon":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"tue":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"wed":[["12:04","12:12"],["11:59","12:12"],["18:00","19:00"]],"thu":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"fri":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sat":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sun":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"apply_datetime":1587124290,"stop_datetime":1587124290,"repeat":100}}

      http://www.hivemq.com/demos/websocket-client/?#data
*/
const char *AP_Name = "ten wifi dat cho esp";

bool shouldSaveConfig = false;
bool forceConfigPortal = false;
WiFiManager wifiManager; 
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}
void setupRealTimeClock()
{
    configTime(0, 0, "0.vn.pool.ntp.org", "3.asia.pool.ntp.org", "0.asia.pool.ntp.org");
    //set mui gio cua viet nam
    //Get JSON of Olson to TZ string using this code https://github.com/pgurenko/tzinfo
    setenv("TZ", "WIB-7", 1);
    tzset();
    Serial.print("Clock before sync: "); 
    digitalClockDisplay();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    randomSeed(micros());
}
void setup_wifi()
{

    for (int i = 0; i < 5; i++)
    {
        Serial.print(".");
        delay(1000);
    }
    WiFi.mode(WIFI_STA); // thu xem co duoc khong
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    if (forceConfigPortal)
    {
        wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
        // Force config portal while jumper is set for flashing
        if (!wifiManager.startConfigPortal(AP_Name))
        {
            Serial.println("** ERROR ** Failed to connect with new config / possibly hit config portal timeout; Resetting in 3sec...");
            delay(3000);
            //reset and try again, or maybe put it to deep sleep
            // ESP.reset();
            // ESP.restart();

            delay(5000);
        }
    }
    else
    {
        // Autoconnect if we can

        // Fetches ssid and pass and tries to connect; if it does not connect it starts an access point with the specified name
        // and goes into a blocking loop awaiting configuration
        if (!wifiManager.autoConnect(AP_Name))
        {
            Serial.println("** ERROR ** Failed to connect with new config / possibly hit timeout; Resetting in 3sec...");
            delay(3000);
            //reset and try again, or maybe put it to deep sleep
            // ESP.reset();
            // ESP.restart();
            delay(5000);
        }
    }
    WiFi.SSID().toCharArray(saved_ssid, 100);
    WiFi.psk().toCharArray(saved_psk, 100);
}

void reconnectIfNecessary()
{
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Disconnected; Attempting reconnect to " + String(saved_ssid) + "...");
        WiFi.disconnect();
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(saved_ssid, saved_psk);
        // Output reconnection status info every second over the next 10 sec
        for (int i = 0; i < 10; i++)
        {
            delay(1000);
            Serial.print("WiFi status = ");
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.println("Connected");
                break;
            }
            else
            {
               
                Serial.println("Disconnected");
            }
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Failure to establish connection after 10 sec. Will reattempt connection in 2 sec");
            ledWifi.on();
            delay(2000);
        }
    }
}

void digitalClockDisplay()
{
    time_t tnow = time(nullptr);
    Serial.println(tnow);
    Serial.println(ctime(&tnow));

    //    Serial.println(ctime(&tnow));
}

void setup()
{
    Serial.begin(115200);
    adc1_config_width(ADC_WIDTH_12Bit);

    ledWifi.begin();
    // ledMode1.begin();
    // ledMode3.begin();
    // ledMode4.begin();
    for (int i = 0; i < 4; i++)
    {
        relayPin[i].begin();
    }

    button1.init();
    setup_wifi();
    setupRealTimeClock();
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    mode = 0;
}

void loop()
{
    if (!client.connected())
    {
        ledWifi.on();
        reconnect();
    }
    client.loop();
    sensorUpdate();
 
    if (button1.isPressed())
    {
        delay(20);
        while ( button1.isPressed() ) {};
        relayPin[relayNo].status() == false? relayPin[relayNo].on(): relayPin[relayNo].off();
    }
  
    //  if (mode == MODE_AUTO_BALANCE)
    // {
    //     if (chedo == false)
    //     {
    //         ((celsius > nhietmax) || (celsius < nhietmin))? relayPin[relayNo].on(): relayPin[relayNo].off();
    //     }
    //     if (chedo == true)
    //     {
    //         ((celsius < nhietmax) && (celsius > nhietmin)) ? relayPin[relayNo].on() : relayPin[relayNo].off();
    //     }
    // }
   
        digitalClockDisplay();
        Alarm.delay(1000);
    
}
void mqttCallback(char *topic, byte *payload, unsigned int length)
{

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print(length);
    Serial.println((char *)payload);

    if (String(topic).startsWith("setting") && String(topic).endsWith("0"))
    {
        relayNo = 0;
        char *json = (char *)payload;
        relayPin[0].applySetting(json);
        
        Serial.println("da nhan duoc setting 3");
    }else if ( String(topic).startsWith("setting") && String(topic).endsWith("1") )
    {
        relayNo = 1;
        char *json = (char *)payload;
        relayPin[1].applySetting(json);
        Serial.println("da nhan duoc setting 1");

      
    }else if ( String(topic).startsWith("setting") && String(topic).endsWith("2") )
    {
        relayNo = 2;
        char *json = (char *)payload;
        relayPin[2].applySetting(json);

        
        Serial.println("da nhan duoc setting 2");
    }else if ( String(topic).startsWith("setting") && String(topic).endsWith("3") )
    {
        relayNo = 3;
        char *json = (char *)payload;
        relayPin[3].applySetting(json);
        Serial.println("da nhan duoc setting 3");
    }
      if ((payload[0] == '1'))
    {
        relayPin[relayNo].on();
        Serial.println("da nhan duoc mode 1 bat");
    }
    else if ( (payload[0] == '0'))
    {
        relayPin[relayNo].off();
    }
}
void sensorUpdate()
{
    unsigned long now = millis();
    if (now - lastMsg > 3000)
    {
        lastMsg = now;

        tempReading = analogRead(tempPin);
        //tempreading = adc1_get_raw(ADC1_CHANNEL_4);
        int mv = (tempReading / 4095.0) * 3300;
        int degreesC = mv / 10;
        Serial.println(String(degreesC));
        snprintf(msg, MSG_BUFFER_SIZE, "%d", degreesC);
        Serial.print("Publish message: ");
        Serial.println(msg);
        Serial.println(degreesC);
        publish("sensor", msg);
        // ledWifi.on();
        Serial.println(mode);
    }
}
void subscribe(char *subtopic)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) +strlen(device_relay_number[relayNo]) + 1];
    // sprintf(topic, "%s%s%s%s%s", subtopic, "/", device_serial_number,"/", device_relay_number[relayNo]);
    // client.subscribe(topic);
    for (int i = 0; i < 4; i++)
    {
         sprintf(topic, "%s%s%s%s%s", subtopic, "/", device_serial_number,"/", device_relay_number[i]);
    client.subscribe(topic);
    }
    
    Serial.print("subscribed to topic: ");
    Serial.println(topic);
}
void publish(char *subtopic, char *msg)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + strlen(device_relay_number[relayNo]) + 1];
    sprintf(topic, "%s%s%s%s%s", subtopic, "/", device_serial_number, "/", device_relay_number[relayNo] );
    client.publish(topic, msg);
    Serial.print("published to topic: ");
    Serial.print(topic);
    Serial.print("; with message: ");
    Serial.println(msg);
}
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {

        ledWifi.on();
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), willTopic, willQoS, willRetain, willMessage))
        {
            Serial.println("connected");
            publish("connected", "1");                   //neu ket noi dc thi gui len sever connected/192168110 [1]
            Serial.println("da ket noi duoc voi sever"); //neu ket noi dc thi gui len sever connected/192168110 [1]
            // Once connected, publish an announcement...

            // ... and resubscribe
            subscribe("setting");
            subscribe("turn_on");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}