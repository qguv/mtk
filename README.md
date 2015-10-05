todo for Arduino port:

  - Replace `getch()` with [Arduino pushbutton](https://www.arduino.cc/en/Tutorial/Button)
  - Replace `gettimeofday()` with [Arduino `micros()`](https://www.arduino.cc/en/Reference/Micros)
  - Replace `sleep()` with [Arduino `delay`](https://www.arduino.cc/en/Reference/Delay) and [`delayMicroseconds`](https://www.arduino.cc/en/Reference/DelayMicroseconds)
  - Replace `printf()` with more complicated calls to the [graphics library](https://github.com/olikraus/u8glib/wiki) (OLED)
  - Replace `rand()`/`seed()` with [Arduino `random()`/`randomSeed()`](https://www.arduino.cc/en/Reference/RandomSeed)
