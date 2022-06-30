
//Note: This file will be overwritten by researchers at BoysTown by using Daniel's controller app
 
#include <AudioEffectCompWDRC_F32.h>  //WEA Nov 2021: This isn't needed
 
 // Per-band prescription for the multi-band processing
static BTNRH_WDRC::CHA_DSL2 dsl = {
  5,  // attack (ms)
  50,  // release (ms)
  119,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // 0=left, 1=right...ignored
  8,    //num channels...ignored.  8 is always assumed
  {317.1666, 502.9734, 797.6319, 1264.9, 2005.9, 3181.1, 5044.7},   // cross frequencies (Hz)
  {0.57, 0.57, 0.57, 0.57, 0.57, 0.57, 0.57, 0.57},   // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {-50.0, -50.0, -50.0, -50.0, -50.0, -50.0, -50.0, -50.0},   // expansion-end kneepoint
  {-13.5942, -16.5909, -3.7978, 6.6176, 11.3050, 23.7183, 25.0, 25.0},   // compression-start gain
  {0.7, 0.9, 1, 1.1, 1.2, 1.4, 1.6, 1.7},   // compression ratio
  {32.2, 26.5, 26.7, 26.7, 29.8, 33.6, 34.3, 32.7},   // compression-start kneepoint (input dB SPL)
  {78.7667, 88.2, 90.7, 92.8333, 98.2, 103.3, 101.9, 99.8},   // broadband output limiting threshold (comp ratio 10)
};

// Used for broad-band limiter.
BTNRH_WDRC::CHA_WDRC2 gha = {
  1.0f,  // attack time (ms)
  50.0f,  // release time (ms)
  24000.f,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  119.0f,  // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.f,      // compression-start gain....set to zero for pure limitter
  105.f,    // compression-start kneepoint...set to some high value to make it not relevant
  10.f,      // compression ratio...set to 1.0 to make linear (to defeat)
  105.0     // broadband output limiting threshold...hardwired to compression ratio of 10.0
};
 
// Settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc = {   
  1,  //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled   !!!! IGNORED BY THIS CHAPRO VERSION
  42,  //afl, length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  0.004607254,  //mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.0072189585,  //rho, smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.000919300,  //eps, when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};
