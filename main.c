#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define led_pin    13

int  random_tempo();
void wait_for_press();
void toggle_led(int *);

/* Attach a 10K resistor from ground to a pushbutton and pin 2. Connect the
 * other side of the button to 5V. pinMode(button_pin, INPUT); */
void wait_for_press() {

  // read the pushbutton input pin:
  unsigned int button_state = digitalRead(button_pin);

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

// Far more straightforward on the Arduino than in standard C!
int random_tempo() { return random(min_tempo, max_tempo + 1); }

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

  // make a tempo in beats/minute
  // then convert to nanoseconds/beat
  // and microseconds/beat
  int bpm = random_tempo();
  unsigned long int uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  // 0:   the one beat before the first user-beat
  // 1-8: the user's keypress times
  unsigned long int timed[9];

  // prepare other variables
  unsigned long int ideal;
  double difference;
  int is_late;

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
    delayMicroseconds(uspb);

    // count off eight beats
    for (int i = 1; i <= 6; i++) {
      Serial.println(i);
      set_led(i % 2);
      delayMicroseconds(uspb);
    }

    Serial.println("7 ready");
    set_led(true);
    delayMicroseconds(uspb);

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
    delayMicroseconds(uspb);

    Serial.println("Okay!");
    set_led(true);
    delayMicroseconds(4 * uspb);

    // Take the beat before the user started tapping. Project forward to see at
    // what time a perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 8);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    difference = (timed[8] - ideal) / 1e6;
    is_late = (difference > 0);

    Serial.print(difference);
    Serial.print("s ");
    Serial.println(is_late ? "late" : "early");
    set_led(false);
    delayMicroseconds(4 * uspb);

    // I'd like to divide this by tpb to give a better statistic#TODO

    Serial.println("One");
    set_led(true);
    delayMicroseconds(2 * uspb);

    Serial.println("more");
    set_led(false);
    delayMicroseconds(2 * uspb);

    Serial.println("time?");
    set_led(true);
    delayMicroseconds(3 * uspb);

  } while (1);
}
