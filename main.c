#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define buzzer_pin  3
#define num_modes   2
#define hold_threshold 300000 // in microseconds

// Display: http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/
// OLED pins: SDA -> A4, SCL -> A5
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

namespace beep {
  typedef enum tone { LO, HI, RISE, FALL, GREET, ERR } tone;
}

unsigned long int off_by(unsigned long int, unsigned long int, boolean *);
void pdelay(unsigned long int, unsigned long int *);
void wait_for_press();
boolean wait_was_that_a_hold();
void raw_beep(int, int);
void make_sound(beep::tone);
void print_many(const char *, char *[3]);
void print_one(const char *, const char *);
void straight_eight();
void entropy();
void menu();

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
 * (70 minutes). Pass in the time before a screen blit to "begin" for a more
 * accurate time. */
void pdelay(unsigned long int total, unsigned long int *last) {

  // figure out how much time we've already spent "waiting" from printing to the screen, etc
  total -= micros() - *last;

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
  unsigned long int start = micros();
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
      make_sound(beep::LO);
      break;
    case beep::GREET:
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

void eight_by_two() {

  // Far more straightforward on the Arduino than in standard C!
  int bpm = random(min_tempo, max_tempo + 1);
  char bpm_s[16];
  (String("TEMPO ") + String(bpm)).toCharArray(bpm_s, 16);
  
  // then convert to microseconds/beat
  unsigned long int uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  unsigned long int timed[8];

  // prepare phrases
  char *p1[] = {"I'll show you a",
                "tempo, then try",
                "to match it!   "};
  char *p2[] = {"I'll give you 8",
                "beats, then tap",
                "the next 8 on  "};
  char *p3[] = {"the button.    ",
                "Press to start!", ""};
  char *p4[] = {"     One", "", ""};
  char *p5[] = {"     One",
                "     more", ""};
  char *p6[] = {"     One",
                "     more",
                "     time?"};
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

  // prepare other variables
  unsigned long int ideal, error, last_beat;
  boolean is_late;
  char result_s[16];

  print_many(bpm_s, p1);
  make_sound(beep::LO);
  wait_for_press();

  print_many(bpm_s, p2);
  make_sound(beep::LO);
  wait_for_press();

  print_many(bpm_s, p3);
  make_sound(beep::LO);
  wait_for_press();

  last_beat = micros();
  print_one(bpm_s, "");
  make_sound(beep::LO);
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
    pdelay(4 * uspb, &last_beat);

    // Take the user's first beat. Project forward to see at what time a
    // perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 7);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    error = off_by(ideal, timed[7], &is_late);

    (String("  ") + String(error / 1e6) + String(is_late ? " late" : " early")).toCharArray(result_s, 16);
    print_one(bpm_s, result_s);
    make_sound(beep::RISE);
    pdelay(4 * uspb, &last_beat);

    // I'd like to divide this by tpb to give a better statistic #TODO

    print_many(bpm_s, p4);
    make_sound(beep::LO);
    pdelay(2 * uspb, &last_beat);

    print_many(bpm_s, p5);
    make_sound(beep::LO);
    pdelay(2 * uspb, &last_beat);

    print_many(bpm_s, p6);
    make_sound(beep::LO);
    pdelay(3 * uspb, &last_beat);
  }
}

void entropy() {

  // available bases, in order
  // end in zero so I don't have to count the number of elements
  int bases[] = {10, 16, 6, 20, 12, 2, 6, 8, 4, 3, 7, 0};

  // digits correspond to each base
  char digits[] = "0123456789abcdefghij";

  // to store the current base as a string (String#toCharArray)
  char base_s[3];

  // print_many display buffer
  char *message[3] = { "", "Choose a base:", "" };

  // generate string line-by-line
  char head[16];

  // index and value of current base in list
  int base_i, base;

  base_i = 0;
  while (true) {
    // display the current base
    base = bases[base_i];
    String(base).toCharArray(message[2], 3);
    print_many("Entropy", message);

    if (wait_was_that_a_hold()) { 

      do {
        // generate random string
        for (int row = 0; row < 3; row++) {
          for (int col = 0; col < 15; col++) {
            message[row][col] = digits[random(0, base + 1)];
          }
          message[row][15] = '\0';
        }

        // display the random string
        strcpy(head, "Entropy base ");
        strcat(head, base_s);
        print_many(head, message);
        base_i = 0;

      // if you hold, you'll choose a new base.
      // if you tap, you'll get new random numbers in the same base.
      } while (!wait_was_that_a_hold());
    }

    // reset if we run out of bases
    if (bases[++base_i] == 0) { base_i = 0; }
  }
}

void menu() {
  char *titles[][3] = {{"",
                        "    8 by 2",
                        "  rhythm game"},

                       {"",
                        "Generate random",
                        "numbers, etc."}};

  void (*funcs[])() = {eight_by_two, entropy};

  // Display start screens to the user. When adding new games, increment num_modes
  while (true) {
    for (int i = 0; i < num_modes; i++) {
      print_many("Choose:", titles[i]);
      if (wait_was_that_a_hold()) { funcs[i](); i--; }
      make_sound(beep::RISE);
    }
  }
}

void setup() {

  // seed the random generator
  Serial.begin(9600);
  long int seed = 0;
  for (int i = 0; i < sizeof(long int); i++) {
    seed &= (analogRead(0) << i);
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
  print_one(" The Appliance ", "    Welcome    ");
  make_sound(beep::GREET);
  delay(400);
  menu();
}
