#ifndef TEST_PACKAGE_SRC_UNIT_TESTS_H
#define TEST_PACKAGE_SRC_UNIT_TESTS_H

#include <filesystem>
#include <unordered_map>
#include <string>

enum class TestType {
    DEFAULT,
    GPU,
    APU,
    CPU,
    MEMORY,
};


struct ROMUnitTest {
    std::filesystem::path rom_path;
    uint64_t passing_frame_hash = 0;
    TestType test_type = TestType::DEFAULT;

    ROMUnitTest(const std::filesystem::path & path, const uint64_t & hash,
        const TestType & type = TestType::DEFAULT)
        : rom_path(path)
        , passing_frame_hash(hash)
        , test_type(type)
    {
    }
};

namespace blargg {

    enum class cgb_sound {
        _01_registers,
        _02_len_ctr,
        _03_trigger,
        _04_sweep,
        _05_sweep_details,
        _06_overflow_on_trigger,
        _07_len_sweep_period_sync,
        _08_len_ctr_during_power,
        _09_wave_read_while_on,
        _10_wave_trigger_while_on,
        _11_regs_after_power,
        _12_wave,
        group
    };

    enum class cpu_instrs {
        _01_special,
        _02_interrupts,
        _03_op_sp_hl,
        _04_op_r_imm,
        _05_op_rp,
        _06_ld_r_r,
        _07_jr_jp_call_ret_rst,
        _08_misc_instrs,
        _09_op_r_r,
        _10_bit_ops,
        _11_op_a_hl,
        group
    };

    enum class dmg_sound {
        _01_registers,
        _02_len_ctr,
        _03_trigger,
        _04_sweep,
        _05_sweep_details,
        _06_overflow_on_trigger,
        _07_len_sweep_period_sync,
        _08_len_ctr_during_power,
        _09_wave_read_while_on,
        _10_wave_trigger_while_on,
        _11_regs_after_power,
        _12_wave_write_while_on,
        group
    };

    enum class instr_timing {
        group
    };

    enum class interrupt_time {
        group
    };

    enum class mem_timing {
        _01_read_timing,
        _02_write_timing,
        _03_modify_timing,
        group
    };

    enum class mem_timing_2 {
        _01_read_timing,
        _02_write_timing,
        _03_modify_timing,
        group
    };

    enum class oam_bug {
        _01_lcd_sync,
        _02_causes,
        _03_non_causes,
        _04_scanline_timing,
        _05_timing_bug,
        _06_timing_no_bug,
        _07_timing_effect,
        _08_instr_effect,
        group
    };

    enum class halt_bug {
        group
    };
}

#endif // TEST_PACKAGE_SRC_UNIT_TESTS_H