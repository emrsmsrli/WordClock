# WORD CLOCK ARDUINO

The code in this repository is a heavily modified version of 
[Tinker's Word Clock code](http://www.instructables.com/id/Tinkers-Word-Clock-REVISITED-NOW-110-More-AWESOME-/). 
And uses a modified version of [OneButton](https://github.com/mathertel/OneButton) library.

## TODO
- [x] Add second support alongside of minutes and hours
- [x] Make the leds fade-in and -out when a change in time or color happens
- [x] Use external interrupts to count button clicks so we can increment both hour and minute with a single button
- [x] Implement smoothstep interpolation to led animations
- [ ] Implement night mode, where overall led intensity is 0.4 instead of 1.0