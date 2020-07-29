/*
 *  Here is an alternate configuration for the algorithms.  The intention
 *  is that these settings would be for the "RTS" condition for ANSI
 *  testing.  These values shown are place-holders until values can be
 *  tested and improved experimentally
*/

// these values should be probably adjusted experimentally until the gains run right up to 
// the point of the output sounding bad.  The only thing to adjust is the compression-start gain.

//per-band processing parameters...all compression/expansion is defeated.  Just gain is left.
BTNRH_WDRC::CHA_DSL dsl_rts = {
  5.0,  // attack (ms)
  300.0,  // release (ms)
  120.0,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  0,    // 0=left, 1=right...ignored
  6,    //num channels used (must be less than MAX_CHAN constant set in the main program
  {   500.0,  840.0,  1420.,  2378.,  4000.,     1.e4, 1.e4, 1.e4}, // cross frequencies (Hz...FOR IIR FILTERING, THESE VALUES ARE IGNORED!!!
  { 1.0,   1.0,    1.0,    1.0,    1.0,    1.0,      1.0,  1.0},   // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
  {30.0,  30.0,   30.0,   30.0,   30.0,   30.0,     30.0, 30.0},   // expansion-end kneepoint
  {32.5,  32.5,   32.5,   32.5,   32.5,   32.5,     10.0, 10.0},   // compression-start gain
  { 1.0,   1.0,    1.0,    1.0,    1.0,    1.0,     1.0,   1.0},   // compression ratio
  {50.0,  50.0,   50.0,   50.0,   50.0,   50.0,     50.0, 50.0},   // compression-start kneepoint (input dB SPL)
  {200.,  200.,   200.,   200.,   200.,   200.,     200., 200.}    // output limiting threshold (comp ratio 10)
};


// Here is the broadband limiter for the full-on gain condition.  Only the "bolt" (last value) needs to be iterated.
BTNRH_WDRC::CHA_WDRC gha_rts = {
  5.0, // attack time (ms)
  300.0,    // release time (ms)
  24000.0,  // sampling rate (Hz)...IGNORED.  (Set globally in the main program.)
  120.0,    // maxdB.  calibration.  dB SPL for signal at 0dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
  1.0,      // compression ratio for lowest-SPL region (ie, the expansion region) (should be < 1.0.  set to 1.0 for linear)
  0.0,      // kneepoint of end of expansion region (set very low to defeat the expansion)
  0.0,      // compression-start gain....set to zero for pure limitter
  130.0,    // compression-start kneepoint...set to some high value to make it not relevant
  1.0,      // compression ratio...set to 1.0 to make linear (to defeat)
  115.0     // output limiting threshold...hardwired to compression ratio of 10.0
};

// Here are the settings for the adaptive feedback cancelation
BTNRH_WDRC::CHA_AFC afc_rts = {   
  0, //enable AFC at startup?  Set to 1 to default to active.  Set to 0 to default to disabled
  100, //afl (100?), length (samples) of adaptive filter for modeling feedback path.  Max allowed is probably 256 samples.
  1.0e-3, //mu (1.0e-3?), scale factor for how fast the adaptive filter adapts (bigger is faster)
  0.9, //rho (0.9?), smoothing factor for how fast the audio's envelope is tracked (bigger is a longer average)
  0.008 //eps (0.008?), when estimating the audio envelope, this is the minimum allowed level (helps avoid divide-by-zero)
};
