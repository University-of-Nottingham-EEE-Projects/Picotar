//*************************************************************************//
//*  University of Nottingham                                             *//
//*  Department of Electrical and Electronic Engineering                  *//
//*  UoN Offer Holder Day Practical                                       *//
//*                                                                       *//
//*  'Picotar' - Raspberry Pi Pico W Controlled 'Electric' Guitar         *//
//*  Samples, Metronome & Audio FX Example                                *//
//*                                                                       *//
//*  Created By: Nathaniel (Nat) Dacombe                                  *//
//*************************************************************************//
//* this example allows the 'Picotar' to have three distinct functions    *//
//* 1. play a chosen sample, with the audio FX being to vary the speed    *//
//* 2. act as a metronome, with the audio FX being to set the BPM         *//
//* 3. add multiple tones and echoes using the 'slider' and audio FX dial *// 
//*************************************************************************//

// the example codes and all other files relating to the 'Picotar' project can be accessed using the QR code on your PCB or from: https://github.com/University-of-Nottingham-EEE-Projects/Picotar
// feel free to modify the examples and create your own versions, or take the Raspberry Pi Pico W and use it in an entirely different project - we would love to see what you end up creating!; share these with us by emailing nathaniel.dacombe@nottingham.ac.uk
// feel free to also contact by emailing nathaniel.dacombe@nottingham.ac.uk with any queries, questions or feedback

// programming the Raspberry Pi Pico W with the Arduino IDE...
// if you haven't programmed the Raspberry Pi Pico on the current PC or laptop that you are running the Arduino IDE on, chances are that you will need to follow the below steps - if it has, ignore the steps below, if so, read on
// 1. inside the Arduino IDE, click 'File' (top left corner) --> 'Preferences'
// 2. enter the following URL into the 'Additional Boards Manager URLs' field: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json - then click 'OK'
// 3. click 'Tools' --> 'Board' --> 'Boards Manager...' and search 'pico'
// 4. install the Raspberry Pi Pico/RP2040 boards, once completed if you go into 'Tools' --> 'Board' there should be a selection of Raspberry Pi Pico boards (use the 'Raspberry Pi Pico W' board for this project)

// now for some notes about the code...
// the 'Picotar' project utilises the Mozzi audio library by Tim Barrass
// full details, including tutorials, examples and forums are available on the Mozzi GitHub page here: https://sensorium.github.io/Mozzi/
// the variable naming conventions using 'k' stand for 'control' and 'a' stand for 'audio'
// comments are added throughout the code for your understanding to explain key processes or functions - if anything is unclear, feel free to ask, or search for the functions online

// finally, enjoy and rock on!

// include the necessary Mozzi libraries
#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_rand.h> // for the rand() 'random' function

// include the libraries needed for the 'Sample' and 'Metronome' audio FX
#include <Sample.h>
#include <SampleHuffman.h>

// include the library needed for the 'Metronome' audio FX
#include <Metronome.h>

// include the libraries needed for the 'Echo' audio FX
#include <RollingAverage.h>
#include <ControlDelay.h>

// include tables of waveform types
#include <tables/sin2048_int8.h>
#include <tables/waveshape2_softerclip_int8.h>

// include libraries of samples needed for the 'Sample' and 'Metronome' audio FX
#include <samples/thumbpiano_huffman/thumbpiano0.h>
#include <samples/bamboo/bamboo_00_2048_int8.h> 
#include <samples/bamboo/bamboo_01_2048_int8.h>
#include <samples/bamboo/bamboo_02_2048_int8.h>
#include <samples/bamboo/bamboo_03_2048_int8.h> 
#include <samples/bamboo/bamboo_04_2048_int8.h> 
#include <samples/bamboo/bamboo_05_2048_int8.h> 
#include <samples/bamboo/bamboo_06_2048_int8.h> 
#include <samples/bamboo/bamboo_07_2048_int8.h>
#include <samples/bamboo/bamboo_08_2048_int8.h>
#include <samples/bamboo/bamboo_09_2048_int8.h> 
#include <samples/bamboo/bamboo_10_2048_int8.h>

// some constant definitions
#define CONTROL_RATE 64

// pin definitions for the potentiometers, buttons and LEDs
#define PITCH_PIN 28
#define VOLUME_PIN 27
#define AUDIOFX_PIN 26

#define NOTE1_PIN 17
#define NOTE2_PIN 16
#define NOTE3_PIN 4

#define LED1 21
#define LED2 20
#define LED3 19

// some general variables
int audioFXFlag = -1; // -1 (no output), 0 (sample), 1 (metronome), 2 (echo) 

// variables for the analogue inputs
byte volume;
unsigned int volumeTemp;
unsigned int audioFXVal;

// 'Sample' Audio FX Variables
// -----------------------------------------------------------------------
// for scheduling samples to play
EventDelay kTriggerDelay;
EventDelay kTriggerSlowDelay;

const byte NUM_PLAYERS = 3; // 3 samples used
const byte NUM_TABLES = 11;

byte ms_per_note = 111; // subject to CONTROL_RATE

// gain for each sample player
byte gain[NUM_PLAYERS];

// stores the samples
Sample <BAMBOO_00_2048_NUM_CELLS, AUDIO_RATE> aSample[NUM_PLAYERS] ={
  Sample <BAMBOO_00_2048_NUM_CELLS, AUDIO_RATE> (BAMBOO_00_2048_DATA),
  Sample <BAMBOO_01_2048_NUM_CELLS, AUDIO_RATE> (BAMBOO_01_2048_DATA),
  Sample <BAMBOO_02_2048_NUM_CELLS, AUDIO_RATE> (BAMBOO_02_2048_DATA),
};

// table of samples which are randomised
const int8_t * tables[NUM_TABLES] ={
  BAMBOO_00_2048_DATA,
  BAMBOO_01_2048_DATA,
  BAMBOO_02_2048_DATA,
  BAMBOO_03_2048_DATA,
  BAMBOO_04_2048_DATA,
  BAMBOO_05_2048_DATA,
  BAMBOO_06_2048_DATA,
  BAMBOO_07_2048_DATA,
  BAMBOO_08_2048_DATA,
  BAMBOO_09_2048_DATA,
  BAMBOO_10_2048_DATA
};

// 'Metronome' Audio FX Variables
// -----------------------------------------------------------------------
int bpm; // beats per minute

SampleHuffman thumb0(THUMB0_SOUNDDATA,THUMB0_HUFFMAN,THUMB0_SOUNDDATA_BITS);
Metronome kMetro(800); // ensure there is enough delay so samples do not overlap

// 'Echo' Audio FX Variables
// -----------------------------------------------------------------------
int averaged;
unsigned int echo_cells_1 = 32;
unsigned int echo_cells_2 = 60;
unsigned int echo_cells_3 = 127;

ControlDelay <128, int> kDelay; // 2 second delay

// oscil used to compare the bumpy input to the averaged control input
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin0(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);

RollingAverage <int, 32> kAverage; // how_many_to_average has to be a power of 2


// Setup Function - only runs once
// -----------------------------------------------------------------------
void setup() {
  kDelay.set(echo_cells_1);
  kTriggerDelay.set(ms_per_note); // countdown in milliseconds (ms), within the resolution of CONTROL_RATE
  kTriggerSlowDelay.set(ms_per_note*6); // resolution-dependent inaccuracy leads to polyrhythm
  
  setupFastAnalogRead();

  // set the LED pins as outputs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  startMozzi();
}


// Randomise Function - used by the 'Sample' Audio FX Effect
// -----------------------------------------------------------------------
byte randomGain() {
  return rand((byte)80, (byte)255); // pick a random value between 80 and 255
}


// Update Control Function - where controlling inputs are read & values updated
// -----------------------------------------------------------------------
void updateControl() {
  // read the button values
  unsigned char note1Val = digitalRead(NOTE1_PIN);
  unsigned char note2Val = digitalRead(NOTE2_PIN);
  unsigned char note3Val = digitalRead(NOTE3_PIN);

  // depending which button is pressed, depends which audio FX is applied
  if (note1Val == 1){
    audioFXFlag = 0;
  }

  else if (note2Val == 1){
    audioFXFlag = 1;
  }

  else if (note3Val == 1){
    audioFXFlag = 2;
  }

  else {
    // audioFXFlag remains as -1;
  }

  // read the potentiometer values
  volumeTemp = analogRead(VOLUME_PIN);  // range of 0 to 1023
  volume = map(volumeTemp, 0, 1023, 0, 255);  // new range of 0 to 255
  audioFXVal = analogRead(AUDIOFX_PIN); // range of 0 to 1023

  // 'Sample' Audio FX
  if (audioFXFlag == 0) {
    analogWrite(LED1, volume);  // LED brightness varies with volume
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    
    static byte player = 0;
    audioFXVal = map(audioFXVal, 0, 1023, 8192, 128); // new range of 8192 to 128 - change the last two values if you wish to vary the max and min sample speeds

    if (kTriggerDelay.ready()) {
      gain[player] = randomGain();
      (aSample[player]).setTable(tables[rand(NUM_TABLES)]);
      (aSample[player]).start();
      player++;

      if (player == NUM_PLAYERS) {
        player = 0;
      }

      kTriggerDelay.start();
    }

    if (kTriggerSlowDelay.ready()) {
      gain[player] = randomGain();
      (aSample[player]).setTable(tables[rand(NUM_TABLES)]);
      (aSample[player]).start();
      player++;
      if (player == NUM_PLAYERS) {
        player = 0;
      }
      kTriggerSlowDelay.start();
    }
  }

  // 'Metronome' Audio FX
  else if (audioFXFlag == 1) {
    digitalWrite(LED1, LOW);
    analogWrite(LED2, volume);  // LED brightness varies with volume
    digitalWrite(LED3, LOW);

    bpm = map(audioFXVal, 0, 1023, 10, 200);  // range of 10 to 200 - change the last two values if you wish to vary the min and max metronome speeds (beats per minute)
    kMetro.setBPM(bpm);

    static unsigned int counter = 0;
    counter++;

    if (counter==0) {
      kMetro.start();
      counter = 0;
    }

    if (kMetro.ready()) {
        thumb0.start();
    }
  }

  // 'Echo' Audio FX
  else if (audioFXFlag == 2){
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    analogWrite(LED3, volume);  // LED brightness varies with volume

    int bumpy_input = analogRead(PITCH_PIN) + audioFXVal; // sum the 'slider' and 'Audio FX Level' dial values
    averaged = kAverage.next(bumpy_input); // calculate the current rolling average

    // set the sine wave frequencies
    aSin0.setFreq(averaged);
    aSin1.setFreq(kDelay.next(averaged));
    aSin2.setFreq(kDelay.read(echo_cells_2));
    aSin3.setFreq(kDelay.read(echo_cells_3));
  }

  else {
    // no output and no audio FX
  }
}


// Update Audio Function - where the audio signal is updated with the control values
// -----------------------------------------------------------------------
AudioOutput_t updateAudio() {
  // 'Sample' Audio FX
  if (audioFXFlag == 0){
    for (int i = 0; i < NUM_PLAYERS; i++) {  // for each sample
      (aSample[i]).setFreq(((float) BAMBOO_00_2048_SAMPLERATE) / audioFXVal); // set the playback speed based on the 'Audio FX Level' dial
    }

    long aSignal = 0;

    // sum all of the sample signals together into one waveform
    for (byte i = 0; i < NUM_PLAYERS; i++) {
      aSignal += (int)(aSample[i]).next() * volume;
    }
    
    return MonoOutput::fromAlmostNBit(15, aSignal).clip(); // clip any stray peaks to max output range
  }
  
  // 'Metronome' Audio FX
  else if (audioFXFlag == 1) {
    int aSignal = (int)thumb0.next();
    float volumeScale = map(volume, 0, 255, 0, 20); // new range of 0 to 20 - change the last two values if you wish to vary the min and max volume for the metronome only (be warned, if the max is too high, the sound is distorted)
    
    return MonoOutput::from8Bit(aSignal * (volumeScale / 10)); // note: the samples do not overlap here, therefore this  sum is still only 8 bits 
  }
  
  // 'Echo' Audio FX
  else if (audioFXFlag == 2) {
    float volumeScale = map(volume, 0, 255, 0, 20); // new range of 0 to 20 - change the last two values if you wish to vary the min and max volume for the metronome only (be warned, if the max is too high, the sound is distorted)
    long aSignalTemp = 3 * (int)aSin0.next() + aSin1.next() + (aSin2.next() >> 1) + (aSin3.next() >> 2);  // sum the sine waves together into one waveform
    int aSignal = aSignalTemp * (volumeScale / 10); // apply the volume scale

    return MonoOutput::fromAlmostNBit(12, aSignal).clip();
  }

  else {
    // no output and no audio FX
  }
  return 0;
}


// Loop Function - continually runs and synthesises audio
// -----------------------------------------------------------------------
void loop() {
  audioHook();
}
