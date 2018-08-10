# GBCEmulator
A WIP Gameboy (Color) emulator written in C++ and packaged in Conan

## Progress

The emulator currently passes all of blargg's cpu instruction tests.

![](https://github.com/joshgamer474/GBCEmulator/raw/master/res/blargg_cpu_intrs.gif)

## How to build

### Prerequisites

* CMake
* Python 3.x
* pip
* Conan.io via pip
* Conan package SDL/2.0.8@josh/stable

```pip install conan```

```conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan```

## Building

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator```

```conan install . --install-folder build```

```conan build . --build-folder build```

## Building Qt wrapped GBCEmulator

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator/qt_wrap```

```conan install . --install-folder build```

```conan build . --build-folder build```

### How to use
Drag and drop your favorite rom into the built GBCEmulator.exe or into GBCEmulator_qt.exe


## Issues

Most Games currently do not boot as they get stuck in an infinite instruction loop.

Dr. Mario currently boots (!).


# TODO

## Implement some Core features
- [ ] GPU Sprite Rendering
- [ ] Input handling using SDL
- [ ] Add timing and sleeping to core while() loop so the emulator runs at its proper speed
- [ ] Sound
- [ ] Controls
- [ ] Create a GUI (maybe Qt?)
- [ ] Linux build support

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
