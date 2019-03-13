#include "AudioDebuggerWindow.h"
#include "ui_AudioDebuggerWindow.h"
#include <string>
#include <GBCEmulator.h>
#include <APU.h>
#include <QMutex>
#include <QThread>
#include <QApplication>

#define SAMPLE_PRECISION 500
#define IMAGE_HEIGHT 50

AudioDebuggerWindow::AudioDebuggerWindow(QWidget *parent)
    :   QMainWindow(parent),
        ui(new Ui::AudioDebuggerWindow),
        isHidden(false)
{
    ui->setupUi(this);

    initWaveforms(SAMPLE_PRECISION);
    this->show();
}

AudioDebuggerWindow::~AudioDebuggerWindow()
{
    emu.reset();

    delete ui;
}

void AudioDebuggerWindow::showEvent(QShowEvent * e)
{
    isHidden = false;

    if (apu)
    {
        apu->sendSamplesToDebugger(false);
    }
    QWidget::showEvent(e);
}

void AudioDebuggerWindow::hideEvent(QHideEvent * e)
{
    isHidden = true;

    if (apu)
    {
        apu->sendSamplesToDebugger(false);
    }
    QWidget::hideEvent(e);
}

void AudioDebuggerWindow::initEmulatorConnections(std::shared_ptr<GBCEmulator> _emu)
{
    emu = _emu;

    if (emu)
    {
        apu = emu->get_APU();
        apu->setSampleUpdateMethod(std::bind(&AudioDebuggerWindow::pushSample, this, std::placeholders::_1, std::placeholders::_2));
        apu->sendSamplesToDebugger(true);
    }

    // Setup updateGUITimer to update GUI when the emulator is running
    connect(&updateGUITimer, &QTimer::timeout, [&]()
    {
        // Prevent updating debug window's GUI when it isn't being shown
        if (isHidden)
        {
            return;
        }

        // Get new image
        updateWaveformImage2(Channel::SQUARE1);
        updateWaveformImage2(Channel::SQUARE2);
        updateWaveformImage2(Channel::WAVE);
        updateWaveformImage2(Channel::NOISE);

        // Push QImage to QLabel
        waveformLabels[Channel::SQUARE1]->setPixmap(QPixmap::fromImage(waveformImages[0]));
        waveformLabels[Channel::SQUARE2]->setPixmap(QPixmap::fromImage(waveformImages[1]));
        waveformLabels[Channel::WAVE]->setPixmap(QPixmap::fromImage(waveformImages[2]));
        waveformLabels[Channel::NOISE]->setPixmap(QPixmap::fromImage(waveformImages[3]));
    });
    updateGUITimer.start(1000 / 60.0);
}

void AudioDebuggerWindow::initWaveforms(const size_t & num_samples)
{
    waveforms[Channel::SQUARE1] = std::deque<float>(num_samples, 0.0f);
    waveforms[Channel::SQUARE2] = std::deque<float>(num_samples, 0.0f);
    waveforms[Channel::WAVE]    = std::deque<float>(num_samples, 0.0f);
    waveforms[Channel::NOISE]   = std::deque<float>(num_samples, 0.0f);

    waveformLabels[Channel::SQUARE1]    = ui->image_Square1;
    waveformLabels[Channel::SQUARE2]    = ui->image_Square2;
    waveformLabels[Channel::WAVE]       = ui->image_Wave;
    waveformLabels[Channel::NOISE]      = ui->image_Noise;

    waveformImages = std::vector<QImage>(4, QImage(SAMPLE_PRECISION, IMAGE_HEIGHT, QImage::Format::Format_RGB888));
    for (auto & image : waveformImages)
    {
        image.fill(Qt::black);
    }
}

void AudioDebuggerWindow::pushSample(float sample, int channel)
{
    if (imageMutex.try_lock())
    {
        // Get waveform deque
        auto & waveform = waveforms[static_cast<Channel>(channel)];

        if (waveform.size() >= SAMPLE_PRECISION)
        {   // Pop front of waveform off
            waveform.pop_front();
        }

        waveform.push_back(sample);

        imageMutex.unlock();
    }
}

void AudioDebuggerWindow::updateWaveformImage(const Channel & channel)
{
    std::unique_lock<std::timed_mutex> lock(imageMutex, std::defer_lock);
    if (lock.try_lock_for(std::chrono::milliseconds(1)))
    {
        // Get waveform deque
        const auto & waveform = waveforms[channel];

        // Get waveform's QImage
        auto & image = waveformImages[channel];

        // Clear the image
        image.fill(Qt::black);

        // Draw waveform onto blank image
        int y = 0;
        for (int x = 0; x < SAMPLE_PRECISION; x++)
        {   // Normalize sample (get y coordinate)
            y = ((waveform[x] + 1.0f) * IMAGE_HEIGHT) / 2.0;

            // Get the current scanline
            uchar * scanline = image.scanLine(y);

            // Color the pixel
            size_t offset = x * 3;      // RGB888
            scanline[offset] = 0;       // R
            scanline[offset + 1] = 0;   // G
            scanline[offset + 2] = 255; // B
        }

        imageMutex.unlock();
    }
}

void AudioDebuggerWindow::updateWaveformImage2(const Channel & channel)
{
    std::unique_lock<std::timed_mutex> lock(imageMutex, std::defer_lock);
    if (lock.try_lock_for(std::chrono::milliseconds(1)))
    {
        // Get waveform deque
        const auto & waveform = waveforms[channel];

        // Get waveform's QImage
        auto & image = waveformImages[channel];
        const int imageWidth = image.width();
        const int imagePixelWidth = imageWidth * 3; // RGB888: 1 pixel = 3 bytes

        // Move image 1 pixel to the left
        for (int y = 0; y < image.height(); y++)
        {   // Update each scanline to move pixel data 1 pixel to the left
            uchar * scanline = image.scanLine(y);
            
            for (int x = 0; x < imagePixelWidth - 3; x += 3)
            {   // An attempt to optimize
                if (scanline[x + 1] != 0)   // G
                {   // Blacken this pixel
                    scanline[x + 1] = 0;
                }

                // Get which scanline (y) the latest waveform is
                int waveformY;
                if (waveform[x / 3] == 0)
                {
                    waveformY = IMAGE_HEIGHT >> 1;  // divided by 2
                }
                else
                {
                    waveformY = waveform[x / 3] * IMAGE_HEIGHT;
                }
                //const int waveformY = ((waveform[x / 3] + 1.0f) * IMAGE_HEIGHT) / 2.0;
                //const int waveformY = waveform[x / 3] * IMAGE_HEIGHT;

                if (y == waveformY)
                {   // Color this pixel green
                    scanline[x + 1] = 255;     // G
                }
            }
        }

        imageMutex.unlock();
    }
}