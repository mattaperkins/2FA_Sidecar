#!/usr/local/bin/python
# Script that will set the time via USB fallback if wifi is not around. 
# Set the serial port bellow and run the program at the prompt
# Needs the python serial lib. Will only work if your computers clock is accurate 
# Tested on MacOS and Linux. 


import time
import serial

SERIAL_PORT = '/dev/cu.usbmodem2214302'  # Use 'COM3' for Windows if needed
BAUD_RATE = 115200

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        time.sleep(2)  # Wait for ESP32 to reset

        # Get current Unix timestamp
        timestamp = int(time.time())
        print("Sending current Unix time: {}".format(timestamp))

        # Send the timestamp followed by newline
        ser.write(str(timestamp) + '\n')

        # Optional: read response
        time.sleep(1)
        while ser.inWaiting():
            line = ser.readline().strip()
            print(line)

        ser.close()

    except serial.SerialException as e:
        print("Serial error: {}".format(e))

if __name__ == "__main__":
    main()

