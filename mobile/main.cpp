//#include <jni.h>
#include <SDL_main.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <GBCEmulator.h>
#include <SDLWindowAndroid.h>
#include <JoypadXInput.h>
#include <spdlog/spdlog.h>
#include <atomic>
#include <memory>
#include <string>
#include <experimental/filesystem>
#include <sstream>

std::unique_ptr<SDLWindowAndroid> window;
std::shared_ptr<GBCEmulator> emu;
bool start_emu;
bool run;
std::atomic_bool keep_main_running = true;
std::atomic_bool sdlwindowandroid_ready = false;

extern "C" {

//#include <jni.h>
#include <memory>

// for string delimiter
std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

int main(int argc, char *argv[])
{
    SDL_Log("Starting main()");
    if (argc < 2) {
        SDL_Log("Didn't give a ROM image...");
        return -1;
    }
    SDL_Log("Gave ROM: %s", argv[1]);
    const std::string rompathStr(argv[1]);
    // Generate SDLWindow log name
    const std::string sdlwindow_logpath = ".SDLWindow.txt";
    const std::string sdl_config = ".SDLWindow.config";
    const std::string rom_logfile = rompathStr + ".log.txt";

    SDL_Log("Passed in filename %s", rompathStr.c_str());
    SDL_Log("Creating SDLWindow");
    //if (!window)
    {
        window.reset(new SDLWindowAndroid(sdl_config
                , sdlwindow_logpath));
    }

    if (!emu && SDLWindowAndroid::romIsValid(rompathStr))
    {
        SDL_Log("ROM was valid: %s", rompathStr.c_str());
        emu = std::make_shared<GBCEmulator>(rompathStr, rom_logfile);
        emu->set_logging_level(spdlog::level::err);
        //emu->logger->set_level(spdlog::level::debug);
        window->hookToEmulator(emu);
        start_emu = true;

        window->startEmulator();
    }
    window->run();

    SDL_Log("main() returning 0");
    keep_main_running = true;
    return 0;
}

/*int main(int argc, char *argv[])
{
    SDL_Log("Starting main()");
    // Wait for SDLWindowAndroid to be initialized
    sdlwindowandroid_ready = false;
    while (!sdlwindowandroid_ready) {
        //SDL_Delay(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    keep_main_running = true;
    while (keep_main_running) {
        //SDL_Delay(1000);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    SDL_Log("main() returning 0");
    keep_main_running = true;
    return 0;
}*/

/*
JNIEXPORT jboolean JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeInit(
        JNIEnv *env,
        jclass type,
        jstring config_path,
        jstring rompath)
{

    const std::string configpathStr = std::string(env->GetStringUTFChars(config_path, NULL));
    const std::string rompathStr = std::string(env->GetStringUTFChars(rompath, NULL));
    start_emu = false;
    run = true;

    // Get filename from filepath
    std::istringstream iss(rompathStr);
    std::vector<std::string> split_v = split(rompathStr, "/");
    const std::string rom_filename = split_v.at(split_v.size() - 1);
    // Generate SDLWindow log name
    const std::string sdlwindow_logpath = configpathStr + "/" + rom_filename + ".SDLWindow.txt";
    const std::string sdl_config = configpathStr + "/" + ".SDLWindow.config";
    const std::string rom_logfile = configpathStr + "/" + rom_filename + ".log.txt";

    SDL_Log("Passed in filename %s", rompathStr.c_str());

    SDL_Log("Creating SDLWindow");
    //if (!window)
    {
        window.reset(new SDLWindowAndroid(sdl_config
                , sdlwindow_logpath));
    }

    if (!emu && SDLWindowAndroid::romIsValid(rompathStr))
    {
        SDL_Log("ROM was valid: %s", rompathStr.c_str());
        emu = std::make_shared<GBCEmulator>(rompathStr, rom_logfile);
        emu->set_logging_level(spdlog::level::err);
        //emu->logger->set_level(spdlog::level::debug);
        window->hookToEmulator(emu);
        start_emu = true;

        window->startEmulator();
    }

    sdlwindowandroid_ready = true;

    SDL_Log("Returning start_emu: %d", start_emu);
    return start_emu;
}

JNIEXPORT jboolean JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeUpdate(
        JNIEnv *env,
        jclass type)
{
    if (!window)
    {
        return false;
    }

    window->update(run);

    return run;
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativePause(
        JNIEnv *env,
        jclass type)
{
    SDL_Log("Doing nativePause()");

    if (window)
    {
        window->pause();
    }
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeFinish(
        JNIEnv *env,
        jclass type)
{
    SDL_Log("Doing nativeFinish()");
    keep_main_running = false;

    if (window)
    {
        window->stopEmulator();
        window->quit = true;
        window.reset();
    }

    if (emu)
    {
        emu.reset();
    }
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeToggleMoveButtons(
        JNIEnv *env,
        jclass clazz,
        jboolean val)
{
    if (window)
    {
        window->toggleMoveButtons(val);
    }
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeToggleResizeButtons(
        JNIEnv *env,
        jclass clazz,
        jboolean val)
{
    if (window)
    {
        window->toggleResizeButtons(val);
    }
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeResetButtons(
        JNIEnv *env,
        jclass clazz)
{
    if (window)
    {
        window->resetButtons();
    }
}

JNIEXPORT void JNICALL
Java_org_childers_gbcemulator_EmulatorActivity_nativeChangeCGBPalette(
        JNIEnv *env,
        jclass clazz)
{
    if (window)
    {
        window->changeCGBPalette();
    }
}*/

} // end extern "C"
