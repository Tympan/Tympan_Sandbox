 
// This code converts from Daniels use of "CHA_DSL2" and "CHA_WDRC2" into the data structures needed
// by Steve's chapro.h version of CHA_DSL and CHA_WDRC

#ifndef _translator_h
#define _translator_h

#include <chapro.h> //to get the definition of the destination types  CHA_DSL and CHA_WDRC
#include <BTNRH_WDRC_Types.h>  //from Tympan_Library for the definition of BTNRH_WDRC::AFC (and maybe for DSL_MXH??)

// First, we define the CHA_DSL2 and CHA_WDRC2 stuctures that don't already exist somewhere else
namespace BTNRH_WDRC {
  
  struct CHA_DSL2 {
      double attack;               // attack time (ms)
      double release;              // release time (ms)
      double maxdB;                // maximum signal (dB SPL)
      int32_t ear;                 // 0=left, 1=right
      int32_t nchannel;            // number of channels
      double cross_freq[DSL_MXCH]; // cross frequencies (Hz)
      double exp_cr[DSL_MXCH];     // expansion-regime compression ratio
      double exp_tk[DSL_MXCH];      // expansion-regime end kneepoint
      double tkgain[DSL_MXCH];     // compression-start gain
      double cr[DSL_MXCH];         // compression ratio
      double tk[DSL_MXCH];         // compression-start kneepoint
      double bolt[DSL_MXCH];       // broadband output limiting threshold
  };
  
  struct CHA_WDRC2 {
      double attack;          // attack time (ms)
      double release;         // release time (ms)
      double fs;              // sampling rate (Hz)
      double maxdB;           // maximum signal (dB SPL)
      double exp_cr;          // expansion-regime compression ratio
      double exp_tk;          // expansion-regime end kneepoint
      double tkgain;          // compression-start gain
      double tk;              // compression-start kneepoint
      double cr;              // compression ratio
      double bolt;            // broadband output limiting threshold
  };

}

// now, write functions to translate between CHA_DSL2 to CHA_DSL and CHA_WDRC2 to CHA_WDRC
void convertStructures_DSL(BTNRH_WDRC::CHA_DSL2 &dsl_in, CHA_DSL &dsl_out) {
  dsl_out.attack = dsl_in.attack;
  dsl_out.release = dsl_in.release;
  dsl_out.maxdB = dsl_in.maxdB;
  dsl_out.ear = dsl_in.ear;
  dsl_out.nchannel = dsl_in.nchannel;
  for (int i=0; i<dsl_in.nchannel; i++) {
      dsl_out.cross_freq[i] = dsl_in.cross_freq[i];
      dsl_out.tkgain[i] = dsl_in.tkgain[i];
      dsl_out.cr[i] = dsl_in.cr[i];
      dsl_out.tk[i] = dsl_in.tk[i];
      dsl_out.bolt[i] = dsl_in.bolt[i];      
  }  
}
void convertStructures_WDRC(BTNRH_WDRC::CHA_WDRC2 &agc_in, CHA_WDRC &agc_out) {
  agc_out.attack = agc_in.attack;
  agc_out.release = agc_in.release;
  agc_out.fs = agc_in.fs;
  agc_out.maxdB = agc_in.maxdB;
  agc_out.tkgain = agc_in.tkgain;
  agc_out.tk = agc_in.tk;
  agc_out.cr = agc_in.cr;
  agc_out.bolt = agc_in.bolt;

  //The fields td, nz, new, wt are not available in agc_in, so we won't do anything with them.
  //Alternatively, we could have zero'd out these fields here when setting agc_out.
}
void convertStructures_AFC(BTNRH_WDRC::CHA_AFC &afc_in, CHA_AFC &afc_out) {
  if (afc_in.default_to_active == 0) {
    Serial.println("covnertStructures_AFC: *** WARNING ***");
    Serial.println("    : The given AFC configuration from GHA_Constants.h had default_to_active set false.");
    Serial.println("    : This is not supported in this CHAPRO-based example.");
    Serial.println("    : Ignoring this setting.");
  }
  
  afc_out.rho = afc_in.rho;
  afc_out.eps = afc_in.eps;
  afc_out.mu = afc_in.mu;
  afc_out.afl = afc_in.afl;

  //None of the other AFC are available in afc_in, so we won't do anything with them.
  //Alternatively, we could have zero'd out these fields here when setting afc_out.
}

#endif
