#ifndef TEST_PACKAGE_SRC_ROM_TEST_FIXTURE_H
#define TEST_PACKAGE_SRC_ROM_TEST_FIXTURE_H

#include <gtest/gtest.h>
#include <SDL.h>
#include <UnitTests.h>
#include <experimental/filesystem>
#include <atomic>
#include <array>
#include <memory>
#include <GBCEmulator.h>
#include <string>

class ROMTestFixture : public ::testing::Test
{
protected:
    ROMTestFixture();
    virtual ~ROMTestFixture() {}

    template <typename T>
    void SetUp(T _unit_test)
    {
        init();
        unit_test = getUnitTest(_unit_test);
        test();
    }
    virtual void TearDown()
    {
        if (hash_passed == false)
        {
            // Save last frame and frame hash
            auto pngPath = unit_test.rom_path;
            pngPath += "_" + std::to_string(curr_hash) + "_";
            pngPath += "failed.png";
            emu->saveFrameToPNG(pngPath);

            // Destruct emulator
            emu.reset();

            // Remove .log file
            auto logPath = unit_test.rom_path;
            logPath += ".log";
            if (std::experimental::filesystem::exists(logPath))
            {
                std::experimental::filesystem::remove(logPath);
            }

            // Remove .sav file
            auto savPath = unit_test.rom_path.replace_extension(".sav");
            if (std::experimental::filesystem::exists(savPath))
            {
                std::experimental::filesystem::remove(savPath);
            }
        }
    }

    void init();
    void test();
    void frameUpdatedFunction(std::array<SDL_Color, SCREEN_PIXEL_TOTAL>& /* frame */);
    ROMUnitTest getUnitTest(blargg::cgb_sound test);
    ROMUnitTest getUnitTest(blargg::cpu_instrs test);
    ROMUnitTest getUnitTest(blargg::dmg_sound test);
    ROMUnitTest getUnitTest(blargg::instr_timing test);
    ROMUnitTest getUnitTest(blargg::interrupt_time test);
    ROMUnitTest getUnitTest(blargg::mem_timing test);
    ROMUnitTest getUnitTest(blargg::mem_timing_2 test);
    ROMUnitTest getUnitTest(blargg::oam_bug test);
    ROMUnitTest getUnitTest(blargg::halt_bug test);

private:
    std::unique_ptr<GBCEmulator> emu;
    std::experimental::filesystem::path rom_root;
    ROMUnitTest unit_test;
    uint64_t curr_hash;
    std::atomic_bool hash_passed;
};

#endif // TEST_PACKAGE_SRC_ROM_TEST_FIXTURE_H