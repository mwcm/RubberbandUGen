# RubberbandUGen
SuperCollider UGen wrapper for the Rubberband audio time-stretching and pitch-shifting library


https://github.com/user-attachments/assets/b720fa8f-f9c9-420c-b418-ef7ddd977c16


## Requirements
```
SuperCollider 3.10+
Rubberband library 4.0.0+
CMake 3.5+
C++ compiler with C++11 support
```
## Installation

```
# Clone this repo
git clone https://github.com/yourusername/RubberbandUGen.git

# Build the UGen
cd RubberbandUGen
mkdir build && cd build
cmake ..
make
sudo make install

# copy to your extensions folder
cp RubberbandPitchShift.so /usr/local/lib/SuperCollider/plugins/
cp ../source/RubberbandUGens/RubberbandPitchShift.sc ~/.local/share/SuperCollider/Extensions/
```

## Usage
```
// Basic pitch shifting
{ RubberbandPitchShift.ar(SinOsc.ar(440), SinOsc.ar(440), 0.5) }.play; // one octave down
{ RubberbandPitchShift.ar(SinOsc.ar(440), SinOsc.ar(440), 2.0) }.play; // one octave up

or

{ RubberbandPitchShift.arMono(SinOsc.ar(440), 0.5) }.play; // one octave down
{ RubberbandPitchShift.arMono(SinOsc.ar(440), 2.0) }.play; // one octave up
```

## Orbit Example

In SuperCollider
```
  fork {
        s.sync;
        2.wait;  // Wait for SuperDirt to fully initialize

        ~rbPitchBus = Bus.audio(s, 2);

        SynthDef(\rbPitchShift, {
            arg inBus;
            var sig, shifted, pitch;
            sig = In.ar(inBus, 2);
            pitch = \pitch.kr(1.0);
            shifted = RubberbandPitchShift.ar(sig[0], sig[1], pitch);
            Out.ar(0, shifted);
        }).add;

        s.sync;

        ~rbPitchSynth = Synth(\rbPitchShift, [
            \inBus, ~rbPitchBus.index,
            \pitch, 1.0
        ], s, \addToTail);

        ~rbPitchNodeID = ~rbPitchSynth.nodeID;  // Store the node ID separately

        // Module to update pitch - use the stored node ID
        ~dirt.addModule('rbpitchControl', { |dirtEvent|
            s.sendMsg('/n_set', ~rbPitchNodeID, \pitch, ~rbpitch);
        }, { ~rbpitch.notNil });

        // Route orbit 2 through pitch shifter
        ~dirt.orbits[2].outBus = ~rbPitchBus;

        "Rubberband pitch shifter ready on orbit 2".postln;
    };
```
In Tidal

```
let rbpitch = pF "rbpitch"
    rbpitchst n = rbpitch (2 ** (n / 12))

d1 $ n (scale "ritusen" "0 .. 7") 
   # sound "superpiano"
   # rbpitchst (-3)
   # orbit 2

-- w/ chop
d1 $ rarely (# (orbit 2 # rbpitch (choose [0.5, 2])))
   $ slow 4
   $ chop 16
   $ s "bev"
   # orbit 0
```
