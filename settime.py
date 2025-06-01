#!/usr/local/bin/python
# Script that will set the time via USB fallback if wifi is not around.
# Set the serial port below and run the program at the prompt
# Needs the python serial lib. Will only work if your computer's clock is accurate.
# Tested on MacOS and Linux. You may need to adjust the tty.usbmodem fro your platform 

import time
import serial
import glob

BAUD_RATE = 115200

def list_serial_ports():
    # List devices matching /dev/tty.usbmodem*
    ports = glob.glob('/dev/tty.usbmodem*')
    return ports

def choose_serial_port(ports):
    if not ports:
        print("No USB serial devices found.")
        return None

    print("Available USB serial devices:")
    for i, port in enumerate(ports, 1):
        print("{}: {}".format(i, port))

    while True:
        try:
            choice = int(raw_input("Select the serial device by number: "))
            if 1 <= choice <= len(ports):
                return ports[choice - 1]
            else:
                print("Invalid choice. Please enter a valid number.")
        except ValueError:
            print("Invalid input. Please enter a number.")

def main():
    ports = list_serial_ports()
    serial_port = choose_serial_port(ports)

    if not serial_port:
        return

    print("Using serial port: {}".format(serial_port))

    try:
        ser = serial.Serial(serial_port, BAUD_RATE, timeout=2)
        time.sleep(2)  # Wait for ESP32 to reset

        # Get current Unix timestamp
        timestamp = int(time.time())
        print("Sending current Unix time: {}".format(timestamp))

        # Send the timestamp followed by newline
        ser.write((str(timestamp) + '\n').encode())

        # Optional: read response
        time.sleep(1)
        while ser.inWaiting():
            line = ser.readline().strip()
            print(line.decode())

        ser.close()

    except serial.SerialException as e:
        print("Serial error: {}".format(e))

if __name__ == "__main__":
    main()

