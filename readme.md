# GBCEmulator
A WIP Gameboy (Color) emulator written in C++ and packaged in Conan

## Instructions

### Prerequisites

* Python 3.x
* pip
* conan.io via pip

```pip install conan```

### How to build

```git clone https://github.com/joshgamer474/GBCEmulator.git```

```cd GBCEmulator```

```conan install . --install-folder build```

```conan build . --install-folder build```

### How to use
Drag and drop your favorite rom into the built GBCEmulator.exe


## Progress

The emulator currently passes all of blargg's individual cpu instruction tests, however it prematurely passes the all-in-one cpu_instrs.gb test.

![](https://github.com/joshgamer474/GBCEmulator/raw/master/res/blargg_cpu_intrs.gif)

## TODO

A few core features need to be ironed out and tested more:
- [ ] Memory Bank Controllers
- [ ] Interrupt handling correctness
- [ ] GPU background rollover testing
- [ ] Get a game to actually boot


Core features that need to be implemented:
- [ ] GPU Sprite Rendering
- [ ] Input handling using SDL
- [ ] Add timing and sleeping to core while() loop so the emulator runs at its proper speed


Organization features:
- [x] Implement Conan.io packaging for libraries
- [x] Implement Conan.io package for this repo
- [x] Include CMake building


## Issues

Currently all of blargg's cpu .gb tests pass individually, however the all-in-one cpu_instrs.gb does not and stops after supposedly passing cpu test 07-jr,jp,call,ret,rst.gb

Games currently do not boot as they get stuck in an infinite loop - probably related to blargg's cpu_instrs.gb issue.