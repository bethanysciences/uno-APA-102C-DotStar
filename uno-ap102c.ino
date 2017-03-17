#include <RTCZero.h>
#include <Process.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>

#define DATAPIN     4
#define CLOCKPIN    5
#define NUMPIXELS   150
#define COLOR       DOTSTAR_BGR   	   // BRG RGB GRB
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, COLOR);
RTCZero rtc;
Process date;

uint32_t RED = strip.Color( 255 , 0   , 0   );
uint32_t GRN = strip.Color( 0   , 255 , 0   );
uint32_t BLU = strip.Color( 0   , 0   , 255 );
uint32_t YEL = strip.Color( 255 , 255 , 0   );
uint32_t CYN = strip.Color( 0   , 255 , 255 );
uint32_t PRP = strip.Color( 255 , 0   , 255 );
uint32_t GLD = strip.Color( 255 , 211 , 0   );
uint32_t WHT = strip.Color( 255 , 255 , 255 );
uint32_t BLK = strip.Color( 0   , 0   , 0   );

int SEGMENT = 10;               // # of Segments
int LED = 15;                   // # LEDS / segment
bool OnOff = 0;

int pinMatrix[10][15] = {
  {GRN,RED,GRN,GRN,YEL,GRN,GRN,BLU,GRN,GRN,GRN,GRN,RED,GRN,BLU},
  {GRN,GRN,YEL,GRN,GRN,GRN,RED,GRN,BLU,GRN,GRN,RED,GRN,BLU,GRN},
  {GRN,GRN,YEL,GRN,RED,GRN,BLU,GRN,GRN,RED,GRN,GRN,BLU,GRN,GRN},
  {GRN,RED,GRN,YEL,GRN,GRN,GRN,BLU,GRN,GRN,GRN,RED,GRN,GRN,BLU},
  {RED,GRN,GRN,GRN,BLU,GRN,YEL,GRN,GRN,GRN,GRN,RED,GRN,BLU,GRN},
  {GRN,RED,GRN,GRN,GRN,BLU,GRN,GRN,RED,GRN,GRN,BLU,GRN,GRN,YEL},
  {GRN,GRN,BLU,GRN,RED,GRN,YEL,GRN,GRN,RED,GRN,GRN,BLU,GRN,YEL},
  {GRN,BLU,GRN,RED,GRN,BLU,GRN,GRN,RED,GRN,BLU,YEL,GRN,RED,GRN},
  {GRN,YEL,GRN,GRN,RED,GRN,GRN,BLU,GRN,GRN,RED,GRN,GRN,GRN,BLU},
  {GRN,YEL,GRN,RED,GRN,GRN,GRN,GRN,BLU,GRN,GRN,RED,GRN,BLU,GRN}, 
};

void setup() {
  Bridge.begin();
  rtc.begin();
  if (!date.running()) {
    date.begin("date");
    date.addParameter("+%T");
    date.run();
  }
  String timeString = date.readString();
  int colon1 = timeString.indexOf(":");
  int colon2 = timeString.lastIndexOf(":");
  String hrsString = timeString.substring(0, colon1);
  String minString = timeString.substring(colon1 + 1, colon2);
  String secString = timeString.substring(colon2 + 1);
  int hours = hrsString.toInt();
  int minutes = minString.toInt();
  int seconds = secString.toInt();
  rtc.setTime(hours, minutes, seconds);
  strip.begin();
}

void loop() {
  if (rtc.getHours() >= 17) {
    if (OnOff == 0) {
      razzle(); 
      OnOff = 1;
    }
    dazzle();
  }
  else {
    bake(); OnOff = 0;
  }
}

void razzle() {
  int i=0;
  for(int s = 0; s <= SEGMENT ; s++) {
    for(int l = 0; l <= LED ; l++) {
      strip.setPixelColor(i, pinMatrix[s][l]);
      i++;
    }
  }
  strip.show();
}

void dazzle() {
  for(int i=1;   i<20;  i++) {
    int rPixel = random(31,120);
    int cClolor = strip.getPixelColor(rPixel);
    strip.setPixelColor(rPixel, WHT);
    strip.show();
    delay(300);
    strip.setPixelColor(rPixel, cClolor);
    strip.show();
  }
}

void bake() {
  for(int i = 1; i < NUMPIXELS ; i++) strip.setPixelColor(i, BLK);
  strip.show();
}

