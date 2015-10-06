#define max_tempo 181
#define min_tempo  60
#define button_pin  2
#define led_pin    13
#define buzzer_pin  3

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);
// Display: http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/
// OLED pins: SDA -> A4, SCL -> A5

unsigned long int off_by(unsigned long int, unsigned long int, boolean *);
void pdelay(unsigned long int, unsigned long int *);
void wait_for_press();
void beat(boolean);

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

/* Should be built into your board, remember to pinMode(led_pin, OUTPUT). */
void beat(boolean state) {
  digitalWrite(led_pin, state ? HIGH : LOW);
  for (int i = 0; i < 20; i++) {
    digitalWrite(buzzer_pin, HIGH);
    delayMicroseconds(750);
    digitalWrite(buzzer_pin, LOW);
    delayMicroseconds(750);
  }
}

void setup() {

  // seed the random generator
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // initialize the button pin as a input
  pinMode(button_pin, INPUT);

  // initialize the LED as an output and turn it off
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // initialize the buzzer as an output
  pinMode(buzzer_pin, OUTPUT);

  u8g.firstPage();
  //u8g.setRot180();
}

void loop() {

  // Far more straightforward on the Arduino than in standard C!
  int bpm = random(min_tempo, max_tempo + 1);
  char bpm_s[16];
  String(bpm).toCharArray(bpm_s, 16);
  
  // then convert to microseconds/beat
  unsigned long int uspb = 6e7 / bpm;

  // prepare an array to store time measurements
  unsigned long int timed[8];

  // prepare phrases
  char *title[] = {"", " ~ RHYTHMOID ~ ", ""};
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

  print_many("", title);
  beat(true);
  wait_for_press();

  print_many(bpm_s, p1);
  beat(false);
  wait_for_press();

  print_many(bpm_s, p2);
  beat(true);
  wait_for_press();

  print_many(bpm_s, p3);
  beat(false);
  wait_for_press();

  last_beat = micros();
  print_one(bpm_s, "");
  beat(true);
  pdelay(uspb, &last_beat);

  do {
    print_one(bpm_s, "  Here we go!");
    beat(false);
    pdelay(uspb, &last_beat);

    // count off eight beats
    for (int i = 1; i <= 6; i++) {
      print_one(bpm_s, count[i - 1]);
      beat(i % 2);
      pdelay(uspb, &last_beat);
    }

    print_many(bpm_s, ready);
    beat(true);
    pdelay(uspb, &last_beat);

    print_many(bpm_s, go);
    beat(false);

    // the user taps enter eight times
    // we measure the clock time of each
    for (int i = 0; i < 8; i++) {
      wait_for_press();
      timed[i] = micros();

      print_one(bpm_s, count[i]);
      beat(!(i % 2));
    }
    last_beat = timed[7];
    pdelay(uspb, &last_beat);

    print_one(bpm_s, "     Okay!");
    beat(true);
    pdelay(4 * uspb, &last_beat);

    // Take the user's first beat. Project forward to see at what time a
    // perfect timekeeper would tap the eighth and final beat.
    ideal = timed[0] + (uspb * 7);

    // We then compare the user's final beat and the ideal final beat to
    // determine how far off the user was in total.
    error = off_by(ideal, timed[7], &is_late);

    (String("  ") + String(error / 1e6) + String(is_late ? " late" : " early")).toCharArray(result_s, 16);
    print_one(bpm_s, result_s);
    beat(false);
    pdelay(4 * uspb, &last_beat);

    // I'd like to divide this by tpb to give a better statistic #TODO

    print_many(bpm_s, p4);
    beat(true);
    pdelay(2 * uspb, &last_beat);

    print_many(bpm_s, p5);
    beat(false);
    pdelay(2 * uspb, &last_beat);

    print_many(bpm_s, p6);
    beat(true);
    pdelay(3 * uspb, &last_beat);

  } while (1);
}

// The body string array must be three lines.
void print_many(const char *bpm, char *body[3]) {
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.setPrintPos(0, 10);
    u8g.print("TEMPO ");
    u8g.setPrintPos(45, 10);
    u8g.print(bpm);
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
