#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define led_pin    13
#define to_s(X) String(X).c_str()

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);
// Display: http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/
// OLED pins: SDA -> A4, SCL -> A5

unsigned long int off_by(unsigned long int, unsigned long int, boolean *);
void pdelay(unsigned long int);
void wait_for_press();
void set_led(boolean);


unsigned long int off_by(unsigned long int ideal, unsigned long int observed, boolean *is_late) {
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
void pdelay(unsigned long int us) {
  delay((unsigned long int) us / 1000);
  delayMicroseconds((unsigned int) us % 1000);
}

/* Attach a 10K resistor from ground to a pushbutton and pin 2. Connect the
 * other side of the button to 5V. pinMode(button_pin, INPUT); */
void wait_for_press() {

  // read the pushbutton input pin
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

  u8g.firstPage();
  //u8g.setRot180();
}

void loop() {

  // Far more straightforward on the Arduino than in standard C!
  int bpm = random(min_tempo, max_tempo + 1);
  const char *bpm_a = to_s(bpm);
  
  // then convert to microseconds/beat
  unsigned long int uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  // 0:   the one beat before the first user-beat
  // 1-8: the user's keypress times
  unsigned long int timed[9];

  // prepare other variables
  unsigned long int ideal, error;
  boolean is_late;

  char *c1[] = {"I'll show you", "a tempo, then", "match it!   [>]"};
  print(bpm_a, c1, 3);
  set_led(true);
  wait_for_press();

  char *c2[] = {"I'll give you", "8 beats, then", "tap out the [>]"};
  print(bpm_a, c2, 3);
  set_led(false);
  wait_for_press();

  char *c3[] = {"next 8 on the", "button. Press", "to start!   [>]"};
  print(bpm_a, c3, 3);
  set_led(true);
  wait_for_press();

  do {
    println(bpm_a, "Here we go!");
    set_led(false);
    pdelay(uspb);

    // count off eight beats
    for (int i = 1; i <= 6; i++) {
      println(bpm_a, to_s(i));
      set_led(i % 2);
      pdelay(uspb);
    }

    println(bpm_a, "7 ready");
    set_led(true);
    pdelay(uspb);

    // record the beat before the user's first
    println(bpm_a, "8 go");
    set_led(false);
    timed[0] = micros();

    // the user taps enter eight times
    // we measure the clock time of each
    for (int i = 1; i <= 8; i++) {
      wait_for_press();
      timed[i] = micros();

      println(bpm_a, to_s(i));
      set_led(i % 2);
    }
    pdelay(uspb);

    println(bpm_a, "Okay!");
    set_led(true);
    pdelay(4 * uspb);

    // Take the beat before the user started tapping. Project forward to see at
    // what time a perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 8);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    error = off_by(ideal, timed[8], &is_late);

    println(bpm_a, to_s(error / 1e6));
    pdelay(2 * uspb);
    set_led(false);

    println(bpm_a, is_late ? "late" : "early");
    pdelay(2 * uspb);

    // I'd like to divide this by tpb to give a better statistic#TODO

    println(bpm_a, "One");
    set_led(true);
    pdelay(2 * uspb);

    println(bpm_a, "more");
    set_led(false);
    pdelay(2 * uspb);

    println(bpm_a, "time?");
    set_led(true);
    pdelay(3 * uspb);

  } while (1);
}

void print(const char *bpm, char **body, int lines) {
  u8g.nextPage();
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(0, 10);
  u8g.print("TEMPO ");
  u8g.setPrintPos(60, 10);
  u8g.print(bpm);
  for (int i = 0; i < lines; i++) {
    u8g.setPrintPos(0, 30 + (25 * i));
    u8g.print(body[i]);
  }
}

void println(const char *bpm, const char *body) {
  char *body_wrap[] = {(char *) body};
  print(bpm, body_wrap, 1);
}
