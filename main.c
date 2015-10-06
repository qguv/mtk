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
	button_state = digitalRead(button_pin);

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

int random_tempo() { return random(min_tempo, max_tempo + 1); }

/* Should be built into your board, remember to pinMode(led_pin, OUTPUT). */
void toggle_led(int last) { digitalWrite(led_pin, LOW ? last : HIGH); }

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

int loop() {

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
	int *led_is_on;
	unsigned long int ideal;
	double difference;
	int is_late;

	Serial.println("I'll show you a tempo, then match it!");
	Serial.println("I'll give you eight beats, then tap out the next eight on the return key.");
	Serial.println("Press the button to begin.");
	toggle_led(&led_is_on);
	wait_for_press();

	Serial.print("Tempo is ");
	Serial.println(bpm);
	delay(1000);

	do {
		Serial.println("Here we go!");
		toggle_led(&led_is_on);
		delayMicroseconds(uspb);

		// count off eight beats
		for (int i = 1; i <= 6; i++) {
			Serial.println(i);
			toggle_led(&led_is_on);
			delayMicroseconds(uspb);
		}

		Serial.println("7 ready");
		toggle_led(&led_is_on);
		delayMicroseconds(uspb);

		// record the beat before the user's first
		Serial.println("8 go");
		toggle_led(&led_is_on);
		timed[0] = micros();

		// the user taps enter eight times
		// we measure the clock time of each
		for (int i = 1; i <= 8; i++) {
			wait_for_press();
			timed[i] = micros();

			Serial.println(i);
			toggle_led(&led_is_on);
		}
		delayMicroseconds(uspb);

		Serial.println("Okay!");
		toggle_led(&led_is_on);
		delayMicroseconds(4 * uspb);

		// Take the beat before the user started tapping. Project forward to see at
		// what time a perfect timekeeper would tap the eighth and final beat.
		ideal = timed[0] + (tpb * 8);

		// We then compare the user's final beat and the ideal final beat to
		// determine how far off the user was in total.
		difference = (timed[8] - ideal) / 1e6;
		is_late = (difference > 0);

		Serial.print(difference);
		Serial.print("s ");
		Serial.println("late" ? is_late : "early");
		toggle_led(&led_is_on);
		delayMicroseconds(4 * uspb);

		// I'd like to divide this by tpb to give a better statistic#TODO

		Serial.println("One");
		toggle_led(&led_is_on);
		delayMicroseconds(2 * uspb);

		Serial.println("more");
		toggle_led(&led_is_on);
		delayMicroseconds(2 * uspb);

		Serial.println("time?");
		toggle_led(&led_is_on);
		delayMicroseconds(3 * uspb);

	} while (1);
}
