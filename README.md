# TouchDRO-Simulator-on-ESP32
The first try to get a deeper understanding on the BT SPP protocol used by TouchDRO at http://www.yuriystoys.com/

 
                This is just some testing code for the protocol. No code for reading calipers !!


  TouchDRO doeas at the moment only work with Bluetooth Classis and Serial Port Profile (SPP).
  SPP does not use baudrate, the BT Device just sends as fast as the over air inteface can handle.
  This is the same with the well known HC05 Devices, the Baudrate is just an higlevel software 
  implementation when it acts as a server/client and uart bridge.

  X,Y and Z -Axis: 
  What I've managed to figure out on the Serial Protocol used by TouchDRO is that it uses plain ASCII (char).
  The format is Axis + Absolute possition in Counts Per Inch(long) and ended with a terminating character.
  
  Example:  X1234567;Y2345678;Z3456789;
  
  W - Axis:
  Configured as an A-axis
  A standard A-axis (This would be nice!!)
  ToDo: Use W-axis as an A-axis. radians to degree conversion ?

  Tachometer:
  Same format, but uses "T" for tach and rpm in unsinged-long (Wy not a signed when reversing the spindel ?)
  Example T123456789;
  ToDo: test a signed long for negative read out then a CV and a CCV flag can be used

  Probe:
  Uses "P" and a unsigned int (using a bool for better looking code) (ToDo: check if inverted or not9
  Example: P1; and P0;
