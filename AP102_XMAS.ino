#include <FastGPIO.h>
#define APA102_USE_FAST_GPIO
#include <APA102.h>
#include <EEPROM.h>

const uint8_t dataPin =   4;
const uint8_t clockPin =  5;
APA102<dataPin, clockPin> ledStrip;
#define LED_COUNT 210
#define NEXT_PATTERN_BUTTON_PIN   2   // button between this pin and ground
#define AUTOCYCLE_SWITCH_PIN      3   // switch between this pin and ground
rgb_color colors[LED_COUNT];
unsigned int loopCount = 0;
unsigned int seed = 0;                // initialize random number generator

#define NUM_STATES                7   // patterns to cycle through
enum Pattern {
  WarmWhiteShimmer    = 0,  // warm white shimmer for 300 loops fade / last 70
  RandomColorWalk     = 1,  // alt red grn randomly walk to colors 400 loops fade / last 80
  TraditionalColors   = 2,  // repeat red, grn, org, blu mag slowly moving 400 loops
  ColorExplosion      = 3,  // random color bursts radiate out 630 loops
  Gradient            = 4,  // red -> white -> green -> white -> red ... gradiant that scrolls
  BrightTwinkle       = 5,  // random LEDs light up brightly and fade away
  Collision           = 6,  // grow towards each other from the two ends of the strips
  AllOff              = 255
};

char* PatternStr [] = {"Warm White Shimmer", "Random Color Walk", "Traditional Colors", "Color Explosion", "Gradient", 
                        "Bright Twinkle", "Collision", "All Off"};
unsigned char pattern = AllOff;
unsigned int maxLoops;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 8; i++){seed += analogRead(i);}
  seed += EEPROM.read(0);                         // get part of the seed from EEPROM
  randomSeed(seed);                               // saverandom number to EEPROM for seed
  EEPROM.write(0, random(256));
  pinMode(AUTOCYCLE_SWITCH_PIN, INPUT_PULLUP);    // freeze the cycle on current pattern
  pinMode(NEXT_PATTERN_BUTTON_PIN, INPUT_PULLUP); // advance to the next pattern
  delay(10);
}

void loop() {
  uint8_t startTime = millis();
  handleNextPatternButton();
  if (loopCount == 0) {
    for (int i = 0; i < LED_COUNT; i++) {colors[i] = (rgb_color){0, 0, 0};}
  }
  if (pattern == WarmWhiteShimmer || pattern == RandomColorWalk) {
    if (loopCount % 6 == 0) {seed = random(30000);}
    randomSeed(seed);
  }
  switch (pattern) {
    case WarmWhiteShimmer:    
      maxLoops = 300; warmWhiteShimmer(loopCount > maxLoops - 70); break;
    case RandomColorWalk:     
      maxLoops = 400; randomColorWalk(loopCount == 0 ? 1 : 0, loopCount > maxLoops - 80); break;
    case TraditionalColors:   
      maxLoops = 400; traditionalColors(); break;
    case ColorExplosion:      
      maxLoops = 630; colorExplosion((loopCount % 200 > 130) || (loopCount > maxLoops - 100)); break;
    case Gradient:            
      maxLoops = 250; gradient(); delay(6); break;
    case BrightTwinkle:       
      maxLoops = 1200;
      if (loopCount < 400) {brightTwinkle(0, 1, 0);      }
      else if (loopCount < 650) {brightTwinkle(0, 2, 0); }
      else if (loopCount < 900) {brightTwinkle(1, 2, 0); }
      else {brightTwinkle(1, 6, loopCount > maxLoops - 100); }
      break;
    case Collision:     
      if (!collision()) {maxLoops = loopCount + 2;} break;}
  ledStrip.write(colors, LED_COUNT);
  while((uint8_t)(millis() - startTime) < 20) { }
  loopCount++;
  if (loopCount >= maxLoops && digitalRead(AUTOCYCLE_SWITCH_PIN)) {
    loopCount = 0;
    pattern = ((unsigned char)(pattern+1))%NUM_STATES;
    Serial.println(PatternStr[pattern]);
  }
}

void handleNextPatternButton() {
  if (digitalRead(NEXT_PATTERN_BUTTON_PIN) == 0) {
    while (digitalRead(NEXT_PATTERN_BUTTON_PIN) == 0) {
      while (digitalRead(NEXT_PATTERN_BUTTON_PIN) == 0);
      delay(10);  // debounce the button
    }
    loopCount = 0;  // reset timer
    pattern = ((unsigned char)(pattern+1))%NUM_STATES;
  }
}

void randomWalk(unsigned char *val, unsigned char maxVal, unsigned char changeAmount, unsigned char directions) {

  unsigned char walk = random(directions);  // direction of random walk
  if (walk == 0) {
    if (*val >= changeAmount) {*val -= changeAmount;}
    else {*val = 0;}
  }
  else if (walk == 1) {
    if (*val <= maxVal - changeAmount) {*val += changeAmount;}
    else {*val = maxVal;}
  }
}

void fade(unsigned char *val, unsigned char fadeTime) {
  if (*val != 0) {
    unsigned char subAmt = *val >> fadeTime;
    if (subAmt < 1) subAmt = 1; *val -= subAmt;}
}

void warmWhiteShimmer(unsigned char dimOnly) {
  const unsigned char maxBrightness = 120;
  const unsigned char changeAmount = 2;
  for (int i = 0; i < LED_COUNT; i += 2) {
    randomWalk(&colors[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 2);
    colors[i].green = colors[i].red*4/5;  // green = 80% of red
    colors[i].blue = colors[i].red >> 3;  // blue = red/8
    if (i + 1 < LED_COUNT) {
      colors[i+1] = (rgb_color){colors[i].red >> 2, colors[i].green >> 2, colors[i].blue >> 2};
    }
  }
}

void randomColorWalk(unsigned char initializeColors, unsigned char dimOnly) {
  const unsigned char maxBrightness = 180;  // cap on LED brightness
  const unsigned char changeAmount = 3;  // size of random walk step
  unsigned char start;
  switch (LED_COUNT % 7) {
    case 0: start = 3; break;
    case 1: start = 0; break;
    case 2: start = 1; break;
    default: start = 2;
  }
  for (int i = start; i < LED_COUNT; i+=7) {
    if (initializeColors == 0) {
      randomWalk(&colors[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 3);
      randomWalk(&colors[i].green, maxBrightness, changeAmount, dimOnly ? 1 : 3);
      randomWalk(&colors[i].blue, maxBrightness, changeAmount, dimOnly ? 1 : 3);
    }
    else if (initializeColors == 1) {
      if (i % 2) {colors[i] = (rgb_color){maxBrightness, 0, 0};}
      else{colors[i] = (rgb_color){0, maxBrightness, 0};}
    }
    else {colors[i] = (rgb_color){random(maxBrightness), random(maxBrightness), random(maxBrightness)};}
    if (i >= 1){colors[i-1] = (rgb_color){colors[i].red >> 2, colors[i].green >> 2, colors[i].blue >> 2};}
    if (i >= 2){colors[i-2] = (rgb_color){colors[i].red >> 3, colors[i].green >> 3, colors[i].blue >> 3};}
    if (i + 1 < LED_COUNT){colors[i+1] = colors[i-1];}
    if (i + 2 < LED_COUNT){colors[i+2] = colors[i-2];}
  }
}

void traditionalColors() {
  const unsigned char initialDarkCycles = 10;
  const unsigned char brighteningCycles = 20;
  if (loopCount < initialDarkCycles) {return;}
  unsigned int extendedLEDCount = (((LED_COUNT-1)/20)+1)*20;
  for (int i = 0; i < extendedLEDCount; i++) {
    unsigned char brightness = (loopCount - initialDarkCycles)%brighteningCycles + 1;
    unsigned char cycle = (loopCount - initialDarkCycles)/brighteningCycles;
    unsigned int idx = (i + cycle)%extendedLEDCount;
    if (idx < LED_COUNT){ if (i % 4 == 0){switch ((i/4)%5) {
      case 0:
        colors[idx].red = 200 * brightness/brighteningCycles;
        colors[idx].green = 10 * brightness/brighteningCycles;
        colors[idx].blue = 10 * brightness/brighteningCycles;
        break;
      case 1:  // green
        colors[idx].red = 10 * brightness/brighteningCycles;
        colors[idx].green = 200 * brightness/brighteningCycles;
        colors[idx].blue = 10 * brightness/brighteningCycles;
        break;
      case 2:  // orange
        colors[idx].red = 200 * brightness/brighteningCycles;
        colors[idx].green = 120 * brightness/brighteningCycles;
        colors[idx].blue = 0 * brightness/brighteningCycles;
        break;
      case 3:  // blue
        colors[idx].red = 10 * brightness/brighteningCycles;
          colors[idx].green = 10 * brightness/brighteningCycles;
          colors[idx].blue = 200 * brightness/brighteningCycles;
          break;
        case 4:  // magenta
          colors[idx].red = 200 * brightness/brighteningCycles;
          colors[idx].green = 64 * brightness/brighteningCycles;
          colors[idx].blue = 145 * brightness/brighteningCycles;
          break;
        }
      }
      else {
        fade(&colors[idx].red, 3);
        fade(&colors[idx].green, 3);
        fade(&colors[idx].blue, 3);
      }
    }
  }
}

void brightTwinkleColorAdjust(unsigned char *color) {
  if (*color == 255) {*color = 254;}
  else if (*color % 2) {*color = *color * 2 + 1;}
  else if (*color > 0) {fade(color, 4);
    if (*color % 2) {(*color)--;}
  }
}


void colorExplosionColorAdjust(unsigned char *color, unsigned char propChance,
 unsigned char *leftColor, unsigned char *rightColor) {
  if (*color == 31 && random(propChance+1) != 0) {
    if (leftColor != 0 && *leftColor == 0) {*leftColor = 1;}
    if (rightColor != 0 && *rightColor == 0){*rightColor = 1;}
  }
  brightTwinkleColorAdjust(color);
}


void colorExplosion(unsigned char noNewBursts)
{
  // adjust the colors of the first LED
  colorExplosionColorAdjust(&colors[0].red, 9, (unsigned char*)0, &colors[1].red);
  colorExplosionColorAdjust(&colors[0].green, 9, (unsigned char*)0, &colors[1].green);
  colorExplosionColorAdjust(&colors[0].blue, 9, (unsigned char*)0, &colors[1].blue);

  for (int i = 1; i < LED_COUNT - 1; i++)
  {
    // adjust the colors of second through second-to-last LEDs
    colorExplosionColorAdjust(&colors[i].red, 9, &colors[i-1].red, &colors[i+1].red);
    colorExplosionColorAdjust(&colors[i].green, 9, &colors[i-1].green, &colors[i+1].green);
    colorExplosionColorAdjust(&colors[i].blue, 9, &colors[i-1].blue, &colors[i+1].blue);
  }

  // adjust the colors of the last LED
  colorExplosionColorAdjust(&colors[LED_COUNT-1].red, 9, &colors[LED_COUNT-2].red, (unsigned char*)0);
  colorExplosionColorAdjust(&colors[LED_COUNT-1].green, 9, &colors[LED_COUNT-2].green, (unsigned char*)0);
  colorExplosionColorAdjust(&colors[LED_COUNT-1].blue, 9, &colors[LED_COUNT-2].blue, (unsigned char*)0);

  if (!noNewBursts)
  {
    // if we are generating new bursts, randomly pick one new LED
    // to light up
    for (int i = 0; i < 1; i++)
    {
      int j = random(LED_COUNT);  // randomly pick an LED

      switch(random(7))  // randomly pick a color
      {
        // 2/7 chance we will spawn a red burst here (if LED has no red component)
        case 0:
        case 1:
          if (colors[j].red == 0)
          {
            colors[j].red = 1;
          }
          break;

        // 2/7 chance we will spawn a green burst here (if LED has no green component)
        case 2:
        case 3:
          if (colors[j].green == 0)
          {
            colors[j].green = 1;
          }
          break;

        // 2/7 chance we will spawn a white burst here (if LED is all off)
        case 4:
        case 5:
          if ((colors[j].red == 0) && (colors[j].green == 0) && (colors[j].blue == 0))
          {
            colors[j] = (rgb_color){1, 1, 1};
          }
          break;

        // 1/7 chance we will spawn a blue burst here (if LED has no blue component)
        case 6:
          if (colors[j].blue == 0)
          {
            colors[j].blue = 1;
          }
          break;

        default:
          break;
      }
    }
  }
}


// ***** PATTERN Gradient *****
// This function creates a scrolling color gradient that smoothly
// transforms from red to white to green back to white back to red.
// This pattern is overlaid with waves of brightness and dimness that
// scroll at twice the speed of the color gradient.
void gradient() {
  unsigned int j = 0;
  while (j < LED_COUNT)
  {
    // transition from red to green over 8 LEDs
    for (int i = 0; i < 8; i++)
    {
      if (j >= LED_COUNT){ break; }
      colors[(loopCount/2 + j + LED_COUNT)%LED_COUNT] = (rgb_color){160 - 20*i, 20*i, (160 - 20*i)*20*i/160};
      j++;
    }
    // transition from green to red over 8 LEDs
    for (int i = 0; i < 8; i++)
    {
      if (j >= LED_COUNT){ break; }
      colors[(loopCount/2 + j + LED_COUNT)%LED_COUNT] = (rgb_color){20*i, 160 - 20*i, (160 - 20*i)*20*i/160};
      j++;
    }
  }
  const unsigned char fullDarkLEDs = 10;  // number of LEDs to leave fully off
  const unsigned char fullBrightLEDs = 5;  // number of LEDs to leave fully bright
  const unsigned char cyclePeriod = 14 + fullDarkLEDs + fullBrightLEDs;
  unsigned int extendedLEDCount = (((LED_COUNT-1)/cyclePeriod)+1)*cyclePeriod;

  j = 0;
  while (j < extendedLEDCount){
    unsigned int idx;
    for (int i = 1; i < 8; i++)
    {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }

      colors[idx].red >>= i;
      colors[idx].green >>= i;
      colors[idx].blue >>= i;
    }
    for (int i = 0; i < fullDarkLEDs; i++) {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }
      colors[idx].red = 0;
      colors[idx].green = 0;
      colors[idx].blue = 0;
    }
    for (int i = 0; i < 7; i++)
    {
      idx = (j + loopCount) % extendedLEDCount;
      if (j++ >= extendedLEDCount){ return; }
      if (idx >= LED_COUNT){ continue; }

      colors[idx].red >>= (7 - i);
      colors[idx].green >>= (7 - i);
      colors[idx].blue >>= (7 - i);
    }
    j += fullBrightLEDs;
  }
}

void brightTwinkle(unsigned char minColor, unsigned char numColors, unsigned char noNewBursts) {
  for (int i = 0; i < LED_COUNT; i++)
  {
    brightTwinkleColorAdjust(&colors[i].red);
    brightTwinkleColorAdjust(&colors[i].green);
    brightTwinkleColorAdjust(&colors[i].blue);
  }

  if (!noNewBursts)
  {
    // if we are generating new twinkles, randomly pick four new LEDs
    // to light up
    for (int i = 0; i < 4; i++)
    {
      int j = random(LED_COUNT);
      if (colors[j].red == 0 && colors[j].green == 0 && colors[j].blue == 0)
      {
        // if the LED we picked is not already lit, pick a random
        // color for it and seed it so that it will start getting
        // brighter in that color
        switch (random(numColors) + minColor)
        {
          case 0:
            colors[j] = (rgb_color){1, 1, 1};  // white
            break;
          case 1:
            colors[j] = (rgb_color){1, 0, 0};  // red
            break;
          case 2:
            colors[j] = (rgb_color){0, 1, 0};  // green
            break;
          case 3:
            colors[j] = (rgb_color){0, 0, 1};  // blue
            break;
          case 4:
            colors[j] = (rgb_color){1, 1, 0};  // yellow
            break;
          case 5:
            colors[j] = (rgb_color){0, 1, 1};  // cyan
            break;
          case 6:
            colors[j] = (rgb_color){1, 0, 1};  // magenta
            break;
          default:
            colors[j] = (rgb_color){1, 1, 1};  // white
        }
      }
    }
  }
}


unsigned char collision() {
  const unsigned char maxBrightness = 180;  // max brightness for the colors
  const unsigned char numCollisions = 5;  // # of collisions before pattern ends
  static unsigned char state = 0;  // pattern state
  static unsigned int count = 0;  // counter used by pattern

  if (loopCount == 0)
  {
    state = 0;
  }

  if (state % 3 == 0)
  {
    // initialization state
    switch (state/3)
    {
      case 0:  // first collision: red streams
        colors[0] = (rgb_color){maxBrightness, 0, 0};
        break;
      case 1:  // second collision: green streams
        colors[0] = (rgb_color){0, maxBrightness, 0};
        break;
      case 2:  // third collision: blue streams
        colors[0] = (rgb_color){0, 0, maxBrightness};
        break;
      case 3:  // fourth collision: warm white streams
        colors[0] = (rgb_color){maxBrightness, maxBrightness*4/5, maxBrightness>>3};
        break;
      default:  // fifth collision and beyond: random-color streams
        colors[0] = (rgb_color){random(maxBrightness), random(maxBrightness), random(maxBrightness)};
    }

    // stream is led by two full-white LEDs
    colors[1] = colors[2] = (rgb_color){255, 255, 255};
    // make other side of the strip a mirror image of this side
    colors[LED_COUNT - 1] = colors[0];
    colors[LED_COUNT - 2] = colors[1];
    colors[LED_COUNT - 3] = colors[2];

    state++;  // advance to next state
    count = 8;  // pick the first value of count that results in a startIdx of 1 (see below)
    return 0;
  }

  if (state % 3 == 1)
  {
    // stream-generation state; streams accelerate towards each other
    unsigned int startIdx = count*(count + 1) >> 6;
    unsigned int stopIdx = startIdx + (count >> 5);
    count++;
    if (startIdx < (LED_COUNT + 1)/2)
    {
      // if streams have not crossed the half-way point, keep them growing
      for (int i = 0; i < startIdx-1; i++)
      {
        // start fading previously generated parts of the stream
        fade(&colors[i].red, 5);
        fade(&colors[i].green, 5);
        fade(&colors[i].blue, 5);
        fade(&colors[LED_COUNT - i - 1].red, 5);
        fade(&colors[LED_COUNT - i - 1].green, 5);
        fade(&colors[LED_COUNT - i - 1].blue, 5);
      }
      for (int i = startIdx; i <= stopIdx; i++)
      {
        // generate new parts of the stream
        if (i >= (LED_COUNT + 1) / 2)
        {
          // anything past the halfway point is white
          colors[i] = (rgb_color){255, 255, 255};
        }
        else
        {
          colors[i] = colors[i-1];
        }
        // make other side of the strip a mirror image of this side
        colors[LED_COUNT - i - 1] = colors[i];
      }
      // stream is led by two full-white LEDs
      colors[stopIdx + 1] = colors[stopIdx + 2] = (rgb_color){255, 255, 255};
      // make other side of the strip a mirror image of this side
      colors[LED_COUNT - stopIdx - 2] = colors[stopIdx + 1];
      colors[LED_COUNT - stopIdx - 3] = colors[stopIdx + 2];
    }
    else
    {
      // streams have crossed the half-way point of the strip;
      // flash the entire strip full-brightness white (ignores maxBrightness limits)
      for (int i = 0; i < LED_COUNT; i++)
      {
        colors[i] = (rgb_color){255, 255, 255};
      }
      state++;  // advance to next state
    }
    return 0;
  }

  if (state % 3 == 2)
  {
    // fade state
    if (colors[0].red == 0 && colors[0].green == 0 && colors[0].blue == 0)
    {
      // if first LED is fully off, advance to next state
      state++;

      // after numCollisions collisions, this pattern is done
      return state >= 3*numCollisions;
    }

    // fade the LEDs at different rates based on the state
    for (int i = 0; i < LED_COUNT; i++)
    {
      switch (state/3)
      {
        case 0:  // fade through green
          fade(&colors[i].red, 3);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 2);
          break;
        case 1:  // fade through red
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 3);
          fade(&colors[i].blue, 2);
          break;
        case 2:  // fade through yellow
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 3);
          break;
        case 3:  // fade through blue
          fade(&colors[i].red, 3);
          fade(&colors[i].green, 2);
          fade(&colors[i].blue, 4);
          break;
        default:  // stay white through entire fade
          fade(&colors[i].red, 4);
          fade(&colors[i].green, 4);
          fade(&colors[i].blue, 4);
      }
    }
  }
  return 0;
}
