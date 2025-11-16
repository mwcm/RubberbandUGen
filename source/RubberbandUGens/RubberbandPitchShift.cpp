#include "SC_PlugIn.hpp"
#include "rubberband/RubberBandStretcher.h"

// Simple ring buffer class
template <typename T>
class RingBuffer {
private:
    T *m_buffer;
    int m_size;
    int m_read;
    int m_write;

public:
    RingBuffer(int size) : m_size(size), m_read(0), m_write(0) {
        m_buffer = new T[size];
        for (int i = 0; i < size; i++) {
            m_buffer[i] = T(0);
        }
    }
    
    ~RingBuffer() {
        delete[] m_buffer;
    }
    
    void reset() {
        m_read = 0;
        m_write = 0;
    }
    
    int getReadSpace() const {
        int w = m_write;
        int r = m_read;
        if (w >= r) return w - r;
        return (w + m_size) - r;
    }
    
    int getWriteSpace() const {
        int w = m_write;
        int r = m_read;
        if (w >= r) return (m_size - (w - r)) - 1;
        return r - w - 1;
    }
    
    int read(T *dest, int n) {
        int available = getReadSpace();
        if (n > available) n = available;
        
        for (int i = 0; i < n; i++) {
            dest[i] = m_buffer[m_read];
            m_read = (m_read + 1) % m_size;
        }
        return n;
    }
    
    int write(const T *src, int n) {
        int space = getWriteSpace();
        if (n > space) n = space;
        
        for (int i = 0; i < n; i++) {
            m_buffer[m_write] = src[i];
            m_write = (m_write + 1) % m_size;
        }
        return n;
    }
};

static InterfaceTable *ft;

struct RubberbandPitchShift : public SCUnit {
public:
    RubberbandPitchShift();
    ~RubberbandPitchShift();

private:
    void next(int nSamples);

    RubberBand::RubberBandStretcher* stretcher;
    float** inputBuffers;
    float** outputBuffers;
    RingBuffer<float>** ringBuffers;
    float** scratchBuffers;
    float currentPitch;
    int numChannels;
    int reserve;
    int bufferSize;
};

RubberbandPitchShift::RubberbandPitchShift() {
    numChannels = 2;
    currentPitch = 1.0f;
    reserve = 2048;
    
    stretcher = new RubberBand::RubberBandStretcher(
        sampleRate(),
        numChannels,
        RubberBand::RubberBandStretcher::OptionProcessRealTime |
        RubberBand::RubberBandStretcher::OptionPitchHighSpeed |  // Faster, less latency
        RubberBand::RubberBandStretcher::OptionWindowShort |
        RubberBand::RubberBandStretcher::OptionTransientsCrisp  // 
    );
    
    stretcher->setTimeRatio(1.0);
    
    bufferSize = 1024 + reserve + 8192;
    
    inputBuffers = new float*[numChannels];
    outputBuffers = new float*[numChannels];
    ringBuffers = new RingBuffer<float>*[numChannels];
    scratchBuffers = new float*[numChannels];
    
    for (int i = 0; i < numChannels; i++) {
        inputBuffers[i] = nullptr;
        outputBuffers[i] = nullptr;
        ringBuffers[i] = new RingBuffer<float>(bufferSize);
        scratchBuffers[i] = new float[bufferSize];
        memset(scratchBuffers[i], 0, bufferSize * sizeof(float));
    }
    
    // Prime stretcher and pre-fill ring buffers to eliminate startup artifacts
    stretcher->process(scratchBuffers, reserve, false);
    
    int available = stretcher->available();
    if (available > 0) {
        for (int i = 0; i < numChannels; i++) {
            outputBuffers[i] = scratchBuffers[i];
        }
        stretcher->retrieve(outputBuffers, available);
        
        for (int i = 0; i < numChannels; i++) {
            ringBuffers[i]->write(scratchBuffers[i], available);
        }
    }
    
    mCalcFunc = make_calc_function<RubberbandPitchShift, &RubberbandPitchShift::next>();
    next(1);
}

RubberbandPitchShift::~RubberbandPitchShift() {
    delete stretcher;
    for (int i = 0; i < numChannels; i++) {
        delete ringBuffers[i];
        delete[] scratchBuffers[i];
    }
    delete[] ringBuffers;
    delete[] scratchBuffers;
    delete[] inputBuffers;
    delete[] outputBuffers;
}

void RubberbandPitchShift::next(int nSamples) {
    const float* in0 = in(0);
    const float* in1 = in(1);
    float pitchRatio = in(2)[0];
    float* out0 = out(0);
    float* out1 = out(1);
    
    if (pitchRatio != currentPitch) {
        stretcher->setPitchScale(pitchRatio);
        currentPitch = pitchRatio;
    }
    
    inputBuffers[0] = const_cast<float*>(in0);
    inputBuffers[1] = const_cast<float*>(in1);
    
    // Process input
    stretcher->process(inputBuffers, nSamples, false);
    
    // Retrieve available output and store in ring buffer
    int available = stretcher->available();
    if (available > 0) {
        int toRetrieve = (available > bufferSize) ? bufferSize : available;
        for (int i = 0; i < numChannels; i++) {
            outputBuffers[i] = scratchBuffers[i];
        }
        stretcher->retrieve(outputBuffers, toRetrieve);
        
        for (int i = 0; i < numChannels; i++) {
            ringBuffers[i]->write(scratchBuffers[i], toRetrieve);
        }
    }
    
    // Read from ring buffer to output
    int readSpace = ringBuffers[0]->getReadSpace();
    
    if (readSpace >= nSamples) {
        ringBuffers[0]->read(out0, nSamples);
        ringBuffers[1]->read(out1, nSamples);
    } else {
        // Underrun - read what we have and zero the rest
        if (readSpace > 0) {
            ringBuffers[0]->read(out0, readSpace);
            ringBuffers[1]->read(out1, readSpace);
        }
        for (int i = readSpace; i < nSamples; i++) {
            out0[i] = 0.f;
            out1[i] = 0.f;
        }
    }
}

PluginLoad(RubberbandUGens) {
    ft = inTable;
    registerUnit<RubberbandPitchShift>(ft, "RubberbandPitchShift", false);
}
