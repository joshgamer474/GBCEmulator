//
// Created by childers on 9/7/19.
//

#ifndef ANDROID_PROJECT_BGCOLORS_H
#define ANDROID_PROJECT_BGCOLORS_H

enum BGColor : int {
    BLACK,
    WHITE,
    LIGHT_GRAY,
    ATOMIC_PURPLE,
    DRK_PURPLE,
    PIKA_YELLOW,
    TEAL,
    LIME_GREEN,
    LAST_COLOR
};

struct RGB {
    const uint8_t r, g, b;
};

inline
RGB GetColor(const BGColor color)
{
    switch(color)
    {
        case BGColor::BLACK:        return RGB{0,0,0};
        case BGColor::WHITE:        return RGB{255,255,255};
        case BGColor::LIGHT_GRAY:   return RGB{194,191,192};
        case BGColor::DRK_PURPLE:   return RGB{44,38,85};
        case BGColor::ATOMIC_PURPLE:return RGB{181,164,200};
        case BGColor::PIKA_YELLOW:  return RGB{249,186,46};
        case BGColor::TEAL:         return RGB{8,127,151};
        case BGColor::LIME_GREEN:   return RGB{151,205,57};
        default: return RGB{0,0,0};
    }
}

#endif //ANDROID_PROJECT_BGCOLORS_H
