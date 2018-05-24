# GBCEmulator
A WIP Gameboy (Color) emulator written in C++ and packaged in Conan

## Progress

The emulator currently passes all of blargg's individual cpu instruction tests, however it prematurely passes the all-in-one cpu_instrs.gb test.

![](https://github.com/joshgamer474/GBCEmulator/raw/master/res/blargg_cpu_intrs.gif)

## How to build

### Prerequisites

* Python 3.x
* pip
* Conan.io via pip
* Conan package SDL/2.0.8@josh/stable

```pip install conan```

```git clone https://github.com/joshgamer474/conan-SDL```

```cd conan-SDL```

```conan export conanfile.py josh/stable```

### Building

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator```

```conan install . --install-folder build```

```conan build . --build-folder build```

### How to use
Drag and drop your favorite rom into the built GBCEmulator.exe


## Issues

Currently all of blargg's cpu .gb tests pass individually, however the all-in-one cpu_instrs.gb does not and stops after supposedly passing cpu test 07-jr,jp,call,ret,rst.gb

Games currently do not boot as they get stuck in an infinite loop - probably related to blargg's cpu_instrs.gb issue.


# TODO

## Implement some Core features
- [ ] GPU Sprite Rendering
- [ ] Input handling using SDL
- [ ] Add timing and sleeping to core while() loop so the emulator runs at its proper speed
- [ ] Sound
- [ ] Controls
- [ ] Create a GUI (maybe Qt?)

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
