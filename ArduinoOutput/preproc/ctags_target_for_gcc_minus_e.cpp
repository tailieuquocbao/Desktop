# 1 "/home/quocbao/Desktop/4replay_arduino/main.ino"
# 2 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 3 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 4 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 5 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 6 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 7 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 8 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2

# 10 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 11 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 12 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 13 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2

# 15 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 16 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 17 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2

# 19 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2
# 20 "/home/quocbao/Desktop/4replay_arduino/main.ino" 2

Settup_wifi WIFI_AFFOZ;

double degreesC;

byte willQoS = 1; // gui tin hieu len sever khi mat ket noi
boolean willRetain = false;
const char *willTopic = "connected/1921681229";
const char *device_serial_number = "1921681229";
const char *device_relay_number[] = {"0", "1", "2", "3"};

const char *willMessage = "0";

#define MSG_BUFFER_SIZE (50) /* khai bao bo nho so luong ki tu gui len cho sever*/
char msg[(50) /* khai bao bo nho so luong ki tu gui len cho sever*/];
int relayNo = 0;

// DigitalPin ledWifi(18);

const char *mqtt_server = "farm-dev.affoz.com";

AlarmId id; // setup cho ham free alarm dung de reset bo nho alarm
WiFiClient espClient;
PubSubClient client(espClient);

const int NUMBER_OF_SENSOR = 1;
Sensor sensors[NUMBER_OF_SENSOR] = {
    Sensor(32, client)};

const int NUMBER_OF_RELAY = 4;

NewRelay relays[NUMBER_OF_RELAY] = {
    // NewRelay(4, client),
    // NewRelay(16, client),
    NewRelay(14, 2, client),
    NewRelay(27, 2, client),
    NewRelay(26, 2, client),
    NewRelay(25, 2, client)};
// const int NUMBER_OF_BUTTON = 4;
// Button buttons[NUMBER_OF_BUTTON] = {
//     Button(5, client),
//     Button(2, client),
//     Button(5, client),
//     Button(2, client)

// };

bool status[] = {0, 1, 2, 3};
ButtonRS buttons(16);

/*
wifi: AFFOZ_Office
password: &L2I@PxQ
{"mode":1,"setting":{"min":20,"max":33,"mode_on":true}}
{"mode":2,"setting":{"min":20,"max":33,"mode_on":true}}
{"mode":3,"setting":{"run_time":4,"rest_time":2,"apply_datetime":1587124290,"stop_datetime":1687124290,"repeat":100}}
{"mode":4,"setting":{"mon":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"tue":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"wed":[["12:04","12:12"],["11:59","12:12"],["18:00","19:00"]],"thu":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"fri":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sat":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"sun":[["11:59","12:12"],["11:59","12:12"],["18:00","19:00"]],"apply_datetime":1587124290,"stop_datetime":1587124290,"repeat":1}}

      http://www.hivemq.com/demos/websocket-client/?#data
*/

// ButtonRS buttons(19);

void setupRealTimeClock()
{
    configTime(0, 0, "0.vn.pool.ntp.org", "3.asia.pool.ntp.org", "0.asia.pool.ntp.org");
    //set mui gio cua viet nam
    //Get JSON of Olson to TZ string using this code https://github.com/pgurenko/tzinfo
    setenv("TZ", "WIB-7", 1);
    tzset();
    Serial.print("Clock before sync: ");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    randomSeed(micros());
}

void setup()
{
    // wifiManager.resetSettings();
    Serial.begin(115200);
    adc1_config_width(ADC_WIDTH_BIT_12);
    buttons.resetWifi();
    // ledWifi.begin();
    WIFI_AFFOZ.settingwifi();
    setupRealTimeClock();
    client.setServer(mqtt_server, 1883);
    client.setCallback(mqttCallback);
    for (int i = 0; i < NUMBER_OF_RELAY; i++)
    {
        relays[i].clearTimer();
    }
}

void loop()
{
    WIFI_AFFOZ.reconnectIfNecessary();
    // Button();
    if (!client.connected())
    {
        // ledWifi.on();
        reconnect();
    }
    client.loop();
    buttons.resetWifi();
    // if (button1.isPressed())
    // {
    //     delay(20);
    //     while ( button1.isPressed() ) {};
    //     relays[relayNo].status() == false? relays[relayNo].on(): relays[relayNo].off();
    // }

    for (int i = 0; i < NUMBER_OF_RELAY; i++)
    {
        relays[i].loop();
    }

    for (int i = 0; i < NUMBER_OF_SENSOR; i++)
    {
        float celsius = sensors[i].value();
        snprintf(msg, (50) /* khai bao bo nho so luong ki tu gui len cho sever*/, "%.2f", celsius);
        publish_sensor("sensor", msg);
    }

    // for (int i = 0; i < NUMBER_OF_BUTTON; i++)
    // {
    //     buttons[i].loop();
    // }

    for (int i = 0; i < NUMBER_OF_RELAY; i++)
    {
        if (relays[i].status() != status[i])
        {
            if (relays[i].status() == false)
            {
                publish("turned_on", "0");
                status[i] = relays[i].status();
            }
            else if (relays[i].status() == true)
            {
                publish("turned_on", "1");
                status[i] = relays[i].status();
            }
        }
    }
}
void mqttCallback(char *topic, byte *payload, unsigned int length)
{

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print(length);
    Serial.println((char *)payload);
    for (int i = 0; i < NUMBER_OF_RELAY; i++)
    {
        String s = String(topic);
        char last = s.charAt(s.length() - 1);
        int a = String(last).toInt();
        if (a == i)
        {
            relayNo = i;
            char *json = (char *)payload;
            Serial.print("da nhan duoc setting ");
            Serial.println(relayNo);
            relays[relayNo].applySetting(json);
        }
    }
}

void subscribe(char *subtopic)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + strlen(device_relay_number[0]) + 1];
    for (int i = 0; i < 4; i++)
    {
        sprintf(topic, "%s%s%s%s%s", subtopic, "/", device_serial_number, "/", device_relay_number[i]);
        client.subscribe(topic);
    }

    Serial.print("subscribed to topic: ");
    Serial.println(topic);
}
void publish(char *subtopic, char *msg)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + strlen(device_relay_number[0]) + 1];
    sprintf(topic, "%s%s%s%s%s", subtopic, "/", device_serial_number, "/", device_relay_number[relayNo]);
    client.publish(topic, msg);
    Serial.print("published to topic: ");
    Serial.print(topic);
    Serial.print("; with message: ");
    Serial.println(msg);
}
void publish_sensor(char *subtopic, char *msg)
{
    char topic[strlen(subtopic) + strlen(device_serial_number) + 1];
    sprintf(topic, "%s%s%s", subtopic, "/", device_serial_number);
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

        // ledWifi.on();
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), 16);
        // Attempt to connect
        if (client.connect(clientId.c_str(), willTopic, willQoS, willRetain, willMessage))
        {
            Serial.println("connected");
            publish("connected", "1"); //neu ket noi dc thi gui len sever connected/192168110 [1]
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
