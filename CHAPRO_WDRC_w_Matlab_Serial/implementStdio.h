

//The Arduino-Teensy-Tympan does not normally support printf because there is no stdout or console.
//This hack restores some of that capability to enable one to use printf

// https://forum.pjrc.com/threads/28473-Quick-Guide-Using-printf()-on-Teensy-ARM?highlight=stdout
#include <cstdio>
auto &debugOut = Serial;

extern "C" {
  int _write(int fd, const char *buf, int len) {
    int ret_val;
    
    // Send both stdout and stderr to debugOut
    if (fd == stdout->_file || fd == stderr->_file) {
      ret_val = debugOut.write(reinterpret_cast<const uint8_t *>(buf), len);
      debugOut.flush();
      return ret_val;
    }
  
    // Doing the following keeps this compatible with Print.cpp's requirements
    Print *p = reinterpret_cast<Print *>(fd);
    ret_val = p->write(reinterpret_cast<const uint8_t *>(buf), len);
    p->flush();
    return ret_val;
  }
}
