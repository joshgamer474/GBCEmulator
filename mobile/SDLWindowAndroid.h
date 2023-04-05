//
// Created by childers on 6/9/19.
//

#ifndef ANDROID_PROJECT_SDLWINDOWANDROID_H
#define ANDROID_PROJECT_SDLWINDOWANDROID_H

#define SDL_MAIN_HANDLED

#include <ScreenInterface.h>
#include <GBCEmulator.h>
#include <JoypadXInput.h>
#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>
#include <ScreenInterface.h>
#include <bgcolors.h>

extern "C" {
#include <SDL.h>
#include <SDL2/SDL_image.h>
}

enum SDLRenderType : char {
    NEAREST_PIXEL = 0,
    LINEAR_FILTERING,
    ANISOTROPIC_FILTERING
};

enum class GAMEPAD_BUTTON : uint8_t {
    UP      = 0b00000001,
    DOWN    = 0b00000010,
    LEFT    = 0b00000100,
    RIGHT   = 0b00001000,
    START   = 0b00010000,
    SELECT  = 0b00100000,
    A       = 0b01000000,
    B       = 0b10000000,
};

struct SDLTextureRect {
    std::string name;
    SDL_Texture* texture;
    SDL_Rect rect;
    Joypad::BUTTON joypad_button = Joypad::BUTTON::NONE;
};

struct SDL_DPAD {
    SDL_Rect orig_rect;
    SDL_Rect bounding_rect;

    void initialize(SDL_Rect& _rect)
    {
        orig_rect = _rect;
        bounding_rect.h = _rect.h * 2;
        bounding_rect.w = _rect.w * 2;
        bounding_rect.x = _rect.x - (_rect.h / 2);
        bounding_rect.y = _rect.y - (_rect.w / 2);
    }

    SDL_Rect move(int x, int y, SDL_Rect& curr_rect)
    {
        SDL_Rect ret;
        ret.w = curr_rect.w;
        ret.h = curr_rect.h;

        int halfWidth = curr_rect.w / 2;
        int halfHeight = curr_rect.h / 2;

        // Perform X border check
        if (x < bounding_rect.x + halfWidth)
        {   // Hit left border
            ret.x = bounding_rect.x;
        }
        else if (x > bounding_rect.x + bounding_rect.w - halfWidth)
        {   // Hit right border
            ret.x = bounding_rect.x + bounding_rect.w - curr_rect.w;
        }
        else
        {   // Move regularly in X (keep centered)
            ret.x = x - halfWidth;
        }

        // Perform Y border check
        if (y < bounding_rect.y + halfHeight)
        {   // Hit top border
            ret.y = bounding_rect.y;
        }
        else if (y > bounding_rect.y + bounding_rect.h - halfHeight)
        {   // Hit bottom border
            ret.y = bounding_rect.y + bounding_rect.h - curr_rect.h;
        }
        else
        {
            ret.y = y - halfHeight;
        }

        return ret;
    }

    uint8_t getDirection(SDL_Rect& curr_rect)
    {
        // Compare current rect's [x, y] to
        // orig_rect's [x, y]
        int xDiff = orig_rect.x - curr_rect.x;
        int yDiff = orig_rect.y - curr_rect.y;

        int ret = 0;
        int halfWidth = curr_rect.w / 2;
        int halfHeight = curr_rect.h / 2;
        int pixelThreashold = halfWidth * 0.75;

        std::string log;

        if (xDiff > pixelThreashold)
        {
            ret |= (uint8_t)GAMEPAD_BUTTON::LEFT;
            log += "LEFT, ";
        }
        else if (xDiff < -pixelThreashold)
        {
            ret |= (uint8_t)GAMEPAD_BUTTON::RIGHT;
            log += "RIGHT, ";
        }

        if (yDiff > pixelThreashold)
        {
            ret |= (uint8_t)GAMEPAD_BUTTON::UP;
            log += "UP, ";
        }
        else if (yDiff < -pixelThreashold)
        {
            ret |= (uint8_t)GAMEPAD_BUTTON::DOWN;
            log += "DOWN, ";
        }

        if (log.size())
        {
            //SDL_Log("DPAD Direction: %s", log.c_str());
        }

        return ret;
    }
};

class SDLWindowAndroid : public ScreenInterface
{
public:
    SDLWindowAndroid(const std::string& config_path,
            const std::string& log_name = "SDLWindowAndroid.log",
            const BGColor background_color = BGColor::BLACK);
    virtual ~SDLWindowAndroid();

    void cleanupSDL();
    void display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame);
    void hookToEmulator(std::shared_ptr<GBCEmulator> emulator);
    static bool romIsValid(const std::string & filepath);
    static std::string getFileExtension(const std::string & filepath);
    int run(bool start_emu = false);
    void setBGColor(const BGColor color);

    void startEmulator();
    void handleSDLEvent(SDL_Event& event, bool& run);
    void update(bool& run);
    void pause();
    void resume();
    void stopEmulator();

    // Toggle-able Android functions
    void toggleMoveButtons(const bool val);
    void toggleResizeButtons(const bool val);
    void resetButtons();
    void changeCGBPalette();

    bool quit;
    bool pressedDPAD;
    bool backIsPressed;
    int backPressed;
    int drawableWidth, drawableHeight;
    int windowWidth, windowHeight;
    size_t numEventsPerFrame;
    std::vector<SDLTextureRect*> buttonTextures;

private:
    void init();
    void initGamepadTextures();
    void resetTextureSize(SDLTextureRect& texture_rect);
    void updateWindowTitle(const std::string & framerate);
    void takeSaveState();
    void loadSaveState();
    bool loadImageIntoTextureRect(const std::string& img_path, SDLTextureRect& texture_rect);
    bool intersects(const int x, const int y, const SDLTextureRect& texture_rect) const;
    bool intersects(const int x, const int y, const SDL_Rect& rect) const;
    void presentFrame(const std::vector<SDLTextureRect*>& button_textures);
    void clearScreenDoubleBuffer();
    void drawGrid(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);
    void drawLinesThroughButtons(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);

    void saveSDLTextureRects();
    void loadSDLTextureRects();
    std::vector<std::string> splitString(std::string in, const std::string& delimiter);

    std::shared_ptr<GBCEmulator> emu;
    std::shared_ptr<GBCEmulator> emu_savestate;
    std::shared_ptr<Joypad> joypad;
    std::shared_ptr<JoypadXInput> joypadx;
    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<std::thread> emu_thread;
    std::mutex renderer_mutex;
    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> curr_frame;
    std::chrono::duration<double> second_start_clock;
    std::map<SDL_FingerID, SDLTextureRect*> fingerTouchMap;
    std::map<int32_t, SDL_Joystick*> joystickMap;

    SDL_DisplayMode current_display_mode;
    SDL_GLContext glContext;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Renderer* renderer;
    SDL_Joystick* sdl_joystick;
    SDLTextureRect screen_texture_rect;
    SDLTextureRect gamepad_texture_rect;
    SDLTextureRect texture_button_a;
    SDLTextureRect texture_button_b;
    SDLTextureRect texture_button_dpad;
    SDLTextureRect texture_button_dpad_border;
    SDLTextureRect texture_button_start;
    SDLTextureRect texture_button_select;
    SDLTextureRect texture_button_bgcolor_change;
    SDLTextureRect texture_button_dmgpalette_change;
    SDL_Event prevEvent;
    SDL_DPAD dpad;
    BGColor bg_color;
    uint16_t fps;
    uint8_t num_joypads_looked_at;
    std::string config_path;
    bool keep_aspect_ratio;
    bool initialized_SDL;
    bool new_frame;
    bool move_buttons;
    bool resize_buttons;
    bool is_android_tv;
    std::atomic_bool screen_orientation_changed;
};

#endif //ANDROID_PROJECT_SDLWINDOWANDROID_H
