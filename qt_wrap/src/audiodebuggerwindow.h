#ifndef AUDIO_DEBUGGER_WINDOW_H
#define AUDIO_DEBUGGER_WINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QImage>
#include <QLabel>
#include <QMutex>
#include <src/emuview.h>
#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

enum Channel : int {
    SQUARE1 = 0,
    SQUARE2,
    WAVE,
    NOISE,
};

namespace Ui {
    class AudioDebuggerWindow;
}

class APU;

class AudioDebuggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AudioDebuggerWindow(QWidget *parent = 0);
    virtual ~AudioDebuggerWindow();

    void showEvent(QShowEvent * e);
    void hideEvent(QHideEvent * e);
    void initEmulatorConnections(std::shared_ptr<GBCEmulator> emu);

private:
    void initWaveforms(const size_t & num_samples);
    void pushSample(float sample, int channel);
    void updateWaveformImage(const Channel & channel);

    Ui::AudioDebuggerWindow *ui;
    std::shared_ptr<GBCEmulator> emu;
    std::shared_ptr<APU> apu;
    std::vector<QImage> waveformImages;
    std::unordered_map<Channel, std::deque<float>> waveforms;
    std::unordered_map<Channel, QLabel*> waveformLabels;
    QTimer updateGUITimer;
    std::timed_mutex imageMutex;
    bool isHidden;
};

#endif // AUDIO_DEBUGGER_WINDOW_H