
#ifndef AudioPath_Base_h
#define AudioPath_Base_h

#include <vector>

// AudioPath_Base:
//
// This is a base class intended to provide an interface (and some common functionality) for derived AudioPath classes
// that are actually employed by the user.
//
// The idea is that an AudioPath is a composite class that instantiates and manages a whole chain of AudioStream_F32
// audio-processing objects.  One can simply instantiate an AudioPath and all of the constituant objects and connections
// are created.  The only thing left to do is connect the final output.
//
// Note that the final output is treated separately because the final output AudioStream_F32 object should be instantiated
// last, after *all* of the AudioPath instances are created.  This ensures that the output is serviced (behind the scenes,
// by AudioStrea::update_all()) after all of the AudioPath continuents.  This minimizes audio latency through the system.
// 
// So, in your overall program, you should instantiate your audio source (AudioInputI2S_F32, for example), then you
// should instantiate all of the your AudioPath objects, then you should instantiate your output desitnation (AudioMixer8_F32
// and AudioOutputI2S_f32, for example)
//
class AudioPath_Base {
  public:
    AudioPath_Base(void) {}
    AudioPath_Base(AudioSettings_F32 &_audio_settings, Tympan *_tympan_ptr, EarpieceShield *_shield_ptr) : tympan_ptr(_tympan_ptr), shield_ptr(_shield_ptr) {}
    
    //Destructor: note that destroying Audio Objects is not a good idea because AudioStream (from Teensy)
    //does not currently support (as of Sept 9, 2024) having instances of AudioStream be destroyed.  The
    //problem is that AudioStream keeps pointers to all AudioStream instances ever created.  If an AudioStream
    //instance is deleted, AudioStream does not remove the instance from its lists.  So, it has stale pointers
    //that eventually take down the system.
    //So, it is *not* recommended that you destroy AudioStream objects or, therefore, AudioPath objects
    virtual ~AudioPath_Base() {
      for (int i=(int)patchCords.size()-1; i>=0; i--) delete patchCords[i]; //destroy all the Audio Connections (in reverse order...maybe less memroy fragmentation?)
      Serial.println("AudioPath_Base: Destructor: *** WARNING ***: destroying AudioStream instances, which is not supported by Teensy's AudioStream.");
      for (int i=(int)audioObjects.size()-1; i>=0; i--) delete audioObjects[i];       //destroy all the audio class instances (in reverse order...maybe less memroy fragmentation?)
    }

    //get the starting and ending nodes to enable audio connections
    virtual AudioSwitchMatrix4_F32* getStartNode(void) { if (startNode == NULL) Serial.println("AudioPath_Base (" + name + "): getStartNode: *** WARNING ***: Returning NULL pointer."); return startNode; }
    virtual AudioSwitchMatrix4_F32* getEndNode(void)   { if (endNode == NULL)   Serial.println("AudioPath_Base (" + name + "): getEndNode: *** WARNING ***: Returning NULL pointer.");   return endNode;   }

    //Loop through each audio object and set them all to the desired state of active or inactive.
    //Per the rules of AudioStream::update_all(), active = false should mean that the audio object
    //is not invoked, thereby saving CPU.
    virtual bool setActive(bool _active) {
      if (_active == true) setupHardware();  //calling this instance's setupHardware()
      
      for (auto & audioObject : audioObjects) { //iterate over each audio object 
        //Serial.println("AudioPath_Base: setActive(" + String(_active) + "): calling setActive() for object " + audioObject->instanceName);
        audioObject->setActive(_active);   // set whether it is active. (setting to False disables the algorithms and reduces CPU)
      } 
      return _active;
    }
    virtual void setupHardware(void) {}  //override this as desired in your derived class
  
    //Interface to allow for any slower main-loop updates
    virtual int serviceMainLoop(void) { return 0; }  //Do nothing.  You can override in your derived class, if you want to do something in the main loop.

    //Interface for use by SerialMonitor
    virtual void printHelp(void) { Serial.println("    : (none)"); };  //print default message
    virtual void respondToByte(char c) { };                           //default do nothing

    String name = "(unnamed)";   //human-readable name for your audio path.  You should override this in the constructor (or wherever) of your derived class.
  protected:
    AudioSwitchMatrix4_F32 *startNode = NULL; //instantiate as the first audio class in your AudioPath (even if you have no inputs)
    AudioSwitchMatrix4_F32 *endNode = NULL;   //instantiate as the last audio class in your AudioPath (even if you have no outputs)
    Tympan *tympan_ptr = NULL;
    EarpieceShield *shield_ptr = NULL;
    std::vector<AudioConnection_F32 *> patchCords;
    std::vector<AudioStream_F32 *> audioObjects;
};


#endif

