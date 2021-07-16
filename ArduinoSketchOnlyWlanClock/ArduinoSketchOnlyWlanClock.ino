#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#endif

//#include <DS3231_Simple.h>


// Create a variable to hold the time data 
int intMin;
int intHour;
int intTimeZone = 2;
int intConnectionError;


// Which pin on the Arduino is connected to the NeoPixels?
#define LEDCLOCK_PIN    2
#define LEDDOWNLIGHT_PIN    5

// How many NeoPixels are attached to the Arduino?
#define LEDCLOCK_COUNT 252
#define LEDDOWNLIGHT_COUNT 14

//(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
// or 3 hex byte 00 -> ff for RGB eg 0x123456 for red=12(hex) green=34(hex), and green=56(hex) 
// this hex method is the same as html colour codes just with "0x" instead of "#" in front
uint32_t savedColor = 0x00FF00;
uint32_t overAllColor;
uint32_t clockMinuteColour;
uint32_t clockHourColour;

int clockFaceBrightness = 0;

// Declare our NeoPixel objects:
Adafruit_NeoPixel stripClock(LEDCLOCK_COUNT, LEDCLOCK_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel stripDownlighter(LEDDOWNLIGHT_COUNT, LEDDOWNLIGHT_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


const int maxBrightness = 80;

//Amount of False internet
int FalseTry = 0;

//CheckTimeChange
int AmountSameTime = 0;
int LastTime = 0;

//Smoothing of the readings from the light sensor so it is not too twitchy
const int numReadings = 5;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
long total = 0;                  // the running total
long average = 0;                // the average


////-----WLan und NTP server connection

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "AUMM_G";  //  your network SSID (name)
char pass[] = "W45532570ff_1";       // your network password


unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "ch.pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


void setup() {

  Serial.begin(9600);
  Serial.println();
  Serial.println();

  

  //Wlan connection
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  
    Serial.println("Starting UDP");
    udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(udp.localPort());

  //LED Initialation
    stripClock.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
    stripClock.show();            // Turn OFF all pixels ASAP
    stripClock.setBrightness(10); // Set inital BRIGHTNESS (max = 80)
   
  
    stripDownlighter.begin();           // INITIALIZE NeoPixel stripClock object (REQUIRED)
    stripDownlighter.show();            // Turn OFF all pixels ASAP
    stripDownlighter.setBrightness(0); // Set BRIGHTNESS (max = 80)
  
    //smoothing
      // initialize all the readings to 0:
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readings[thisReading] = 0;
    }
  
}

void loop() {

  //read the time
  readTheTime();

  //Check amount of False Response
  if(FalseTry > 10){
    overAllColor = 0xFF0000;
  }else{
    overAllColor = savedColor;
  }

  //Transfer Color to Minute and Hour
  clockMinuteColour = overAllColor;
  clockHourColour = overAllColor;

  //Check how often the new Time is the same
  //Somtimes the response give back 0828 without reason. To solve this the Time will be only displayed if the new time is 5 times the same
    if(FalseTry <= 10){
      if(LastTime != intMin){
        LastTime = intMin;
        AmountSameTime = 0;
        Serial.print("Reset AmountSameTime");
     }else{
        AmountSameTime += 1;
        Serial.print("AmountSameTime:");
        Serial.println(AmountSameTime);
     }
    }
     
    if((AmountSameTime == 5)||(FalseTry > 10)){
      Serial.print("DisplayNewTime");
      displayTheTime();
    }
    

  
  //Record a reading from the light sensor and add it to the array
  readings[readIndex] = analogRead(A0); //get an average light level from previouse set of samples
  Serial.print("Light sensor value added to array = ");
  Serial.println(readings[readIndex]);
  readIndex = readIndex + 1; // advance to the next position in the array:

  // if we're at the end of the array move the index back around...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  //now work out the sum of all the values in the array
  int sumBrightness = 0;
  for (int i=0; i < numReadings; i++)
    {
        sumBrightness += readings[i];
    }
  Serial.print("Sum of the brightness array = ");
  Serial.println(sumBrightness);

  // and calculate the average: 
  int lightSensorValue = sumBrightness / numReadings;
  Serial.print("Average light sensor value = ");
  Serial.println(lightSensorValue);


  //set the brightness based on ambiant light levels !!Max Brigness 80. Stromverbrauch sonst zu hoch 1024 max sensor
  clockFaceBrightness = map(lightSensorValue,10, 70, 1, maxBrightness); 
  
  if(clockFaceBrightness > maxBrightness){
    clockFaceBrightness = maxBrightness;
  }
  if(clockFaceBrightness < 1){
    clockFaceBrightness = 1;
  }
  stripClock.setBrightness(clockFaceBrightness); // Set brightness value of the LEDs
  Serial.print("Mapped brightness value = ");
  Serial.println(clockFaceBrightness);

  
  
  //stripClock.setPixelColor(0,overAllColor);
  
  stripClock.show();

   //(red * 65536) + (green * 256) + blue ->for 32-bit merged colour value so 16777215 equals white
  stripDownlighter.fill(16777215, 0, LEDDOWNLIGHT_COUNT);
  stripDownlighter.show();

  //delay(5000);   //this 5 second delay to slow things down during testing
  
}


void readTheTime(){
 //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    FalseTry += 1;
    Serial.println("no packet yet");
  }
  else {
    FalseTry = 0;
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    epoch = epoch + (intTimeZone * 3600);
    intHour = ((epoch  % 86400L) / 3600);
    if (intHour == 24){
      intHour = 0;
    }
    Serial.print(intHour); // print the hour (86400 equals secs per day)
    Serial.print(':');
    intMin = (epoch  % 3600) / 60;
    if ( (intMin) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(intMin); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }

}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void displayTheTime(){

  stripClock.clear(); //clear the clock face 

  
  int firstMinuteDigit = intMin % 10; //work out the value of the first digit and then display it
  displayNumber(firstMinuteDigit, 0, clockMinuteColour);

  
  int secondMinuteDigit = floor(intMin / 10); //work out the value for the second digit and then display it
  displayNumber(secondMinuteDigit, 63, clockMinuteColour);  

  int firstHourDigit = intHour % 10; //work out the value of the first digit and then display it
  displayNumber(firstHourDigit, 126, clockHourColour);

  
  int secondHourDigit = floor(intHour / 10); //work out the value for the second digit and then display it
  displayNumber(secondHourDigit, 189, clockHourColour);  

  stripClock.show();

  }


void displayNumber(int digitToDisplay, int offsetBy, uint32_t colourToUse){
    switch (digitToDisplay){
    case 0:
      digitZero(offsetBy,colourToUse);
      break;
    case 1:
      digitOne(offsetBy,colourToUse);
      break;
    case 2:
    digitTwo(offsetBy,colourToUse);
      break;
    case 3:
    digitThree(offsetBy,colourToUse);
      break;
    case 4:
    digitFour(offsetBy,colourToUse);
      break;
    case 5:
    digitFive(offsetBy,colourToUse);
      break;
    case 6:
    digitSix(offsetBy,colourToUse);
      break;
    case 7:
    digitSeven(offsetBy,colourToUse);
      break;
    case 8:
    digitEight(offsetBy,colourToUse);
      break;
    case 9:
    digitNine(offsetBy,colourToUse);
      break;
    default:
     break;
  }
}

void digitZero(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 27);
    stripClock.fill(colour, (36 + offset), 27);
}

void digitOne(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 9);
    stripClock.fill(colour, (36 + offset), 9);
}

void digitTwo(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 18);
    stripClock.fill(colour, (27 + offset), 9);
    stripClock.fill(colour, (45 + offset), 18);
}

void digitThree(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 18);
    stripClock.fill(colour, (27 + offset), 27);
}

void digitFour(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 9);
    stripClock.fill(colour, (18 + offset), 27);
}

void digitFive(int offset, uint32_t colour){
    stripClock.fill(colour, (9 + offset), 45);
}

void digitSix(int offset, uint32_t colour){
    stripClock.fill(colour, (9 + offset), 54);
}

void digitSeven(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 18);
    stripClock.fill(colour, (36 + offset), 9);
}

void digitEight(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 63);
}

void digitNine(int offset, uint32_t colour){
    stripClock.fill(colour, (0 + offset), 45);
}
