#ifndef PITCH_ANALYZER_H
#define PITCH_ANALYZER_H

#include "Config.h"
#include <fftw3.h>
#include <vector>

class PitchAnalyzer {
private:
    double *in;
    fftw_complex *out;
    fftw_plan plan;
    std::vector<NoteDef> dictionary;

    void initDictionary();

public:
    PitchAnalyzer();
    ~PitchAnalyzer();
    NoteDef analyze(const double* input_buffer, double& out_peak_freq);
    NoteDef identifyNote(double freq);
};

#endif
