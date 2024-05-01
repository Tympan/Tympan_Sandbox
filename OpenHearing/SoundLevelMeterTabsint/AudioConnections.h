#ifndef AudioConnections_h
#define AudioConnections_h

//instantiate the audio objects
AudioInputI2S_F32               i2s_in(audio_settings);            //Digital audio in *from* the Teensy Audio Board ADC.
AudioFilterFreqWeighting_F32    freqWeight1(audio_settings);       //A-weighting filter (optionally C-weighting) for broadband signal
AudioCalcLevel_F32              calcLevel1(audio_settings);        //use this to assess the loudness in the broadband signal
AudioFilterbankBiquad_F32       filterbank(audio_settings);        //this is a parallel bank of IIR (biquad) filters
AudioCalcLevel_F32              calcLevelPerOct[N_FILTER_BANDS];  //we'll use this to assess the loudness in each band
AudioOutputI2S_F32              i2s_out(audio_settings);           //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections for the broadband processing
AudioConnection_F32       patchCord1(i2s_in, 0, freqWeight1, 0);      //connect the Left input to frequency weighting
AudioConnection_F32       patchCord2(freqWeight1, 0, calcLevel1, 0);  //connect the frqeuency weighting to the level time weighting
AudioConnection_F32       patchCord3(i2s_in, 0, i2s_out, 0);      //echo the original signal to the left output
AudioConnection_F32       patchCord4(calcLevel1, 0, i2s_out, 1);     //connect level to the right output

// make all of the audio connections for the per-band processing...call this from setup()
#define N_MAX_CONNECTIONS 100  //some large number greater than the number of connections that we'll ever make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
void makePerBandAudioConnections(void) {
  int count=0;

  //connect to the filterbank
  patchCord[count++] = new AudioConnection_F32(i2s_in,0,filterbank,0);
  
  //send initializing data to the level calcluators that wasn't possible due to creating an array
  for (int i=0; i < N_FILTER_BANDS; i++) calcLevelPerOct[i].setSampleRate_Hz(sample_rate_Hz); //it only wants to know samplerate; it doesn't care about block size

  //connect from the filterbank to the level calcluators
  for (int i=0; i< N_FILTER_BANDS; i++) patchCord[count++] = new AudioConnection_F32(filterbank,i,calcLevelPerOct[i],0);  
}


// configure the processing for the octave-band filters
void configOctaveBandProcessing(const int n_filter_bands, const int filter_order, const int time_const) {
    //choose the filter center frequencies...this line will give a compilation error if n_filter_bands isn't right.  good.
    float center_Hz[n_filter_bands-2] = {125., 250., 500., 1000., 2000., 4000., 8000.};  

    //To setup the filters, we actually need the crossover frequencies, not the center frequencies.
    //So, assuming that they are octave band filters, the crossovers should be related by sqrt(2)
    float crossover_Hz[n_filter_bands];
    for (int i=0; i<n_filter_bands-2; i++) crossover_Hz[i] = center_Hz[i]/sqrtf(2.0);
    crossover_Hz[n_filter_bands-1] = center_Hz[n_filter_bands-1]*sqrtf(2.0); //set the crossover for the last filter
    
    //design the filters!
    int ret_val = filterbank.designFilters(n_filter_bands, filter_order, sample_rate_Hz, audio_block_samples, crossover_Hz);
    if (ret_val < 0) Serial.println("configOctaveBandProcessing: *** ERROR ***: the filters failed to be created.  Why??");

    //set the time constant on the level meters
    //setOctaveBandTimeConstants(n_filter_bands, time_const); //let's not do it here.  Instead, let's do this when we also do the broadband level so that we're doing them all at once
}

#endif
