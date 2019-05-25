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

//#define REMOVE_LOGS 1

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
        {   // Save last frame and frame hash
            auto pngPath = unit_test.rom_path;
            pngPath += "_" + std::to_string(curr_hash) + "_";
            pngPath += "failed.png";
            emu->saveFrameToPNG(pngPath);
        }

        // Destruct emulator
        emu.reset();

#ifdef REMOVE_LOGS
        // Remove .log file
        auto logPath = unit_test.rom_path;
        logPath += ".log";
        tryRemoveFile(logPath);
#endif // REMOVE_LOGS

        // Remove .sav file
        auto savPath = unit_test.rom_path.replace_extension(".sav");
        tryRemoveFile(savPath);
    }

    void init();
    void test();
    void frameUpdatedFunction(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> /* frame */);
    ROMUnitTest getUnitTest(blargg::cgb_sound test) const;
    ROMUnitTest getUnitTest(blargg::cpu_instrs test) const;
    ROMUnitTest getUnitTest(blargg::dmg_sound test) const;
    ROMUnitTest getUnitTest(blargg::instr_timing test) const;
    ROMUnitTest getUnitTest(blargg::interrupt_time test) const;
    ROMUnitTest getUnitTest(blargg::mem_timing test) const;
    ROMUnitTest getUnitTest(blargg::mem_timing_2 test) const;
    ROMUnitTest getUnitTest(blargg::oam_bug test) const;
    ROMUnitTest getUnitTest(blargg::halt_bug test) const;

private:
    void setEmuLogLevels(const TestType& test_type);
    void tryRemoveFile(const std::experimental::filesystem::path& file);

    std::unique_ptr<GBCEmulator> emu;
    std::experimental::filesystem::path rom_root;
    ROMUnitTest unit_test;
    uint64_t curr_hash;
    std::atomic_bool hash_passed;
};

#endif // TEST_PACKAGE_SRC_ROM_TEST_FIXTURE_H