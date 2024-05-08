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
    const int n_center_freqs = n_filter_bands - 2;  //see discussion a few lines below
    float center_Hz[n_center_freqs] = {125., 250., 500., 1000., 2000., 4000., 8000.};  

    //Be aware that the filterbank class in the Tympan_Library assumes that you want to handle *ALL* frequencies,
    //from DC up to Nyquist.  Therefore, if you tell the filterbank that you want N filters, it will use those
    //N filters in the following way:
    //   * One lowpass filter spanning from DC up to the first cross-over frequency.
    //   * N-2 bandbass filters, each filter spanning between neighboring cross-over frequencies
    //   * One highpass filter spannding from the last cross-over frequency up to Nyquist

    //Since the point of our filterbank is to have those middle bandpass filters be octave-band filters,
    //we need to specify the cross-over frequencies in-between our center frequencies.  Being one octave
    //in width (ie, a factor of 2.0), the lower crossover frequency is center_freq[i]/sqrt(2) and the upper
    //crossover frequency is center_freq[i]*sqrt(2) [which for octave-spaced center frequencies should be 
    //the same as center_freq[i+1]/sqrt(2)]

    //OK, given all that backaground, now lets compute the cross-over frequencies from the center frequencies
    const int n_crossover_freqs = n_filter_bands - 1;
    float crossover_Hz[n_crossover_freqs]; 
    for (int i=0; i < n_center_freqs; i++) crossover_Hz[i] = center_Hz[i]/sqrtf(2.0); //loop over each center frequency
    crossover_Hz[n_crossover_freqs-1] = center_Hz[n_center_freqs-1]*sqrtf(2.0); //set the last crossover for the last filter by scaling up from the last center frequency

    //print some debugging info...
    Serial.println("configOctaveBandProcessing: n_filter_bands = " + String(n_filter_bands));
    Serial.print("configOctaveBandProcessing: center = "); for (int i=0; i < n_filter_bands-2; i++) {Serial.print(String(center_Hz[i],2) + ", "); }; Serial.println();
    Serial.print("configOctaveBandProcessing: crossovers = "); for (int i=0; i < n_filter_bands-1; i++) {Serial.print(String(crossover_Hz[i],2) + ", "); }; Serial.println();

    //have the filterbank class design the filters!
    int ret_val = filterbank.designFilters(n_filter_bands, filter_order, sample_rate_Hz, audio_block_samples, crossover_Hz);
    if (ret_val < 0) Serial.println("configOctaveBandProcessing: *** ERROR ***: the filters failed to be created.  Why??");

    //set the time constant on the level meters
    //setOctaveBandTimeConstants(n_filter_bands, time_const); //let's not do it here.  Instead, let's do this when we also do the broadband level so that we're doing them all at once
}

#endif
