#define _DEFAULT_SOURCE // usleep, timeradd, ...
#include <stdlib.h>     // rand
#include <time.h>       // time
#include <stdio.h>      // printf
#include <unistd.h>     // usleep
#include <sys/time.h>   // gettimeofday, timeradd, ...

#define max_tempo 181
#define min_tempo 60

int random_tempo();
void timeradd_repeat(int repeat, struct timeval *, struct timeval *, struct timeval *);
void println_time(struct timeval *);
void println_time_error(struct timeval *, struct timeval *);

int random_tempo() {
	// generate number from 0 t/m RAND_MAX
	double r = (double) rand();

	// scale so that the range reflects the range of tempos
	r *= ((float) (max_tempo - min_tempo) / RAND_MAX);

	// change minimum from zero to the actual minimum
	return min_tempo + (int) r;
}

// repeat > 0 please
void timeradd_repeat(int repeat, struct timeval *begin, struct timeval *addend, struct timeval *result) {

	timeradd(begin, addend, result);
	for (int i = 1; i < repeat; i++) {
		timeradd(result, addend, result);
	}
}

void println_time(struct timeval *t) {

	// timeval works normally from 0.0 upward; display it normally
	if (t->tv_sec >= 0) {
		printf("%ld.%06ld\n",
				(long int) t->tv_sec,
				(long int) t->tv_usec
		);

	// ...but timeval does weird stuff below zero. Since timeval is really tv_sec
	// + tv_usec, values below zero will have a negative tv_sec and a positive
	// tv_usec. For instance, -3.2 seconds will encode as -4 seconds plus 8e5
	// microseconds. To fix this, we add one to tv_sec and subtract a million to
	// "invert" the microsecond display (so it's hanging off the negative number
	// above, not the negative number below). Finally, when we do this, the
	// numbers between 0 and -1 will not have a negative sign, so we'll add that
	// if necessary.

	} else {

		// add the negative sign for numbers between 0 and -1
		if (t->tv_sec == -1) { printf("-"); }

		printf("%ld.%06ld\n",
				(long int) t->tv_sec + 1,

				// "flip around" the microseconds so they describe a relationship with
				// a number closer to zero, not a number further away from zero
				(long int) 1e6 - t->tv_usec
		);
	}
}

void println_time_error(struct timeval *ideal, struct timeval *actual) {

	struct timeval delta;
	char *desc;

	// we'll assume we're too slow and subtract the hypothetically bigger actual
	// time from the hypothetically smaller ideal time
	timersub(actual, ideal, &delta);
	desc = "slow";

	// if we were fast, we just recalculate the other way 'round
	if (delta.tv_sec < 0) {
		timersub(ideal, actual, &delta);
		desc = "fast";
	}

	printf("%ld.%06lds %s\n",
			(long int) delta.tv_sec,
			(long int) delta.tv_usec,
			desc
	);
}

int main() {
	// seed the random generator
	srand(time(NULL));

	// make a tempo in beats/minute
	// then convert to nanoseconds/beat
	// and microseconds/beat
	int bpm = random_tempo();
	struct timeval tpb;
	tpb.tv_sec  = 0;
	tpb.tv_usec = 6e7 / bpm;

	// prepare an array to store time measurements
	// 0:   the one beat before the first user-beat
	// 1-8: the user's keypress times
	struct timeval timed[9];

	printf("\nI'll show you a tempo, then match it!\nI'll give you eight beats, then tap out the next eight\non the return key.\n\nPress return to begin.\n");
	getchar();

	printf("Tempo is %d.\n", bpm);
	sleep(1);

	do {
		printf("\nHere we go!\n");
		usleep(tpb.tv_usec);

		// count off eight beats
		for (int i = 1; i <= 6; i++) {
			printf("%d\n", i);
			usleep(tpb.tv_usec);
		}

		printf("7 ready\n");
		usleep(tpb.tv_usec);

		// record the beat before the user's first
		printf("8 go\n");
		gettimeofday(timed+0, NULL);

		// the user taps enter eight times
		// we measure the clock time of each
		for (int i = 1; i <= 8; i++) {
			getchar();
			gettimeofday(timed+i, NULL);
			printf("%d", i);
		}
		printf("\n\n");
		usleep(tpb.tv_usec);

		printf("Okay!\n");
		usleep(4 * tpb.tv_usec);

		// calculating score
		struct timeval ideal;

		// Take the beat before the user started tapping. Project forward to see at
		// what time a perfect timekeeper would tap the eighth and final beat.
		timeradd_repeat(8, timed, &tpb, &ideal);

		// We then compare the user's final beat and the ideal final beat to
		// determine how far off the user was in total. I'd like to divide this by
		// tpb, but I don't think this is easily possible. #TODO
		println_time_error(&ideal, timed+8);
		usleep(4 * tpb.tv_usec);

		printf("\nOne\n");
		usleep(2 * tpb.tv_usec);
		printf("more\n");
		usleep(2 * tpb.tv_usec);
		printf("time?\n");
		usleep(3 * tpb.tv_usec);

	} while (1);
}
