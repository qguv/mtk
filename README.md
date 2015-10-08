# Rhythmoid
_a very simple rhythm game and shield for the Arduino Nano_

## Parts

  - [Arduino Nano](https://www.arduino.cc/en/Main/ArduinoBoardNano) or compatible device; if you're not using the custom shield PCB, any Arduino will work
  - headers for the above; one male set for the Nano and one female set for the shield, if you're using it
  - [128x64 I2C OLED display](http://www.hobbyelectronica.nl/product/128x64-oled-geel-blauw-i2c/)
  - a little button, normally open, closes on push; SPST/DPST/nPST-NO should be fine
  - a piezo buzzer that doesn't oscillate on its own, i.e. when you give it DC, it shouldn't make sustained noise
  - ~10kΩ resistor (anything is fine, even 1k will do, you're just pulling down, but higher impedence means less unnecessary power consumption)

## Wiring

Look at [the included Eagle files](eagle/). Here's a short explanation:

  - power (3.3v for mine) and ground the OLED display
  - attach SCL to analog pin 5 and SDA fo analog pin 4
  - make a voltage divider with 5V and the 10kΩ resistor, and make the button interrupt current from 5V to the resistor
  - connect the not-5V side of the switch to digital pin 2; this tells the board whether we're pressing the button
  - connect the (admittedly optional) buzzer to digital pin 3 and ground; add a resistor if it's too loud
  - power the board with at least 5V DC unless your board supports 3.3V

## Shield PCB

![Rhythmoid PCB](rhythmoid_pcb.png)

## Gameplay

Press the button to get through the initial menus. A tempo will be chosen pseudo-randomly between 60 (clock) and 180 (hardstyle). The board will count eight beats at that tempo. You then tap out the next eight at the same tempo. It will tell you how much, in seconds, you drifted from the original tempo. More sophisticated statistics coming soon; I'd like a score that allows fairly equal comparison between performance at different tempos.

Some specifics:

  - beats 1 and 4 of the eight-beat segments are accompanied by a high buzzer chirp; other beats have a low chirp
  - you can pause between the machine's 8 beats and your 8 beats--it won't count against you
  - the only thing that's (currently) measured is the first and last beats; you can rush through beats 2 through 7 and it won't count against you as long as you end up pressing beat 8 at the right time
  - the game repeats at the same tempo; use the board's reset button/pin or cycle power to get a new tempo
  - the calls to wait are very precise, on the order of 4μs or so (depending on the board) and account for screen-blit time, sound playback time, pin read/write time, etc.
  - the arduino IDE is terrible and doesn't support sophisticated macros or typedefs, so types are uglier than I'd like them to be to keep the project accessible to folks not using `ino` to flash their boards
  - the biggest memory hog is u8glib, which only needs to be imported once, so loads of additional code (other rhythm games) would easily fit in memory, even on the nano/micro/pro micro/etc.
