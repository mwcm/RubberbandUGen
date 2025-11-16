RubberbandPitchShift : MultiOutUGen {
    *ar { |in1, in2, pitchRatio = 1.0|
        ^this.multiNew('audio', in1, in2, pitchRatio);
    }
    
    // Convenience method for mono input
    *arMono { |input, pitchRatio = 1.0|
        ^this.multiNew('audio', input, input, pitchRatio);
    }
    
    init { arg ... theInputs;
        inputs = theInputs;
        ^this.initOutputs(2, rate);
    }
}
