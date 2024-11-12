# FT2232H-to-FPGA-via-SPI
Tool I made to send data back and forth to an FPGA with an FT2232H chip

Nov 12 - 2024. After 7 years, bringing this back from the dead.

Tool to talk to and ransfer binary data to a Mojo FPGA at USB 2 speeds. 


This is a derivative of my other repo here: 

clone this into the examples folder and it should work. 


Note - The defualt FTDI driver is the Virtual Com Port, it starts automatically when the device is plugged in. To develop d2xx drivers you have to temporarily remove VCP (until device is reset or power cycled): 

    sudo rmmod ftdi_sio; sudo rmmod usbserial;

    