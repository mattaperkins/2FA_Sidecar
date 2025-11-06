#!/usr/local/bin/python
# Script to set the time via USB fallback if Wi-Fi is not available.
# Compatible with macOS and Linux using /dev/tty.usbmodem*
# Works with Python 2.7+ and Python 3.x

import time
import serial
import glob
import sys

BAUD_RATE = 115200

def list_serial_ports():
    # List all devices that match USB modem pattern
    return glob.glob('/dev/tty.usbmodem*')

def choose_serial_port(ports):
    if not ports:
        print("No USB serial devices found.")
        return None

    if len(ports) == 1:
        print("Found one serial port: {}".format(ports[0]))
        return ports[0]

    print("Available USB serial devices:")
    for i, port in enumerate(ports, 1):
        print("{}: {}".format(i, port))

    while True:
        try:
            # raw_input for Python 2, input for Python 3
            if sys.version_info[0] < 3:
                choice = int(raw_input("Select the serial device by number: "))
            else:
                choice = int(input("Select the serial device by number: "))
            if 1 <= choice <= len(ports):
                return ports[choice - 1]
            else:
                print("Invalid choice. Please enter a number between 1 and {}.".format(len(ports)))
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
        time.sleep(2)  # Give the microcontroller time to reset

        timestamp = int(time.time())
        print("Sending current Unix time: {}".format(timestamp))

        ser.write((str(timestamp) + '\n').encode())

        time.sleep(1)
        while ser.inWaiting():
            line = ser.readline().strip()
            print(line.decode())

        ser.close()

    except serial.SerialException as e:
        print("Serial error: {}".format(e))

if __name__ == "__main__":
    main()

