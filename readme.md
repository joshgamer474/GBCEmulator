# GBCEmulator

[![Build Status](https://travis-ci.org/joshgamer474/GBCEmulator.svg?branch=qt_gui)](https://travis-ci.org/joshgamer474/GBCEmulator)
A WIP Gameboy (Color) emulator written in C++ and packaged in Conan

## Progress

The emulator currently passes all of blargg's cpu instruction tests and run a decent amount of GB/GBC games.

![](https://github.com/joshgamer474/GBCEmulator/raw/master/res/blargg_cpu_intrs.gif)


## Issues

* Some GBC games do not boot up
* Qt wrap needs some improvement

## How to build

### Prerequisites

* CMake 3.x
* Python 3.x
* pip3
* Conan.io via pip
* Conan package SDL/2.0.8@josh/stable
##### Prerequisite Commands

```pip3 install conan --user```

```conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan```

## Building

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator```

```conan install . -if=build --build=outdated```

```conan build . -bf=build```

## Building Qt wrapped GBCEmulator

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator```

```conan export conanfile.py josh/testing```

```cd qt_wrap```

```conan install . -if=build --build=outdated```

```conan build . -bf=build```

### How to use
Drag and drop your favorite rom into the built GBCEmulator.exe or into GBCEmulator_qt.exe.


# TODO

## Implement some Core features
- [x] GPU Sprite Rendering
- [x] Input handling using SDL
- [x] Add timing and sleeping to core while() loop so the emulator runs at its proper speed
- [x] Sound
- [x] Controls
- [x] Create a GUI (maybe Qt?)
- [x] Linux build support

## Organization
- [x] Implement Conan.io packaging for libraries
- [x] Implement Conan.io package for this repo
- [x] Include CMake building


## Testing

### Core feature testing and correctness
- [ ] Memory Bank Controllers
- [ ] Interrupt handling correctness
- [ ] GPU background rollover testing
- [ ] Get a game to actually boot

### Create unit tests using Boost
- [x] Implement for blargg's cpu_instrs
- [ ] Implement for blargg's dmg_sound
- [ ] Implement for blargg's instr_timing
- [ ] Implement for blargg's interrupt_time
- [ ] Implement for blargg's oam_bug
- [ ] Implement for blargg's mem_timing
- [ ] Implement for blargg's mem_timing-2
- [ ] Implement for blargg's cgb_sound
