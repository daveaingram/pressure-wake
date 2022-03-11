//
// Configuration variables
//
int on_thresh = 50; // above this percentage is "on"
int off_thresh = 40; // below this percentage is "off"
int min_trigger_time = 200; // must hold state for at least this many milliseconds to trigger
int max_trigger_time = 3000; // this many milliseconds to switch to the next state before resetting

int play_folder = 1;
int play_song = 3;

//
// Script setup
//
#include <MD_YX5300.h>
#include <HardwareSerial.h>

HardwareSerial MP3Serial(1);
MD_YX5300 mp3(MP3Serial);

unsigned long current_time;
unsigned long last_on_trigger_time;
unsigned long last_off_trigger_time;

int trigger_counter = 0;

int fsr_reading; // current reading
int last_fsr_reading; // previous reading
bool blinking_red = false; // whether the LED should blink or not

// Set up pins
const int fsr_pin = 34;
const int yellow_pin = 32;
const int red_pin = 33;
const int mp3_tx = 16;
const int mp3_rx = 17;

void setup() {
	Serial.begin(115200);
  Serial.println("Waking up");

  // set up LEDs
  pinMode(yellow_pin, OUTPUT);
  pinMode(red_pin, OUTPUT);

  // set up MP3 player
  MP3Serial.begin(9600, SERIAL_8N1, mp3_tx, mp3_rx);
  mp3.begin();
  // mp3.setSynchronous(true);
  // mp3.playFolderRepeat(1);
}

void loop() {
  current_time = millis();

  // read in the FSR and scale values between 0-100
  fsr_reading = map(analogRead(fsr_pin), 0, 4095, 0, 100);
  // Serial.println(fsr_reading, DEC);

  // check for thresholds crossing and act accordingly
  if(fsr_reading > on_thresh && last_fsr_reading <= on_thresh) {
    triggered_on(current_time);
  } else if (fsr_reading < off_thresh && last_fsr_reading >= off_thresh) {
    triggered_off(current_time);
  }
	
  // clean up loop for next time
  blink_red();
  reset_counter();
  
  last_fsr_reading = fsr_reading;
  delay(100);
}

void triggered_on(unsigned long trigger_time) { 
  // show visual cue that we're on
  digitalWrite(yellow_pin, HIGH);
  Serial.println("triggered on");

  // if this is the first trigger, always count up
  if(trigger_counter == 0) {
    set_trigger_counter(trigger_counter + 1);
  } else if (trigger_time < last_on_trigger_time + max_trigger_time) {
    // if triggering happened within the allotted time, increment the counter and take action
    set_trigger_counter(trigger_counter + 1);
  }

  // set the time that triggering happened
  last_on_trigger_time = trigger_time;
}

void triggered_off(unsigned long trigger_time) {
  // show visual cue that we're off
  digitalWrite(yellow_pin, LOW);
  Serial.println("triggered off");

  // set the time that triggering happened
  last_off_trigger_time = trigger_time;
}

void set_trigger_counter(int count) {
  trigger_counter = count;
  Serial.println(count, DEC);

  // default everything to off for all cases including 0 (start)
  blinking_red = false;
  digitalWrite(red_pin, LOW);

  switch (count) {
    case 1:
      digitalWrite(red_pin, HIGH);
      break;
    case 2:
      blinking_red = true;
      break;
    case 3:
      triggered();
      break;
  }
}

void reset_counter() {
  static int last_counter;

  // set the counter to 0 only if it's not already there and if the max_trigger_time has passed
  if(last_counter != 0 && current_time > last_on_trigger_time + max_trigger_time) {
    set_trigger_counter(0);
  }

  last_counter = trigger_counter;
}

void blink_red() {
  const int blink_speed = 300;
  static unsigned long last_change;
  static int current_state = LOW;

  if(blinking_red) {
    if (current_time > last_change + blink_speed) {
      if(current_state == LOW) {
        digitalWrite(red_pin, HIGH);
        current_state = HIGH;
      } else {
        digitalWrite(red_pin, LOW);
        current_state = LOW;
      }
    }
  }
}

// do the things we came here to do
void triggered() {
  Serial.println("Triggered");
  mp3.playSpecific(play_folder, play_song);

  // go ahead and reset
  set_trigger_counter(0);
}
