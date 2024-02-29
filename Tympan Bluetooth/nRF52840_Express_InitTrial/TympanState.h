// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This code will run on the **TYMPAN** to handle the Tympan's state, especially with regards to buttons on the App
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef TympanState_h
#define TympanState_h

class TympanState {
  public:
    TympanState(void) {}

    const int LED1_pin = 3;
    bool is_active_LED1 = false;
    const int LED2_pin = 4;
    bool is_active_LED2 = false;
};

#endif