
2020-12-24 WEA

Clean up:

* Remove whatever SdFat from your Documents\Arduino\Libraries that you might have already installed
* Remove whatever MTP from your Documents\Arduino\Libraries that you might have already installed

Install Latest Toolchain:

* Install the latest Arduino IDE (1.8.13)
* Install the beta Teensyduino V 1.54 Beta #5

Get MTP Library:

* Download the MTP_T4 library from: https://github.com/WMXZ-EU/MTP_t4
* Unzip, remove "-master" from the folder name, and put it in Documents\Arduino\Libraries like usual

Open Arduino IDE and select board and USB type

* Start the Arduino IDE
* Under the Tools menu, select Board->Teensyduino->Teensy 3.6
* Under the Tools menu, select USB Type->MTP Disk (Experimental)

Try Chip's MTP Example

* Open "MTP-Test-Chip.ino", compile, and upload to your Tympan
* Your first time, you may need to: In Arduino IDE, under Tools->Ports, select the new port fo the newly flashed Teensy
* Any time you recompile and can't access the COM port, you may need to: Unplug your Tympan from the PC and then re-plug
* Open a Serial Monitor window, if not already open
* Send an 'h' (no quotes) for help. It should print  a directory listing of your SD card.
* Go to Windows, open My Computer (or whatever it’s called) and look for “Teensy” listed with your other drives!  You win!