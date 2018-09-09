#include <ESP8266WiFi.h>  // Include the Wi-Fi library
#include <ESP8266mDNS.h>  // Include the mDNS library
#include <PubSubClient.h> // Include the mqtt client library
#include <Adafruit_NeoPixel.h> // Include the neopixel library

#define wifi_ssid "naonedmakers"
#define wifi_password "davigust"
#define mqtt_server "ironman.local"
#define im_neopixel_topic "im/event/esp8266/neopixel/#" //Topic neopixel

#define PINA 0
#define PINB 2

WiFiClient wifiClient; // Create an instance of the ESP8266WiFi class, called 'espClient'
PubSubClient mqttClient(wifiClient);

Adafruit_NeoPixel stripA = Adafruit_NeoPixel(16, PINA, NEO_GRB + NEO_KHZ800);
uint32_t colorA = 255;//blue R,G,B into packed 32-bit RGB color
char animA = '1';//animation codec 1 = on
int waitA = 1000;
Adafruit_NeoPixel stripB = Adafruit_NeoPixel(16, PINB, NEO_GRB + NEO_KHZ800);
uint32_t colorB = 255;//blue R,G,B into packed 32-bit RGB color
char animB ='1';
int waitB = 1000;

void setup()
{
  Serial.begin(9600);
  Serial.print("setup");
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
  stripA.begin();
  stripA.show();
  stripB.begin();
  stripB.show();
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("connected");
  MDNS.begin("neopixel"); // Start the mDNS responder for neopixel.local
}

//Reconnexion
void reconnectMqtt()
{
  //Boucle jusqu'à obtenur une reconnexion
  while (!mqttClient.connected())
  {
    Serial.print("Connexion au serveur MQTT...");
    if (mqttClient.connect("im-neopixel"))
    {
      Serial.println("OK");
      mqttClient.subscribe(im_neopixel_topic); 
    }
    else
    {
      Serial.print("KO, erreur : ");
      Serial.print(mqttClient.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Affiche le topic entrant - display incoming Topic
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  //get target pin
  char pin = ((char)topic[26]);
  Serial.print("pin ");
  Serial.print(pin);
  // décode le message - decode payload message
  int payloadLength = sizeof(payload);
  String waitStr = "";
  for(int i=1; i<4; i++){
    waitStr += (char)payload[i];
  }
  Serial.print("wait ");
  Serial.print(waitStr.toInt());
  String color = "";
  for(int i=4; i<10; i++){
    color += (char)payload[i];
  }
  Serial.print("color ");
  Serial.print(color);
  if(pin=='B'){
    animB = (char)payload[0];
    waitB = waitStr.toInt();
    colorB = strtol( color.c_str(), NULL, 16);
  }else{
    //default pin A
    animA = (char)payload[0];
    waitA = waitStr.toInt();
    colorA = strtol( color.c_str(), NULL, 16);
  }
}

void neopixelOn(Adafruit_NeoPixel pixelStrip,uint32_t c){
  // set the colors for the strip
   for( int i = 0; i < 16; i++ )
   {
       pixelStrip.setPixelColor(i,c);
   }
   // show all pixels  
   pixelStrip.show();
   // wait for a short amount of time -- sometimes a short delay of 5ms will help
   // technically we only need to execute this one time, since we aren't changing the colors but we will build on this structure
   delay(10);
}
void neopixelOff(Adafruit_NeoPixel pixelStrip){
  // set the colors for the strip
   for( int i = 0; i < 16; i++ )
   {
       pixelStrip.setPixelColor(i, 0, 0, 0);
   }
   // show all pixels  
   pixelStrip.show();
   // wait for a short amount of time -- sometimes a short delay of 5ms will help
   // technically we only need to execute this one time, since we aren't changing the colors but we will build on this structure
   delay(10);
}
//Theatre-style crawling lights.
void neopixelChase(Adafruit_NeoPixel pixelStrip,uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < pixelStrip.numPixels(); i=i+3) {
        pixelStrip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      pixelStrip.show();

      delay(wait);

      for (uint16_t i=0; i < pixelStrip.numPixels(); i=i+3) {
        pixelStrip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}
void neopixelBeat(Adafruit_NeoPixel pixelStrip,uint32_t c, uint8_t wait) {
  //strip.setBrightness(ii);
}


void loop()
{
  Serial.print("loop ");
  if (!mqttClient.connected())
  {
    reconnectMqtt();
  }
  mqttClient.loop();
  //change the animation
  if(animA=='1'){
    neopixelOn(stripA,colorA);  
  }else if(animA=='b'){
    neopixelBeat(stripA,colorA,waitA);  
  }else if(animA=='c'){
    neopixelChase(stripA,colorA, waitA);  
  }else {
    //default off
    neopixelOff(stripA);  
  }
  if(animB=='1'){
    neopixelOn(stripB,colorB);  
  }else if(animB=='b'){
    neopixelBeat(stripB,colorB,waitB);  
  }else if(animB=='c'){
    neopixelChase(stripB,colorB,waitB);  
  }else {
    //default off
    neopixelOff(stripB);  
  }
  delay(100);
}
