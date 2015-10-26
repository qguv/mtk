#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define buzzer_pin  3
#define hold_threshold 300 // in milliseconds
#define tap_timeout   1000 // in milliseconds
#define array_size(x) (sizeof(x)/sizeof(*x))

#include <EEPROM.h>

// Display: http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/
// OLED pins: SDA -> A4, SCL -> A5
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

boolean speaker_enabled = true;

namespace beep {
  typedef enum tone { LO, HI, RISE, FALL, CHIME, ERR, ENABLE, DISABLE } tone;
}

namespace ptime {
  typedef unsigned long int microseconds;
  typedef unsigned long int milliseconds; // currently unused
}

ptime::microseconds off_by(ptime::microseconds, ptime::microseconds, boolean *);
void pdelay(ptime::microseconds, ptime::microseconds *);
boolean wait_for_press(ptime::milliseconds);
boolean wait_was_that_a_hold();
void raw_beep(int, int);
void make_sound(beep::tone);
void print_many(const char *, char *[3]);
void print_one(const char *, const char *);
void rhythm_game();
void measure_tempo();
void entropy();
void information();
void menu();
void toggle_speaker();

ptime::microseconds off_by(ptime::microseconds ideal, ptime::microseconds observed, boolean *is_late) {
  if (observed > ideal) {
    *is_late = true;
    return observed - ideal;
  } else {
    *is_late = false;
    return ideal - observed;
  }
}

/* A more precise delay than Arduino's own with the penalty of quicker overflow
 * (70 minutes). Pass in the time before a screen blit to "begin" for a more
 * accurate time. */
void pdelay(ptime::microseconds total, ptime::microseconds *last) {

  // figure out how much time we've already spent "waiting" from printing to the screen, etc
  total -= micros() - *last;

  // types are different than usual milli-/microseconds because of the
  // differing precision of each type of delay
  delay((unsigned long int) total / 1000);
  delayMicroseconds((unsigned int) total % 1000);
  *last = micros();
}

/* Attach a 10K resistor from ground to a pushbutton and pin 2. Connect the
 * other side of the button to 5V. pinMode(button_pin, INPUT); */
boolean wait_for_press(ptime::milliseconds timeout_length) {

  boolean time_limited = (timeout_length > 0);
  ptime::milliseconds timeout = millis() + ((ptime::milliseconds) timeout_length);

  // you've got to release the button every time
  while (digitalRead(button_pin) != LOW) {
    if (time_limited && (millis() >= timeout)) { return false; }
    delay(5);
  }

  // wait until the button is pressed
  while (digitalRead(button_pin) != HIGH) {
    if (time_limited && (millis() >= timeout)) { return false; }
    delay(5);
  }

  return true;
}

// pauses execution until the button is pressed and released. returns how long the button was depressed.
boolean wait_was_that_a_hold() {

  wait_for_press(0);
  ptime::microseconds threshold_time = millis() + hold_threshold;

  // wait for button release or hold-sensing timeout
  while (digitalRead(button_pin) == HIGH) {

    // was the threshold reached?
    if (millis() >= threshold_time) { return true; }
    delay(5);
  }

  // ... or did we lift the button up before that?
  return false;
}

// Add to a digital out, remember to pinMode(buzzer_pin, OUTPUT).
void raw_beep(int cycles, int delay_time) {
  if (!speaker_enabled) { return; }
  for (int i = 0; i < cycles; i++) {
    digitalWrite(buzzer_pin, HIGH);
    delayMicroseconds(delay_time);
    digitalWrite(buzzer_pin, LOW);
    delayMicroseconds(delay_time);
  }
}

void make_sound(beep::tone which) {
  switch (which) {
    case beep::LO:
      raw_beep(15, 1000);
      break;
    case beep::HI:
      raw_beep(20, 750);
      break;
    case beep::RISE:
      make_sound(beep::LO);
      make_sound(beep::HI);
      break;
    case beep::FALL:
      make_sound(beep::HI);
      make_sound(beep::HI);
      make_sound(beep::LO);
      make_sound(beep::LO);
      break;
    case beep::CHIME:
      make_sound(beep::HI);
      make_sound(beep::LO);
      make_sound(beep::HI);
      break;
    case beep::ERR:
      make_sound(beep::FALL);
      delay(100);
      make_sound(beep::LO);
      make_sound(beep::LO);
      make_sound(beep::LO);
      make_sound(beep::LO);
      break;
    case beep::ENABLE:
      make_sound(beep::HI);
      make_sound(beep::HI);
      delay(100);
      make_sound(beep::HI);
      make_sound(beep::HI);
      make_sound(beep::HI);
      make_sound(beep::HI);
      break;
    case beep::DISABLE:
      make_sound(beep::LO);
      make_sound(beep::LO);
      delay(100);
      make_sound(beep::LO);
      make_sound(beep::LO);
      make_sound(beep::LO);
      make_sound(beep::LO);
      break;
  }
}

// Make a sound even if user has disabled sound output
void force_make_sound(beep::tone which) {
  boolean previous_state = speaker_enabled;
  speaker_enabled = true;
  make_sound(which);
  speaker_enabled = previous_state;
}

// The body string array must be three lines.
void print_many(const char *head, char *body[3]) {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    if (head[0] != '\0') {
      u8g.setPrintPos(0, 10);
      u8g.print(head);
    }
    for (int i = 0; i < 3; i++) {
      u8g.setPrintPos(0, 30 + (15 * i));
      u8g.print(body[i]);
    }
  } while (u8g.nextPage());
}

void print_one(const char *bpm, const char *body) {
  char *body_wrap[] = {"", (char *) body, ""};
  print_many(bpm, body_wrap);
}

void rhythm_game() {

  // Far more straightforward on the Arduino than in standard C!
  int bpm = random(min_tempo, max_tempo + 1);
  char bpm_s[16];
  (String("TEMPO ") + String(bpm)).toCharArray(bpm_s, 16);
  
  // then convert to microseconds/beat
  ptime::microseconds uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  ptime::microseconds timed[8];

  // prepare phrases
  char *count[] = {"       1",
                   "       2",
                   "       3",
                   "       4",
                   "       5",
                   "       6",
                   "       7",
                   "       8"};
  char *ready[] = {"", "       7", "     Ready?"};
  char *go[]    = {"", "       8", "      Go!"};
  char *again[] = {"", "", "    again?"};

  // prepare other variables
  ptime::microseconds ideal, error, last_beat;
  boolean is_late;
  char result_s[16];

  last_beat = micros();
  print_one(bpm_s, "");
  pdelay(uspb, &last_beat);

  while (true) {
    print_one(bpm_s, "  Here we go!");
    make_sound(beep::LO);
    pdelay(uspb, &last_beat);

    // count off eight beats
    for (int i = 0; i < 6; i++) {
      print_one(bpm_s, count[i]);
      make_sound(i % 4 == 0 ? beep::HI : beep::LO);
      pdelay(uspb, &last_beat);
    }

    print_many(bpm_s, ready);
    make_sound(beep::LO);
    pdelay(uspb, &last_beat);

    print_many(bpm_s, go);
    make_sound(beep::LO);

    // the user taps enter eight times
    // we measure the clock time of each
    for (int i = 0; i < 8; i++) {
      wait_for_press(0);
      timed[i] = micros();

      print_one(bpm_s, count[i]);
      make_sound(i % 4 == 0 ? beep::HI : beep::LO);
    }
    last_beat = timed[7];
    pdelay(uspb, &last_beat);

    print_one(bpm_s, "     Okay!");
    make_sound(beep::RISE);

    // Take the user's first beat. Project forward to see at what time a
    // perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 7);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    error = off_by(ideal, timed[7], &is_late);

    // Prepare results. I'd like to divide this by tpb to give better
    // statistics #TODO
    (String("  ") + String(error / 1e3, 0) + String(is_late ? "ms late" : "ms early")).toCharArray(result_s, 16);

    // delay on the beat, but not for too long
    pdelay((bpm > 130 ? 4 : 2) * uspb, &last_beat);

    // show results
    print_one(bpm_s, result_s);
    make_sound(beep::RISE);

    // delay on the beat, but not for too long
    pdelay((bpm > 130 ? 4 : 2) * uspb, &last_beat);

    again[1] = result_s;
    print_many(bpm_s, again);
    make_sound(beep::HI);

    if (wait_was_that_a_hold()) { return; }
  last_beat = micros();
  }
}

float bpm_guess(ptime::microseconds tpb[4], int current_beat) {
  ptime::microseconds the_sum = 0;
  int n = (current_beat >= 4) ? 4 : current_beat;
  for (int i = 0; i < n; i++) { the_sum += tpb[i]; }
  return 60 / ((the_sum / n) / 1e6);
}

void measure_tempo() {
  char *h = " Measure tempo:";
  char result_s[16];

  int beat = 1;
  ptime::microseconds current;
  ptime::microseconds time_per_beat[4];
  ptime::microseconds last = micros();
  print_one(h, "    Begin!");

  // get first beat; quit on hold as always
  if (wait_was_that_a_hold()) { return; }

  do {
    current = micros();
    time_per_beat[(beat - 1) % 4] = current - last;
    last = current;
    make_sound(beep::LO);
    (String("    ") + String(bpm_guess(time_per_beat, beat))).toCharArray(result_s, 16);
    print_one(h, result_s);
    beat++;

    // wait a while for a keypress
    if (!wait_for_press(tap_timeout)) {

      // if we didn't get any after some time, display the option to return to the home screen
      make_sound(beep::HI);
      char *m[] = { "", result_s, "    Return?" };
      print_many(h, m);

      // hold returns to the main menu; tap keeps bpm-counting (for very low bpm, e.g. crew)
      if (wait_was_that_a_hold()) { return; }
    }
  } while (true);
}

void entropy() {

  // available bases, in order
  // end in zero so I don't have to count the number of elements
  int bases[] = {10, 16, 6, 20, 12, 64, 7, 0};
  int base_i, base;
  char base_a[12];

  // digits correspond to each base
  char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/";

  // string buffers
  char line[16];
  char message[3][16];
  char *message_disp[3];

  // choose base, view entropy, repeat
  while (true) {
    base_i = 0;

    // iterate through possible bases
    while (true) {

      // grab the current base and move the index to the next (not selected) base
      base = bases[base_i++];

      // blank out second and last lines and prepare for string entry
      strcpy(message[0], "");
      message_disp[0] = &message[0][0];
      message_disp[1] = &message[1][0];
      message_disp[2] = &message[0][0];

      // if we're out of bases, show a "back" option
      if (base == 0) {
        base_i = 0;
        strcpy(message[1], "    Go back");

      // otherwise, build a normal screen to show to the user
      } else {
        (String("    Base ") + String(base)).toCharArray(base_a, 12);
        strcpy(message[1], base_a);
      }

      print_many(" Choose a base:", message_disp);

      // move to next or first base on press until user chooses one by holding
      if (wait_was_that_a_hold()) { break; }

      // indicate that we're going to the next base...
      if (base != 0) {
        make_sound(beep::LO);

      // ...or back to the beginning of the base list
      } else {
        if (base == 0) { make_sound(beep::HI); }
      }
    }

    // if we asked to go back, do so
    if (base == 0) { return; }

    // user has chosen a base
    // give random numbers until user asks for another base by holding
    do {
      make_sound(beep::RISE);

      for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 15; col++) {
          line[col] = digits[random(0, base)];
        }
        line[15] = '\0';
        strcpy(message[row], line);
      }
      // print random digits in chosen base
      message_disp[0] = &message[0][0];
      message_disp[1] = &message[1][0];
      message_disp[2] = &message[2][0];
      print_many(&base_a[0], message_disp);

    // touch continues, hold resets to base selection
    } while (!wait_was_that_a_hold());

    make_sound(beep::FALL);
  }
}

void menu() {
  char *titles[][3] = {{"",
                        "    Play a",
                        "  rhythm game"},

                       {"",
                        "    Measure",
                        "     tempo"},

                       {"",
                        "    Collect",
                        "    entropy"},

                       {"",
                        "     View",
                        "     info"},

                       {"",
                        "    Toggle",
                        "     sound"}};

  void (*funcs[])() = {rhythm_game, measure_tempo, entropy, information, toggle_speaker};

  // Display start screens to the user.
  int num_modes = array_size(titles);
  boolean chose_toggle_speaker = false;
  for (int i = 0;; i = (i + 1) % num_modes) {
    print_many("    Choose:", titles[i]);

    // if the user holds the button,
    if (wait_was_that_a_hold()) {

      // if use picked the last one, don't make beeps
      chose_toggle_speaker = (i == (num_modes - 1));

      // otherwise make start and finish beeps
      if (!chose_toggle_speaker) { make_sound(beep::RISE); }

      // execute the chosen menu option, and set up to return to the same menu item
      funcs[i--]();

      // beep once task finished unless it's been disabled
      if (!chose_toggle_speaker) { make_sound(beep::FALL); }

    // if the user taps the button, go to the next option
    } else {

      // beep high if we're returning to the first option
      if (i == num_modes - 1) {
        make_sound(beep::HI);

      // beep low if we're iterating to the next option
      } else {
        make_sound(beep::LO);
      }
    }
  }
}

void information() {
  char *titles[] =     {"   Made by:",
                        "   Made at:",
                        "  Return to:"};

  char *bodies[][3] = {{"     Quint",
                        "   Guvernator",
                        "github qguv/mtk"},

                       {"  revspace.nl",
                        "  Hackerspace",
                        "   Den Haag"},

                       {"Stamkartstraat",
                        "      117",
                        "2521EK Den Haag"}};

  // cycle through info pages on press, exit on hold
  int pages = array_size(titles);
  for (int i = 0;; i = (i + 1) % pages) {

    // show current page
    print_many(titles[i], bodies[i]);

    // next page on press, exit on hold
    if (wait_was_that_a_hold()) { return; }
    if (i == pages - 1) {
      make_sound(beep::HI);
    } else {
      make_sound(beep::LO);
    }
  }
}

void toggle_speaker() {
  speaker_enabled = !speaker_enabled;
  EEPROM.write(0, speaker_enabled ? 1 : 0);
  force_make_sound(speaker_enabled ? beep::ENABLE : beep::DISABLE);
}

void setup() {
  speaker_enabled = !(EEPROM.read(0) == 0);

  // seed the random generator
  long int seed = 0;

  // we'll make a random seed by sampling bits of noise until we've filled a
  // long int, the type used to seed arduino's random() function
  for (int i = 0; i < (sizeof(long int) * 8); i++) {

    // sample some analog noise
    // take the least significant bit
    // prepend it to an array of bits represented by a long int
    seed |= (((long int) (analogRead(0) & 1)) << i);
  }
  randomSeed(seed);

  // initialize the button pin as a input
  pinMode(button_pin, INPUT);

  // initialize the buzzer as an output
  pinMode(buzzer_pin, OUTPUT);
  digitalWrite(buzzer_pin, LOW);

  u8g.firstPage();
}

void loop() {
  char *greeting[3] = {"  Compiled on", "  " __DATE__, "  at " __TIME__};
  print_many("      MTK     ", greeting);
  force_make_sound(beep::CHIME);
  delay(350);
  menu();
}
