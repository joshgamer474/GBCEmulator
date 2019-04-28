#include <Fixtures/ROMTestFixture.h>
#include <string>
#include <chrono>
#include <future>
#include <GBCEmulator.h>

ROMTestFixture::ROMTestFixture()
    : unit_test("", 0)
{

}

void ROMTestFixture::init()
{
    rom_root = std::experimental::filesystem::current_path()
        .parent_path() / "bin" / "blarggtests";
    hash_passed = false;
}

void ROMTestFixture::test()
{
    ASSERT_FALSE(unit_test.rom_path.empty());
    //ASSERT_TRUE(unit_test.passing_frame_hash);

    std::string romPath = unit_test.rom_path.string();
    emu = std::make_unique<GBCEmulator>(romPath, romPath + ".log");

    emu->setFrameUpdateMethod(std::bind(&ROMTestFixture::frameUpdatedFunction, this, std::placeholders::_1));
    emu->runWithoutSleep = true; // Run the emulator w/o sleeping

    std::atomic_bool jumpOut(false);
    auto future = std::async(std::launch::async, [&]()
        {
            while (!hash_passed)
            {
                emu->runNextInstruction();

                if (jumpOut)
                {
                    break;
                }
            }
        });

    auto futureStatus = future.wait_for(std::chrono::seconds(5));
    switch (futureStatus)
    {
    case std::future_status::timeout:
    {
        jumpOut = true;
        break;
    }
    }

    jumpOut = true;
    future.get();

    ASSERT_TRUE(hash_passed);
}

void ROMTestFixture::frameUpdatedFunction(SDL_Color* frame)
{
    curr_hash = GBCEmulator::calculateFrameHash(frame);

    // Need to have a passing frame hash in order to pass the unit test
    if (unit_test.passing_frame_hash &&
        curr_hash == unit_test.passing_frame_hash)
    {
        hash_passed = true;
    }
    else
    {
        hash_passed = false;
    }
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::cgb_sound test)
{
    switch (test)
    {
    case blargg::cgb_sound::_01_registers:              return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "01-registers.gb", 94802695448064);
    case blargg::cgb_sound::_02_len_ctr:                return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "02-len ctr.gb", 94648144006656);
    case blargg::cgb_sound::_03_trigger:                return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "03-trigger.gb", 0);
    case blargg::cgb_sound::_04_sweep:                  return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "04-sweep.gb", 0);
    case blargg::cgb_sound::_05_sweep_details:          return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "05-sweep details.gb", 0);
    case blargg::cgb_sound::_06_overflow_on_trigger:    return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "06-overflow on trigger.gb", 88436846888448);
    case blargg::cgb_sound::_07_len_sweep_period_sync:  return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "07-len sweep period sync.gb", 93988167581184);
    case blargg::cgb_sound::_08_len_ctr_during_power:   return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "08-len ctr during power.gb", 0);
    case blargg::cgb_sound::_09_wave_read_while_on:     return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "09-wave read while on.gb", 0);
    case blargg::cgb_sound::_10_wave_trigger_while_on:  return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "10-wave trigger while on.gb", 79380967835136);
    case blargg::cgb_sound::_11_regs_after_power:       return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "11-regs after power.gb", 0);
    case blargg::cgb_sound::_12_wave:                   return ROMUnitTest(rom_root / "cgb_sound" / "rom_singles" / "12-wave.gb", 0);
    case blargg::cgb_sound::group:                      return ROMUnitTest(rom_root / "cgb_sound" / "cgb_sound.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::cpu_instrs test)
{
    switch (test)
    {
    case blargg::cpu_instrs::_01_special:               return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "01-special.gb", 94948892757504);
    case blargg::cpu_instrs::_02_interrupts:            return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "02-interrupts.gb", 94706622930432);
    case blargg::cpu_instrs::_03_op_sp_hl:              return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "03-op sp,hl.gb", 94911299163648);
    case blargg::cpu_instrs::_04_op_r_imm:              return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "04-op r,imm.gb", 94923830361600);
    case blargg::cpu_instrs::_05_op_rp:                 return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "05-op rp.gb", 95111798330880);
    case blargg::cpu_instrs::_06_ld_r_r:                return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "06-ld r,r.gb", 95128506594816);
    case blargg::cpu_instrs::_07_jr_jp_call_ret_rst:    return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "07-jr,jp,call,ret,rst.gb", 94389165915648);
    case blargg::cpu_instrs::_08_misc_instrs:           return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "08-misc instrs.gb", 94710799996416);
    case blargg::cpu_instrs::_09_op_r_r:                return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "09-op r,r.gb", 95128506594816);
    case blargg::cpu_instrs::_10_bit_ops:               return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "10-bit ops.gb", 94982309285376);
    case blargg::cpu_instrs::_11_op_a_hl:               return ROMUnitTest(rom_root / "cpu_instrs" / "individual" / "11-op a,(hl).gb", 94911299163648);
    case blargg::cpu_instrs::group:                     return ROMUnitTest(rom_root / "cpu_instrs" / "cpu_instrs.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::dmg_sound test)
{
    switch (test)
    {
    case blargg::dmg_sound::_01_registers:              return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "01-registers.gb", 97478577815040);
    case blargg::dmg_sound::_02_len_ctr:                return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "02-len ctr.gb", 97319664034560);
    case blargg::dmg_sound::_03_trigger:                return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "03-trigger.gb", 0);
    case blargg::dmg_sound::_04_sweep:                  return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "04-sweep.gb", 0);
    case blargg::dmg_sound::_05_sweep_details:          return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "05-sweep details.gb", 0);
    case blargg::dmg_sound::_06_overflow_on_trigger:    return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "06-overflow on trigger.gb", 90933048046080);
    case blargg::dmg_sound::_07_len_sweep_period_sync:  return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "07-len sweep period sync.gb", 96641059242240);
    case blargg::dmg_sound::_08_len_ctr_during_power:   return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "08-len ctr during power.gb", 0);
    case blargg::dmg_sound::_09_wave_read_while_on:     return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "09-wave read while on.gb", 0);
    case blargg::dmg_sound::_10_wave_trigger_while_on:  return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "10-wave trigger while on.gb", 0);
    case blargg::dmg_sound::_11_regs_after_power:       return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "11-regs after power.gb", 97070555946240);
    case blargg::dmg_sound::_12_wave_write_while_on:    return ROMUnitTest(rom_root / "dmg_sound" / "rom_singles" / "12-wave write while on.gb", 0);
    case blargg::dmg_sound::group:                      return ROMUnitTest(rom_root / "dmg_sound" / "dmg_sound.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::instr_timing test)
{
    switch (test)
    {
    case blargg::instr_timing::group: return ROMUnitTest(rom_root / "instr_timing" / "instr_timing.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::interrupt_time test)
{
    switch (test)
    {
    case blargg::interrupt_time::group: return ROMUnitTest(rom_root / "interrupt_time" / "interrupt_time.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::mem_timing test)
{
    switch (test)
    {
    case blargg::mem_timing::_01_read_timing:   return ROMUnitTest(rom_root / "mem_timing" / "individual" / "01-read_timing.gb", 0);
    case blargg::mem_timing::_02_write_timing:  return ROMUnitTest(rom_root / "mem_timing" / "individual" / "02-write_timing.gb", 0);
    case blargg::mem_timing::_03_modify_timing: return ROMUnitTest(rom_root / "mem_timing" / "individual" / "03-modify_timing.gb", 0);
    case blargg::mem_timing::group:             return ROMUnitTest(rom_root / "mem_timing" / "mem_timing.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::mem_timing_2 test)
{
    switch (test)
    {
    case blargg::mem_timing_2::_01_read_timing:   return ROMUnitTest(rom_root / "mem_timing-2" / "rom_singles" / "01-read_timing.gb", 0);
    case blargg::mem_timing_2::_02_write_timing:  return ROMUnitTest(rom_root / "mem_timing-2" / "rom_singles" / "02-write_timing.gb", 0);
    case blargg::mem_timing_2::_03_modify_timing: return ROMUnitTest(rom_root / "mem_timing-2" / "rom_singles" / "03-modify_timing.gb", 0);
    case blargg::mem_timing_2::group:             return ROMUnitTest(rom_root / "mem_timing-2" / "mem_timing.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::oam_bug test)
{
    switch (test)
    {
    case blargg::oam_bug::_01_lcd_sync:         return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "1-lcd_sync.gb", 0);
    case blargg::oam_bug::_02_causes:           return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "2-causes.gb", 0);
    case blargg::oam_bug::_03_non_causes:       return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "3-non_causes.gb", 94844466107904);
    case blargg::oam_bug::_04_scanline_timing:  return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "4-scanline_timing.gb", 0);
    case blargg::oam_bug::_05_timing_bug:       return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "5-timing_bug.gb", 0);
    case blargg::oam_bug::_06_timing_no_bug:    return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "6-timing_no_bug.gb", 90575504672256);
    case blargg::oam_bug::_07_timing_effect:    return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "7-timing_effect.gb", 0);
    case blargg::oam_bug::_08_instr_effect:     return ROMUnitTest(rom_root / "oam_bug" / "rom_singles" / "8-instr_effect.gb", 0);
    case blargg::oam_bug::group:                return ROMUnitTest(rom_root / "oam_bug" / "oam_bug.gb", 0);
    }
    return ROMUnitTest("", 0);
}

ROMUnitTest ROMTestFixture::getUnitTest(blargg::halt_bug test)
{
    switch (test)
    {
    case blargg::halt_bug::group:   return ROMUnitTest(rom_root / "halt_bug.gb", 0);
    }
    return ROMUnitTest("", 0);
}