//***********************************************************************//
//*  University of Nottingham                                           *//
//*  Department of Electrical and Electronic Engineering                *//
//*  UoN Offer Holder Day Practical                                     *//
//*                                                                     *//
//*  'Picotar' - Raspberry Pi Pico W Controlled 'Electric' Guitar       *//
//*  Three Chord Guitar Example                                         *//
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
#include <Oscil.h>
#include <WaveShaper.h>
#include <Ead.h>  // exponential attack decay (ead)

// include tables of waveform types
#include <tables/sin2048_int8.h>
#include <tables/waveshape_compress_512_to_488_int16.h>
#include <tables/waveshape2_softerclip_int8.h>

// some constant definitions
#define CONTROL_RATE 64
#define VOLUME_MAX 100

// pin definitions for the potentiometers, buttons and LEDs
#define PITCH_PIN 28
#define VOLUME_PIN 27
#define DECAY_PIN 26

#define GUITAR1_PIN 17
#define GUITAR2_PIN 16
#define GUITAR3_PIN 4

// definitions of the min frequency of Octave 4 and the max frequency of Octave 6 for chords A to G
#define CHORD_A_MIN 440
#define CHORD_A_MAX 1760

#define CHORD_B_MIN 493.88
#define CHORD_B_MAX 1975.533

#define CHORD_C_MIN 261.626
#define CHORD_C_MAX 1046.502

#define CHORD_D_MIN 293.665
#define CHORD_D_MAX 1174.659

#define CHORD_E_MIN 329.628
#define CHORD_E_MAX 1318.51

#define CHORD_F_MIN 349.228
#define CHORD_F_MAX 1396.913

#define CHORD_G_MIN 391.995
#define CHORD_G_MAX 1567.982

// allocate a chord to each note button - feel free to change the allocated chords using the definition names above
int CHORD_1_MIN = CHORD_G_MIN;
int CHORD_1_MAX = CHORD_G_MAX;

int CHORD_2_MIN = CHORD_D_MIN;
int CHORD_2_MAX = CHORD_D_MAX;

int CHORD_3_MIN = CHORD_C_MIN;
int CHORD_3_MAX = CHORD_C_MAX;

// oscil used to compare the bumpy input to the averaged control input
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSinNote1(SIN2048_DATA); // sine wave sound source
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSinNote2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSinNote3(SIN2048_DATA);

// defining variables to store various stages of audio waves
WaveShaper <int> aCompress(WAVESHAPE_COMPRESS_512_TO_488_DATA); // to compress instead of dividing by 2 after adding signals

WaveShaper <char> aNote1Shaper(WAVESHAPE2_SOFTERCLIP_DATA);
WaveShaper <char> aNote2Shaper(WAVESHAPE2_SOFTERCLIP_DATA);
WaveShaper <char> aNote3Shaper(WAVESHAPE2_SOFTERCLIP_DATA);

Ead kEnvelopeNote1(CONTROL_RATE);
Ead kEnvelopeNote2(CONTROL_RATE);
Ead kEnvelopeNote3(CONTROL_RATE);

// variables for the analogue inputs
int pitch;
int volumeTemp;
int volume;
int decay;

// variables used for adding 'decay' to the note
unsigned char lastNote1Val = 0, lastNote2Val = 0, lastNote3Val = 0;
unsigned int attackMilliseconds = 20, decayMilliseconds = 3000;
unsigned int lastDecayValue = 0;
int gainNote1, gainNote2, gainNote3;


// Update Decay Values Function - used by the 'Audio FX Level' dial to vary the decay
// -----------------------------------------------------------------------
void updateAttackDecayValues() {
  kEnvelopeNote1.set(attackMilliseconds, decayMilliseconds);
  kEnvelopeNote2.set(attackMilliseconds, decayMilliseconds);
  kEnvelopeNote3.set(attackMilliseconds, decayMilliseconds); 
}


// Setup Function - only runs once
// -----------------------------------------------------------------------
void setup() {
  startMozzi();
  
  // set the initial frequency of each sine wave
  aSinNote1.setFreq(CHORD_1_MIN);
  aSinNote2.setFreq(CHORD_2_MIN);
  aSinNote3.setFreq(CHORD_3_MIN);
  
  updateAttackDecayValues();
  setupFastAnalogRead();

  // set the LED pins as outputs
  pinMode(21, OUTPUT);
  pinMode(20, OUTPUT);
  pinMode(19, OUTPUT);
}


// Update Control Function - where controlling inputs are read & values updated
// -----------------------------------------------------------------------
void updateControl() {
  // read the potentiometer values
  pitch = analogRead(PITCH_PIN);  // Range: 0 to 1023
  volumeTemp = analogRead(VOLUME_PIN);  // Range: 0 to 1023
  volume = map(volumeTemp, 0, 1023, 0, 255);
  decay = analogRead(DECAY_PIN);  // Range: 0 to 1023
  
  // read the button values
  unsigned char note1Val = digitalRead(GUITAR1_PIN);
  unsigned char note2Val = digitalRead(GUITAR2_PIN);
  unsigned char note3Val = digitalRead(GUITAR3_PIN);

  // depending which button is pressed, depends which LED illuminates
  if (note1Val == 1) {
    digitalWrite(21, HIGH);
  }
  else {
    digitalWrite(21, LOW);
  }
  if (note2Val == 1) {
    digitalWrite(20, HIGH);
  }
  else {
    digitalWrite(20, LOW);
  }
  if (note3Val == 1) {
    digitalWrite(19, HIGH);
  }
  else {
    digitalWrite(19, LOW);
  }
  
  // only if a certain amount of time has lapsed then update the decay value
  if (((decay - lastDecayValue) > 4) || ((decay - lastDecayValue) < -4)) { // on Arduino boards you could use abs() to get the magnitude of a value
    decayMilliseconds = decay << 2; 
    lastDecayValue = decay; 
    updateAttackDecayValues();
  }
  
  // if a change in state has occurred, then a button has been pressed
  if (note1Val == HIGH && lastNote1Val == LOW) {
    kEnvelopeNote1.start();
  }
  if (note2Val == HIGH && lastNote2Val == LOW) {
    kEnvelopeNote2.start();
  }
  if (note3Val == HIGH && lastNote3Val == LOW) {
    kEnvelopeNote3.start();
  }
  
  // current button values are now stored as previous button values
  lastNote1Val = note1Val;
  lastNote2Val = note2Val;
  lastNote3Val = note3Val;
  
  // map the potentiometer value to the range of the relevant chosen chords
  float note1Pitch = map(pitch, 0, 1023, CHORD_1_MIN, CHORD_1_MAX);   
  float note2Pitch = map(pitch, 0, 1023, CHORD_2_MIN, CHORD_2_MAX);  
  float note3Pitch = map(pitch, 0, 1023, CHORD_3_MIN, CHORD_3_MAX);

  // set the sine waves to the same frequency as the chosen chords
  aSinNote1.setFreq(note1Pitch);
  aSinNote2.setFreq(note2Pitch);
  aSinNote3.setFreq(note3Pitch);
  
  // return the next value from the envelope
  gainNote1 = kEnvelopeNote1.next();
  gainNote2 = kEnvelopeNote2.next();
  gainNote3 = kEnvelopeNote3.next();
  
  // limit the min and max values of the gain - this restricts the max volume
  gainNote1 = constrain(gainNote1, 0, VOLUME_MAX);
  gainNote2 = constrain(gainNote2, 0, VOLUME_MAX);
  gainNote3 = constrain(gainNote3, 0, VOLUME_MAX);
}


// Update Audio Function - where the audio signal is updated with the control values
// -----------------------------------------------------------------------
int updateAudio() {
  // sine wave sources - returns the next value in the ADSR (attach, decay, sustain, release)
  char aNote1 = aSinNote1.next(); 
  char aNote2 = aSinNote2.next();
  char aNote3 = aSinNote3.next();
  
  int aNote1Shaped = aNote1Shaper.next(aNote1);
  int aNote2Shaped = aNote2Shaper.next(aNote2);
  int aNote3Shaped = aNote3Shaper.next(aNote3);

  // combine the waveforms and adjust the volume - then shift by 8 bits
  long waveSum = (gainNote1 * aNote1Shaped + gainNote2 * aNote2Shaped + gainNote3 * aNote3Shaped) >> 8;
  waveSum = (waveSum*volume) >> 8;

  // use a waveshaping table to squeeze 2 summed 8 bit signals into the range -244 to 243
  int waveshapedOut = aCompress.next(256u + waveSum);
  
  return waveshapedOut;
}


// Loop Function - continually runs and synthesises audio
// -----------------------------------------------------------------------
void loop(){
  audioHook();
}
