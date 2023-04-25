# CHIP-8 Emulator (C with SDL)
Yet another CHIP-8 emulator written in C with SDL.

This is my attempt to start exploring the emulation world. I used [this](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) amazing post to guide myself through the CHIP-8 "hardware" and its concepts. If you wanna start on this world as well, I highly recommend you to not take a look at a single line of code of this repository, but to read the entire aforementioned article and write you own stuff. That is how you're really gonna develop your skills!

The code is pretty simple, with NO software patterns at all. The only intention with this small project was to learn and have fun =)

## How to use it

First of all, you need to install SDL 2.0. It was used to get the user's input from keyboard and to print the pixels on the screen. I followed [this](https://medium.com/@edkins.sarah/set-up-sdl2-on-your-mac-without-xcode-6b0c33b723f7) article to do that.

After installing SDL, run the following command in the project folder to compile the emulator.

```
make emulator
```

Then, to run the emulator you need to execute the following command:

```
./chip_8_emulator.o <PATH_TO_ROM>
```

In the `roms` folder there are a few roms for you to test the emulator. The CHIP-8 (COSMAC VIP) keyboard was mapped to the following QWERTY keyboard's keys:

```
1-2-3-4
q-w-e-r
a-s-d-f
z-x-c-v
