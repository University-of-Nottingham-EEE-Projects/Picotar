//***********************************************************************//
//*  University of Nottingham                                           *//
//*  Department of Electrical and Electronic Engineering                *//
//*  UoN Offer Holder Day Practical                                     *//
//*                                                                     *//
//*  'Picotar' - Raspberry Pi Pico W Controlled 'Electric' Guitar       *//
//*  Three Chord String Guitar Example                                  *//
//*                                                                     *//
//*  Created By: Nathaniel (Nat) Dacombe                                *//
//***********************************************************************//
//* this example allows the 'Picotar' to be used in a similar way to a  *//
//* standard electric guitar, with the buttons selecting the notes, the *//
//* 'slider' determining the pitch and the dials determining the volume *//
//* and audio FX level - in this case, the amount of decay on the note  *//
//***********************************************************************//

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

// pin definitions for the potentiometers, buttons and LEDs
#define PITCH_PIN 28 
#define VOLUME_PIN 27
#define DECAY_PIN 26 

#define GUITAR1_PIN 17
#define GUITAR2_PIN 16
#define GUITAR3_PIN 4

const int buttonPins[] = {GUITAR1_PIN, GUITAR2_PIN, GUITAR3_PIN}; // store the button pins in an array
const int ledPins[] = {21, 20, 19};

// feel free to adjust the values for each of the three notes below - the lower the value the higher the chord/note
#define highNote 200
#define midNote 250
#define lowNote 300

// audio related variables
const int baseBufferSize = lowNote; // ensures that base buffer size is always the same as the largest chord base 
short delayLine[baseBufferSize];  // stores the sound data

unsigned int readIndex = 0, writeIndex = 1; // read and write variables to store the positions to read/write audio to/from
bool lastButtonState[] = {LOW, LOW, LOW}; // the default button state is LOW (when not pressed)

// base buffer sizes for different chords
const int chordBaseSizes[] = {highNote, midNote, lowNote};
int currentBufferSize = baseBufferSize;
float volume = 1.0; // initial volume


// Setup Function - only runs once
// -----------------------------------------------------------------------
void setup() {
  startMozzi();

  // for each of the three variables in an array...
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT); // set the button pins as inputs
    pinMode(ledPins[i], OUTPUT); // set the LED pins as outputs
    digitalWrite(ledPins[i], LOW); // initialize the LEDs to off in the first instance
  }
}


// Update Control Function - where controlling inputs are read & values updated
// -----------------------------------------------------------------------
void updateControl() {
  // read the pitch potentiometer
  int pitchVal = analogRead(PITCH_PIN); // range 0 to 1023
  float pitchAdjustment = map(pitchVal, 0, 1023, 70, 130) / 100.0;  // new range of 0.7 to 1.3

  // read the volume potentiometer and adjust the output volume
  int volumeVal = analogRead(VOLUME_PIN); // range 0 to 1023
  volume = map(volumeVal, 0, 1023, 0, 1023) / 1023.0; // scale the volume between 0 (mute) and 1 (max)

  // for each of the three buttons...
  for (int i = 0; i < 3; i++) {
    bool currentButtonState = digitalRead(buttonPins[i]) == HIGH; // checks if the button has been pressed (so is now HIGH)

    if (currentButtonState && !lastButtonState[i]) {  // checks that a change in button state has occured
      currentBufferSize = chordBaseSizes[i] * pitchAdjustment;  // sets the size of the buffer based on the selected pitch and selected chord
      currentBufferSize = min(currentBufferSize, (int)sizeof(delayLine)/sizeof(delayLine[0]));  // ensures that the buffer size doesn't exceed the delayLine size
      readIndex = 0;  // resets the read index
      writeIndex = 1; // resets the write index

      // fill the buffer with random noise data i.e. the excitation of a string when a guitar string is plucked
      for (int j = 0; j < currentBufferSize; j++) {
        delayLine[j] = random(-2048, 2048); // found from trial and error that 2^11 generate the most 'string-like' sound
      }

      digitalWrite(ledPins[i], !digitalRead(ledPins[i])); // toggle the LED state
    }

    lastButtonState[i] = currentButtonState;  // update the last known button state
  }
}


// Update Audio Function - where the audio signal is updated with the control values
// -----------------------------------------------------------------------
int updateAudio() {
  // dynamically adjust the decay/damping based on the potentiometer value
  int decayVal = analogRead(DECAY_PIN); // range of 0 to 1023
  float decay = map(decayVal, 0, 1023, 800, 999) / 1000.0;  // new range of 0.8 to 0.999

  if ((readIndex < currentBufferSize) && (writeIndex < currentBufferSize)) {  // checks that the index values are within the buffer size
    short nextSample = (delayLine[readIndex] + delayLine[writeIndex]) * decay * 0.5;  // calculates the next audio sample to be played
    delayLine[writeIndex] = nextSample; // updates the position with the new sample to create the vibrating string sound

    // increment the index values, using the modulus sign to ensure the index values wrap round when they reach the end of the buffer i.e. a circular buffer
    readIndex = (readIndex + 1) % currentBufferSize;
    writeIndex = (writeIndex + 1) % currentBufferSize;
    
    // apply the volume control at the final stage
    return nextSample * volume;
  } 
  
  else {
    return 0; // Return silence if outside buffer size
  }
}


// Loop Function - continually runs and synthesises audio
// -----------------------------------------------------------------------
void loop() {
  audioHook();
}