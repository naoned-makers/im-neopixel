#include <ESP8266WiFi.h>  // Include the Wi-Fi library
#include <ESP8266mDNS.h>  // Include the mDNS library
#include <PubSubClient.h> // Include the mqtt client library
#include <Adafruit_NeoPixel.h> // Include the neopixel library
#include <ArduinoJson.h> // Include the json library v5

#define wifi_ssid "naonedmakers"
#define wifi_password "xxxxxx"
#define mqtt_server "ironman.local"
#define im_neopixel_topic "im/event/esp8266/neopixel/#" //Topic neopixel

#define STRIPA_PIN 4
#define STRIPA_LENGTH 16
#define STRIPB_PIN 3
#define STRIPB_LENGTH 16

WiFiClient wifiClient; // Create an instance of the ESP8266WiFi class, called 'espClient'
PubSubClient mqttClient(wifiClient);

//#######################################################################################
//###########FROM https://learn.adafruit.com/multi-tasking-the-arduino-part-3/ ##########
//#######################################################################################

// Pattern types supported:
enum  pattern { NONE =0, RAINBOW_CYCLE=1, THEATER_CHASE=2, COLOR_WIPE=3, SCANNER=4, FADE=5, FIX=6 };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                case FIX:
                    ColorFixUpdate();
                    break;
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        }
        show();
        Increment();
    }
 
    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }
 
    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }

    // Initialize for a ColorSet
    void ColorFix(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FIX;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Color Set Pattern
    void ColorFixUpdate()
    {
        ColorSet(Color1);
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};
//########################################################################################
//#########################################################################################
//#########################################################################################
void StripAComplete();
void StripBComplete();
 
// Define some NeoPatterns for strip as well as some completion routines
NeoPatterns StripA(STRIPA_LENGTH, STRIPA_PIN, NEO_GRB + NEO_KHZ800, &StripAComplete);
NeoPatterns StripB(STRIPB_LENGTH, STRIPB_PIN, NEO_GRB + NEO_KHZ800, &StripBComplete);

// Memory pool for JSON object tree.
//
// Inside the brackets, 200 is the size of the pool in bytes.
// Don't forget to change this value to match your JSON document.
// Use arduinojson.org/assistant to compute the capacity.
StaticJsonBuffer<200> jsonBuffer;

// Initialize everything and prepare to start
void setup()
{
  Serial.begin(9600);
  Serial.print("setup");
  setup_wifi();

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);

  // Initialize the NeoPixel library for all the Strips .
  StripA.begin();
  StripB.begin();
    
  // Kick off a pattern
  StripA.ColorFix(StripA.Color(34,34,255),100000);
  StripB.ColorFix(StripB.Color(34,34,255),100000);
  StripA.Update();
  StripB.Update();
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
  Serial.println("Wifi connected");
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
  Serial.print("Message arrived ");
  //get target pin
  char pin = ((char)topic[26]);
  Serial.print("Strip ");
  Serial.print(pin);
  // décode le message - decode payload message
  char inData[length];
  Serial.print(" payload: ");
  for(int i =0; i<length; i++){
    Serial.print((char)payload[i]);
    inData[i] = (char)payload[i];
  }
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.parseObject(inData);  
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do root["time"].as<long>();
  //long color = root["color"]; strtol( color.c_str(), NULL, 16);

  // root["pattern"]
  int interval = root["interval"] | -1;
  long color1 = root["color1"] | -1L;
  long color2 = root["color2"] | -1L;
  int pattern = root["pattern"] | -1;
  Serial.print(" color1:");
  Serial.print(color1);
  Serial.print(" color2:");
  Serial.print(color2);
  Serial.print(" interval:");
  Serial.print(interval);
  Serial.print(" pattern:");
  Serial.print(pattern);
  
  NeoPatterns *strip;
  if(pin=='B'){
    Serial.print(" B ");
    strip = &StripB;
  }else{
    Serial.print(" A ");
    strip = &StripA;
  }
  if(color1 != -1){
    strip->Color1 = color1;
  }
  if(color2 != -1){
    strip->Color2 = color2;
  }
  if(interval != -1){
    strip->Interval = interval;
  }
  if(pattern == RAINBOW_CYCLE){
    Serial.println(" RAINBOW_CYCLE");
    strip->ActivePattern = RAINBOW_CYCLE;
    strip->Direction = FORWARD;
    strip->Index = 0;
    strip->TotalSteps = 255;
  }else if(pattern == THEATER_CHASE){
    Serial.println(" THEATER_CHASE");
    strip->ActivePattern = THEATER_CHASE;
    strip->Direction = FORWARD;
    strip->Index = 0;
    strip->TotalSteps = strip->numPixels();
    //strip->Color1 = color1;
    //strip->Color2 = color2;
  }else if(pattern == COLOR_WIPE){
    Serial.println(" COLOR_WIPE");
    strip->ActivePattern = COLOR_WIPE;
    strip->Direction = FORWARD;
    strip->Index = 0;
    strip->TotalSteps = strip->numPixels();
    //strip->Color1 = color;
  }else if(pattern == SCANNER){
    Serial.println(" SCANNER");
    strip->ActivePattern = SCANNER;
    strip->Direction = FORWARD;
    strip->Index = 0;
    strip->TotalSteps = (strip->numPixels() - 1) * 2;
    //strip->Color1 = color1;
  }else if(pattern == FADE){
    Serial.println(" FADE");
    strip->ActivePattern = FADE;
    strip->Direction = FORWARD;
    strip->Index = 0;
    strip->TotalSteps = strip->numPixels();//specifies how many steps it should take to get from color1 to color2.
    //strip->Color1 = color1;
    //strip->Color2 = color2;
  }else if(pattern == FIX){
    Serial.println(" FIX");
    strip->ActivePattern = FIX;
    //strip->Direction = FORWARD;
    //strip->Index = 0;
    //strip->TotalSteps = strip->numPixels();//specifies how many steps it should take to get from color1 to color2.
    //strip->Color1 = color1;
    //strip->Color2 = color2;
  }else{
    Serial.print(" pattern:");
    Serial.println(pattern);
  }
}

void loop()
{
  // Serial.println("loop ");
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("wifi not connected.");
    delay(500);
    //WiFi.disconnect();
    //setup_wifi();
    ESP.reset();
  }
  if (!mqttClient.connected())
  {
    reconnectMqtt();
  }
  mqttClient.loop();
  // Update the rings.
  StripA.Update();
  StripB.Update();
}

//------------------------------------------------------------
//Completion Routines - get called on completion of a pattern
//------------------------------------------------------------
// strip A Completion Callback
void StripAComplete()
{
  if(StripA.ActivePattern == FADE){
    StripA.Reverse();
  }
}
 
// Strip B Completion Callback
void StripBComplete()
{
  if(StripB.ActivePattern == FADE){
    StripB.Reverse();
  }
}