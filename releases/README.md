# Binary releases for ESP32 versions will be placed here

These releases will be in the form of a .bin file that can be uploaded to the Morse Tutor device connected to a computer with a USB cable.

You need to use the esptool software from the makers of the ESP chips to upload the .bin file. The release version filename to upload will be  named something like `tutor1-2.bin`

# Prerequisites
Install esptool: 
1. You need Python installed. you'll access it from a command line terminal on your computer.
2. Run `pip install esptool` in your terminal to install esptool.
   (Info on esptool: https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html)

# Uploading
1. plug your Morse Tutor into the USB cable connected to your computer.
2. Determine which com port your computer thinks is connected to the Tutor.
3. In the following command, replace `COM_PORT` with your actual port (e.g., COM3 for Windows or /dev/ttyUSB0 for Linux/Mac)
4. To upload, give the following command `python esptool.py --port COM_PORT write_flash 0x0 tutor1-2.bin`
