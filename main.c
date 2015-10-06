#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define led_pin    13

typedef unsigned long int microseconds;
typedef unsigned int pinstate;

microseconds off_by(microseconds, microseconds, *boolean);
void pdelay(microseconds);
void wait_for_press();
void set_led(boolean);

microseconds off_by(microseconds ideal, microseconds observed, *boolean is_late) {
  if (observed > ideal) {
    *is_late = true;
    return observed - ideal;
  } else {
    *is_late = false;
    return ideal - observed;
  }
}

/* A more precise delay than Arduino's own with the penalty of quicker overflow
 * (70 minutes. */
void pdelay(microseconds us) {
  delay((unsigned long int) us / 1000);
  delayMicroseconds((unsigned int) us % 1000);
}

/* Attach a 10K resistor from ground to a pushbutton and pin 2. Connect the
 * other side of the button to 5V. pinMode(button_pin, INPUT); */
void wait_for_press() {

  // read the pushbutton input pin
  pinstate button_state = digitalRead(button_pin);

  // you've got to release the button every time
  while (button_state != LOW) {
    delay(5);
    button_state = digitalRead(button_pin);
  }

  // wait until the button is pressed
  while (button_state != HIGH) {
    delay(5);
    button_state = digitalRead(button_pin);
  }
}

/* Should be built into your board, remember to pinMode(led_pin, OUTPUT). */
void set_led(boolean state) { digitalWrite(led_pin, state ? HIGH : LOW); }

void setup() {

  // seed the random generator
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // initialize the button pin as a input
  pinMode(button_pin, INPUT);

  // initialize the LED as an output and turn it off
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
}

void loop() {

  // Far more straightforward on the Arduino than in standard C!
  int bpm = random(min_tempo, max_tempo + 1);
  
  // then convert to microseconds/beat
  microseconds uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  // 0:   the one beat before the first user-beat
  // 1-8: the user's keypress times
  microseconds timed[9];

  // prepare other variables
  microseconds ideal, error;
  boolean is_late;

  Serial.println("I'll show you a tempo, then match it!");
  Serial.println("I'll give you eight beats, then tap out the next eight on the button.");
  Serial.println("Press the button to begin.");
  set_led(true);
  wait_for_press();

  Serial.print("Tempo is ");
  Serial.println(bpm);
  delay(1000);

  do {
    Serial.println("Here we go!");
    set_led(false);
    pdelay(uspb);

    // count off eight beats
    for (int i = 1; i <= 6; i++) {
      Serial.println(i);
      set_led(i % 2);
      pdelay(uspb);
    }

    Serial.println("7 ready");
    set_led(true);
    pdelay(uspb);

    // record the beat before the user's first
    Serial.println("8 go");
    set_led(false);
    timed[0] = micros();

    // the user taps enter eight times
    // we measure the clock time of each
    for (int i = 1; i <= 8; i++) {
      wait_for_press();
      timed[i] = micros();

      Serial.println(i);
      set_led(i % 2);
    }
    pdelay(uspb);

    Serial.println("Okay!");
    set_led(true);
    pdelay(4 * uspb);

    // Take the beat before the user started tapping. Project forward to see at
    // what time a perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 8);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    error = off_by(ideal, timed[8], &is_late);

    Serial.print(error / 1e6);
    Serial.print("s ");
    Serial.println(is_late ? "late" : "early");
    set_led(false);
    pdelay(4 * uspb);

    // I'd like to divide this by tpb to give a better statistic#TODO

    Serial.println("One");
    set_led(true);
    pdelay(2 * uspb);

    Serial.println("more");
    set_led(false);
    pdelay(2 * uspb);

    Serial.println("time?");
    set_led(true);
    pdelay(3 * uspb);

  } while (1);
}
