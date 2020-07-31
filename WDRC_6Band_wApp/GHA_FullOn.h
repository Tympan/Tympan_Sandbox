/*
 *  Here is an alternate configuration for the algorithms.  The intention
 *  is that these settings would be for the "full-on gain" condition for ANSI
 *  testing.  These values shown are place-holders until values can be
 *  tested and improved experimentally
*/

// these values should be probably adjusted experimentally until the gains run right up to 
// the point of the output sounding bad.  The only thing to adjust is the compression-start gain.

//per-band processing parameters...all compression/expansion is defeated.  Just gain is left.
BTNRH_WDRC::CHA_DSL dsl_fullon = {
  5.0,  // attack (ms)
  300.0,  // release (ms)
  120.0,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // 0=left, 1=right...ignored
  6,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {500.0, 840.0, 1420.0, 2500.0, 5000.0,             1.e4, 1.e4, 1.e4},// cross frequencies (Hz)
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,      1.0, 1.0},       // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  { 10.0,  10.0,   10.0,   10.0,   10.0,   10.0,     10.0, 10.0},      // expansion-end kneepoint (Input dB SPL) (Note: Disabled by setting the ratio to 1.0)
  { 27.0,  30.0,   40.0,   40.0,   45.0,   48.0,     10.f, 10.f},      // compression-start gain
  {  1.0,   1.0,    1.0,    1.0,    1.0,    1.0,     1.0f, 1.0f},      // compression ratio(Note: Disable by setting the ratio to 1.0)
  { 80.0,  80.0,   80.0,   80.0,   80.0,   80.0,     80.0, 80.0},      // compression-start kneepoint (input dB SPL) 
  {200.0, 200.0,  200.0,  200.0,  200.0,  200.0,    200.0, 200.0}      // output limiting threshold (input dB SPL) (comp ratio 10) (Disabled by setting to high value)
};


// Here is the broadband limiter for the full-on gain condition.  Only the "bolt" (last value) needs to be iterated.
BTNRH_WDRC::CHA_WDRC gha_fullon = {
  5.0,      // attack time (ms)
  300.0,    // release time (ms)
  24000.0,  // sampling rate (Hz)...ignored.  Set globally in the main program.
  130.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  200.0,    // compression-start kneepoint...set to some high value to make it not relevant (Disable by setting knee to high value)
  1.0,      // compression ratio...set to 1.0 to make linear (Bug: Is still enabled when ratio set to 1.0; to disable, set knee high 
  110.0     // output limiting threshold...hardwired to compression ratio of 10.0
};

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc_fullon = {   
  0, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl (100?), length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu (1.0e-3?), scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho (0.9?), smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps (0.008?), when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};
