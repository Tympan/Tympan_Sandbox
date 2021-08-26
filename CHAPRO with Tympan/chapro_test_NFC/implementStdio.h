
// https://forum.pjrc.com/threads/28473-Quick-Guide-Using-printf()-on-Teensy-ARM?highlight=stdout

#include <cstdio>

auto &debugOut = Serial;

extern "C" {
int _write(int fd, const char *buf, int len) {
  // Send both stdout and stderr to debugOut
  if (fd == stdout->_file || fd == stderr->_file) {
    return debugOut.write(reinterpret_cast<const uint8_t *>(buf), len);
  }

  // Doing the following keeps this compatible with Print.cpp's requirements
  Print *p = reinterpret_cast<Print *>(fd);
  return p->write(reinterpret_cast<const uint8_t *>(buf), len);
}
}
