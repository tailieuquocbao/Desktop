#include <FS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266TimeAlarms.h>
#include <string.h>
#include <stdlib.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Esp32WifiManager.h>


byte willQoS = 1; // gui tin hieu len sever khi mat ket noi
const char *willTopic = "connected/192168110";
const char *willMessage = "0";
boolean willRetain = false;

const char *device_serial_number = "192168110";
const bool ON = true;
const bool OFF = false;
const int button = D2;    //khai bao button reset
const int ledwifi = D1;   //khai bao den led wifi
const int ledreset = D4;  //khai bao den led wifi
const int outputpin = A0; //Khai bao chan doc nhiet do
const int Relay = D8;     //Khai bao chan output vao relay
const int l_m1 = D0; // define pin led mode 
const int l_m2 = D5;
const int l_m3 = D6;
const int l_m4 = D7;
const int MODE_MANUAL = 1;
const int MODE_AUTO_BALANCE = 2;
const int MODE_CYCLE = 3;
const int MODE_DATETIME = 4;

bool relay_status = OFF;
bool mode4_status = OFF;
bool is_set_alarm_cycle = false;
bool is_set_alarm_date_time = false;

// // Wifi config
// const char *ssid = "My Friend";
// const char *password = "khongcopass";
String ssid = WiFi.SSID();
String password = WiFi.psk();
char saved_ssid[100] = "";
char saved_psk[100] = "";
const char *mqtt_server = "farm-dev.affoz.com";

AlarmId id;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int mode = 0;
float nhietmax = 0;
float nhietmin = 0;
bool chedo = true;
float celsius = 0;
int solan = 0;
int value = 0;

long cy_run_time = 0;
long cy_rest_time = 0;
long cy_apply_datetime = 0;
long cy_stop_datetime = 0;
long cy_repeat = 0;

int rt_repeat = 0;

const char *AP_Name = "eererer 3";
String mon;
String tue;
String wed;
String thu;
String fri;
String sat;
String sun;

AlarmID_t weekly_Arlarm_id[100];
AlarmID_t My_cy_timer_start ;
AlarmID_t My_cy_timer_stop ;
AlarmID_t My_rt_timer_start ;
AlarmID_t My_rt_timer_repeat ;


const timeDayOfWeek_t weekly_Arlarm_name[7] = {dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday, dowSunday};
const char *weekly_time_name[7] = {"mon", "tue", "wed", "thu", "fri", "sat", "sun"};

char json[550] = "{\"mode\":1,\"limit\":{\"min\":1235,\"max\":1234,\"mode_on\":false}}";
/*
{"mode":2,"setting":{"min":20,"max":33,"mode_on":true}}
{"mode":3,"setting":{"run_time":7,"rest_time":4,"apply_datetime":1587124290,"stop_datetime":1587124290,"repeat":100}}
{"mode":4,"setting":{"mon":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"tue":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"wed":[["12:04","12:12"],["11:59","12:12"],["18:00","19:00"]],"thu":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"fri":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sat":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sun":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"apply_datetime":1587124290,"stop_datetime":1587124290,"repeat":100}}
    
      http://www.hivemq.com/demos/websocket-client/?#data
*/
/**
 * settup function
 **/
bool shouldSaveConfig = false;
bool forceConfigPortal = false;
WiFiManager wifiManager;

// Callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}
void setup_real_time_clock()
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
            ESP.reset();
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
            ESP.reset();
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
                digitalWrite(ledwifi, LOW);
                Serial.println("Disconnected");
            }
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Failure to establish connection after 10 sec. Will reattempt connection in 2 sec");
            digitalWrite(ledwifi, LOW);
            delay(2000);
        }
    }
}
void setup()
{
    pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    pinMode(Relay, OUTPUT);       // Khai bao chan output cua relay
    pinMode(button, INPUT);       //Khai bao button
    pinMode(ledwifi, OUTPUT);     //Khai bao led wifi
    pinMode(ledreset, OUTPUT);
     pinMode(l_m1, OUTPUT); // define OUTPUT led mode 
    pinMode(l_m2, OUTPUT);
    pinMode(l_m3, OUTPUT);
    pinMode(l_m4, OUTPUT);
    digitalWrite(Relay, LOW); //  Relay on 5V
    digitalWrite(button, LOW);
    // digitalWrite(ledreset, LOW);
    Serial.begin(115200);
    setup_wifi();
    setup_real_time_clock();
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    mode = 0;
}
/**
 ********************************      Loop function
 **/

void loop()
{
    reconnectIfNecessary();
    if (digitalRead(button) == HIGH)
  { int h;

    for ( h = 0; h <= 70; h++)
    {
        delay(100);
        if (digitalRead(button) == LOW)
        {
            // stop_all_Alarm_funtion();
            mode = 5;
            led_mode(l_m1, l_m2, l_m3, l_m4, HIGH, HIGH, HIGH, HIGH); // call function Turn/off led mode
            switchRelay(!relay_status);
            break;
        }
    }
    if (h == 71)
    {
        digitalWrite(ledreset, HIGH);
        digitalWrite(ledwifi, LOW);
        Serial.println("nut nhan reset wifi da duoc nhan");
        wifiManager.resetSettings();
        stop_all_Alarm_funtion();
        setup();
    }
  }

      if (!client.connected())
  {
      digitalWrite(ledwifi, LOW);
      reconnect();
    }
    client.loop();
    // publish temperature after 3 seconds
    unsigned long now = millis();
    if (now - lastMsg > 4000)
    {
        lastMsg = now;
        ++value;
        int analogValue = analogRead(outputpin);
        float millivolts = (analogValue / 1024.0) * 3300; //3300 is the voltage provided by NodeMCU
        float nearest = roundf(millivolts * 100) / 100;
        celsius = nearest / 10;

        snprintf(msg, MSG_BUFFER_SIZE, "%.2f", celsius);
        Serial.print("Publish message: ");
        Serial.println(msg);
        Serial.println(celsius);
        digitalWrite(ledreset, LOW);
        publish("sensor", msg);
        // digitalWrite(button, HIGH);
        digitalWrite(ledwifi, HIGH);
        Serial.println(mode);
        if((mode == 3) || (mode == 4))
        {
            digitalClockDisplay();
        }
    }

    if (mode == MODE_MANUAL)
    {
        // CODE MODE MANUAL HERE
    }
    else if (mode == MODE_AUTO_BALANCE)
    {
        int mean_celsius = (nhietmax+nhietmin+1)/2;
        if (chedo == false)
        {
            if ((celsius > nhietmax) || (celsius < nhietmin))
            {
                switchRelay(ON);
                Serial.println("che do False On");
                Serial.println(mean_celsius);
            }
            else 
            {
                celsius = mean_celsius;
                switchRelay(OFF);
                Serial.println("che do False Off");
                Serial.println(mean_celsius);
            }
        }
        if (chedo == true)
        {
            if ((celsius < nhietmax) && (celsius > nhietmin))
            {
                switchRelay(ON);
                Serial.println("che do True on");
                Serial.println(mean_celsius);
            }
            else
            {

                switchRelay(OFF);
                celsius = mean_celsius;
                Serial.println("che do True off");
                Serial.println(mean_celsius);
            }
        }
    }
    else if (mode == MODE_CYCLE)
    {
        // CODE MODE CYCLE HERE
        Alarm.delay(200);
       
    }
    else if (mode == MODE_DATETIME)
    {
        // CODE MODE CYCLE HERE
        Alarm.delay(200);
    }
    else if (mode == 0)
    {

        byte ledPin[] = {l_m1, l_m2, l_m3, l_m4};
        for (int i = -1; i < 4; i++)
        {
            digitalWrite(ledPin[i], HIGH); //Bật đèn
            digitalWrite(ledPin[i+1], HIGH);
            delay(200);                    // Dừng để các đèn LED sáng dần
            digitalWrite(ledPin[i], LOW);
            digitalWrite(ledPin[i+1], LOW);
        }

 
    }
}

/**
 * subscribe helper
 **/
void subscribe(char *subtopic)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + 1];
    sprintf(topic, "%s%s%s", subtopic, "/", device_serial_number);
    client.subscribe(topic);
    Serial.print("subscribed to topic: ");
    Serial.println(topic);
}

/**
 * publish helper
 **/
void publish(char *subtopic, char *msg)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + 1];
    sprintf(topic, "%s%s%s", subtopic, "/", device_serial_number);
    client.publish(topic, msg);
    Serial.print("published to topic: ");
    Serial.print(topic);
    Serial.print("; with message: ");
    Serial.println(msg);
}

/**
 * use to turn on/off relay
 * 
 **/
void switchRelay(bool status)
{
    if (relay_status != status)
    {
        if (status)
        {
            digitalWrite(Relay, HIGH);
            publish("turned_on", "1");
            Serial.println("relay's turned on");
        }
        else
        {
            digitalWrite(Relay, LOW);
            publish("turned_on", "0");
            Serial.println("relay's turned off");
        }
        relay_status = status;
    }
}

void stop_all_Alarm_funtion()
{
    for (int i = 0; i < 100; i++)
        {
            Alarm.free(weekly_Arlarm_id[i]); // free up ID
        }
     Alarm.free(My_cy_timer_start);
        Alarm.free(My_cy_timer_stop);
        Alarm.free(My_rt_timer_start);
        Alarm.free(My_rt_timer_repeat);
    switchRelay(OFF);
    Serial.println("da chay void stop_all_alram_funtion reset toan bo ngay gio cac mode ");
}

/**
 * 
 * 
 *
 **/
void mqttCallback(char *topic, byte *payload, unsigned int length)
{


    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print(length);
    Serial.println((char *)payload);
    
    
    if ((String(topic).startsWith("setting")) && (payload[0] == '{'))
    {

        char *json = (char *)payload;
        StaticJsonBuffer<2000> jsonBuffer;
        JsonObject &root = jsonBuffer.parseObject(json);
        mode = root["mode"];
        stop_all_Alarm_funtion();
        switch (mode)
        {
        case MODE_MANUAL:
            Serial.println("bat che do THU CONG");
            led_mode(l_m1, l_m2, l_m3, l_m4, HIGH, LOW, LOW, LOW); // call function Turn/off led mode

            break;
        case MODE_AUTO_BALANCE:
        {
            led_mode(l_m1, l_m2, l_m3, l_m4, LOW, HIGH, LOW, LOW);
            Serial.println("bat che do TU DONG");
            float min = root["setting"]["min"];
            float max = root["setting"]["max"];
            bool mode_on = root["setting"]["mode_on"];

            Serial.println(mode);
            Serial.println(min);
            Serial.println(max);
            Serial.println(mode_on);

            chedo = mode_on;
            nhietmax = max;
            nhietmin = min;
            Serial.println("Nhiet max:");
            Serial.println(nhietmax);
            Serial.println("Nhiet min:");
            Serial.println(nhietmin);
        }
        break;

        case MODE_CYCLE:
        {
            led_mode(l_m1, l_m2, l_m3, l_m4, LOW, LOW, HIGH, LOW);
            Serial.println("da bat mode 3");
            cy_run_time = root["setting"]["run_time"];
            cy_rest_time = root["setting"]["rest_time"];
            cy_apply_datetime = root["setting"]["apply_datetime"];
            cy_stop_datetime = root["setting"]["stop_datetime"];
            cy_repeat = root["setting"]["repeat"];
            Serial.println(cy_run_time);
            Serial.println(cy_rest_time);
            Serial.println(cy_apply_datetime);
            Serial.println(cy_stop_datetime);
            Serial.println(cy_repeat);
            if ((cy_apply_datetime - time(nullptr)) < 0 )
            {
                Run_Cy_Alarm();
            }else
            {
                My_cy_timer_start = Alarm.timerOnce(cy_apply_datetime - time(nullptr), Run_Cy_Alarm); // hen gio BAT  che do cycle time
            }
            My_cy_timer_stop = Alarm.timerOnce(cy_stop_datetime - time(nullptr), StopcycleMode);  // hen gio TAT che do cycle time
            Serial.println(time(nullptr));
            is_set_alarm_cycle = false;
            Serial.println("bat che do CYCLE TIME");
        }
        break;

        case MODE_DATETIME:
        {
            mode4_status = OFF;
            // ban dau set khong cho chay real time
            led_mode(l_m1, l_m2, l_m3, l_m4, LOW, LOW, LOW, HIGH);
            Serial.println("bat che do REAL TIME");

            long rt_apply_datetime = root["setting"]["apply_datetime"];
            long rt_stop_datetime = root["setting"]["stop_datetime"];
            rt_repeat = root["setting"]["repeat"];

            Serial.println(rt_apply_datetime);
            Serial.println(rt_stop_datetime);
            Serial.println(rt_repeat);
            int i=0;
            for (int week_index = 0; week_index < 7; week_index++)
            {
                int j = 0;
                while (root["setting"][weekly_time_name[week_index]][j])
                {
                    for (int k = 0; k <= 1; k++)
                    {
                        String hourString = root["setting"][weekly_time_name[week_index]][j][k];
                        int hour = hourString.substring(0, 2).toInt();
                        int minute = hourString.substring(3, 5).toInt();

                        Serial.print(hour);
                        Serial.print(minute);

                         weekly_Arlarm_id[i] = Alarm.alarmRepeat(
                                weekly_Arlarm_name[week_index],
                                hour,
                                minute,
                                0,
                                k == 0 ? RunRealtimeAlarm : StopRealtimeAlarm
                            );
                        i++;
                    }
                    // stop
                    j++;
                }
            }
            Serial.println();
            Serial.println("da set date time ca tuan thanh cong");

            if ((rt_apply_datetime - time(nullptr)) < 0)
            {
                set_real_time_start();
            }
            else
            {
                My_rt_timer_start = Alarm.timerOnce((rt_apply_datetime - time(nullptr)), set_real_time_start);
            }

            My_rt_timer_repeat =
                Alarm.timerOnce(
                    ((rt_apply_datetime - time(nullptr)) + 604800 + 604800*rt_repeat), stop_all_Alarm_funtion);

            is_set_alarm_date_time = false;
        }
        break;
        }
    }
    if ((mode == 1) && (payload[0] == '1'))
    {
        switchRelay(ON);
    }
    if ((mode == 1) && (payload[0] == '0'))
    {
        switchRelay(OFF);
    }
    
}

/**
 * 
 * 
 *
 **/
void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {

        digitalWrite(ledwifi, LOW); //neu mat ket noi thi den tat
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
void set_real_time_start()
{
    mode4_status = ON;
    switchRelay(ON);
    Serial.println("Alarm real time  bat dau hoat dong");
}

/**
 * 
 * 
 *
 **/
void StopcycleMode()
{
    Serial.println("Alarm cycle mode TAT");
    switchRelay(OFF);
    // mode = 1;
    cy_repeat = 0;
}
/**
 * 
 * 
 *
 **/
void Run_Cy_Alarm()
{
    
    Serial.println("Alarm:         Bat");
    switchRelay(ON);

    if (mode == 3)
    {
        Alarm.timerOnce(cy_run_time, Stop_Cy_Alarm);
    }
}
/**
 * 
 * 
 *
 **/
void Stop_Cy_Alarm()
{
    Serial.println("Alarm:        Tat");
    switchRelay(OFF);
    cy_repeat--;
    Serial.print("so lan lap lai con    ");
    Serial.println(cy_repeat);
    if ((mode == 3) && (cy_repeat > 0))
    {
        Alarm.timerOnce(cy_rest_time, Run_Cy_Alarm);
    }
}
void RunRealtimeAlarm()
{
   
    if (mode4_status == ON)
    {
        Serial.println("Alarm real time  delay ON");
        switchRelay(ON);
    }
}
void StopRealtimeAlarm()
{
    if (mode4_status == ON)
    {
        Serial.println("Alarm real time  delay OFF");
        switchRelay(OFF);
    }
}
void led_mode(int l1, int l2, int l3, int l4, int a, int b, int c, int d){//function led mode 
  
  digitalWrite(l1, a);
  digitalWrite(l2, b);
  digitalWrite(l3, c);
  digitalWrite(l4, d);
}
/**
 * 
 * 
 *
 **/
void MorningAlarm()
{
    Serial.println("Alarm:         HOAT DONG");
}

/**
 * 
 * 
 *
 **/
void EveningAlarm()
{
    Serial.println("Alarm: - turn lights on");
    switchRelay(OFF);
}

/**
 * 
 * 
 *
 **/
void WeeklyAlarm()
{
    Serial.println("Alarm: - its Monday Morning");
}

/**
 * 
 * 
 *
 **/
void ExplicitAlarm()
{
    Serial.println("Alarm: - this triggers only at the given date and time");
}

/**
 * 
 * 
 *
 **/
void Repeats()
{
    Serial.println("15 second timer");
}

/**
 * 
 * 
 *
 **/
void Repeats2()
{
    Serial.println("2 second timer");
}

/**
 * 
 * 
 *
 **/
void OnceOnly()
{
    Serial.println("This timer only triggers once, stop the 2 second timer");
    // use Alarm.free() to disable a timer and recycle its memory.
    Alarm.free(id);
    // optional, but safest to "forget" the ID after memory recycled
    id = dtINVALID_ALARM_ID;
    // you can also use Alarm.disable() to turn the timer off, but keep
    // it in memory, to turn back on later with Alarm.enable().
}

/**
 * 
 * 
 *
 **/
void digitalClockDisplay()
{
    time_t tnow = time(nullptr);
    Serial.println(tnow);
    Serial.println(ctime(&tnow));

    //    Serial.println(ctime(&tnow));
}