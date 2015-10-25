#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define buzzer_pin  3
#define hold_threshold 300000 // in microseconds

#define array_size(x) (sizeof(x)/sizeof(*x))

// Display: http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/
// OLED pins: SDA -> A4, SCL -> A5
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

namespace beep {
  typedef enum tone { LO, HI, RISE, FALL, CHIME, ERR } tone;
}

namespace ptime {
  typedef unsigned long int microseconds;
  typedef unsigned int      milliseconds;
}

ptime::microseconds off_by(ptime::microseconds, ptime::microseconds, boolean *);
void pdelay(ptime::microseconds, ptime::microseconds *);
void wait_for_press();
boolean wait_was_that_a_hold();
void raw_beep(int, int);
void make_sound(beep::tone);
void print_many(const char *, char *[3]);
void print_one(const char *, const char *);
void rhythm_game();
void entropy();
void information();
void menu();

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

// pauses execution until the button is pressed and released. returns how long the button was depressed.
boolean wait_was_that_a_hold() {

  wait_for_press();
  ptime::microseconds start = micros();
  unsigned int button_state;

  // wait for button release or hold-sensing timeout
  while (true) {
    delay(5);
    button_state = digitalRead(button_pin);

    // hold-sensing timeout reached
    if ((micros() - start) > hold_threshold) {
      return true;

    // button released before timeout expired
    } else if (button_state == LOW) {
      return false;
    }
  }
}

// Add to a digital out, remember to pinMode(buzzer_pin, OUTPUT).
void raw_beep(int cycles, int delay_time) {
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
      make_sound(beep::FALL);
      break;
  }
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
  make_sound(beep::RISE);
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
      wait_for_press();
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
    pdelay((bpm > 100 ? 4 : 2) * uspb, &last_beat);

    // show results
    print_one(bpm_s, result_s);
    make_sound(beep::RISE);

    // delay on the beat, but not for too long
    pdelay((bpm > 100 ? 4 : 2) * uspb, &last_beat);

    again[1] = result_s;
    print_many(bpm_s, again);
    make_sound(beep::HI);

    if (wait_was_that_a_hold()) {
      make_sound(beep::FALL);
      return;
    }
  last_beat = micros();
  }
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

  make_sound(beep::RISE);

  // once you're in the mode, you can't return to the menu unless you reset
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
    if (base == 0) {
      make_sound(beep::FALL);
      return;
    }

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
                        "    Collect",
                        "    entropy"},

                       {"",
                        "     View",
                        "     info"}};

  void (*funcs[])() = {rhythm_game, entropy, information};

  // Display start screens to the user.
  int num_modes = array_size(titles);
  for (int i = 0;; i = (i + 1) % num_modes) {
    print_many("    Choose:", titles[i]);
    if (wait_was_that_a_hold()) { funcs[i](); i--; }

    // beep high if we're returning to the first option
    if (i == num_modes - 1) {
      make_sound(beep::HI);

    // beep low if we're iterating to the next option
    } else {
      make_sound(beep::LO);
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

  make_sound(beep::RISE);

  // cycle through info pages on press, exit on hold
  int pages = array_size(titles);
  for (int i = 0;; i = (i + 1) % pages) {

    // show current page
    print_many(titles[i], bodies[i]);

    // next page on press, exit on hold
    if (wait_was_that_a_hold()) {
      make_sound(beep::FALL);
      return;
    }
    if (i == pages - 1) {
      make_sound(beep::HI);
    } else {
      make_sound(beep::LO);
    }
  }
}

void setup() {

  // seed the random generator
  Serial.begin(9600);
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
  //u8g.setRot180();
}

void loop() {
  char *greeting[3] = {"  Compiled on", "  " __DATE__, "  at " __TIME__};
  print_many(" ~    mtk    ~", greeting);
  make_sound(beep::CHIME);
  delay(400);
  menu();
}