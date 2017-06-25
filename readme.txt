Raspberry Pi GBA Loader - Boot up a GameBoy Advance using Multiboot (no cart req.)

## Description
The GBA file that you wish to load into the GBA is stored on the raspberry pi local file system.

1. run multiboot program
2. turn on GBA
3. next is nothing. very simple! :D


## Wiring
GBA connector(cable side). probably any cable color is different.

  T
1 3 5         1 3v, xxx 3 SI, wht 5 SC, red
2 4 6         2 SO, blk 4 SD, grn 6 GN, xxx

GBA   --- raspberry pi b+

6-GND     GND
3-SI      GPIO 10 (SPI_MOSI)
2-SO      GPIO  9 (SPI_MISO)
5-SC      GPIO 11 (SPI_SCLK)


## command log
$ gcc multiboot.c -lwiringPi -o multiboot
$ ./multiboot

Looking for GBA 0x72026202
0x72026202 0x00006202  ; Found GBA
0x72026202 0x00006102  ; Recognition OK
Send Header(NoDebug)
0x00020000 0x00006200  ; Transfer of header data complete
0x72026200 0x00006202  ; Exchange master/slave info again
0x72026202 0x000063d1  ; Send palette data
0x73c563d1 0x000063d1  ; Send palette data, receive 0x73hh****
0x73c563d1 0x000064d4  ; Send handshake data
0x739564d4 0x000013c0  ; Send length info, receive seed 0x**cc****
Send encrypted data(NoDebug)
Wait for GBA to respond with CRC 0x00750065
0x00750065 0x00000066  ; GBA ready with CRC
0x5a470066 0x00005a47  ; Let's exchange CRC!
CRC ...hope they match!
MulitBoot done


## Author

Ken Kaarvik    kkaarvik@yahoo.com    Nov 13, 2010
akkera102                            Nov 08, 2014(mbed version)
akkera102                            May 20, 2016(raspberry pi version)






## 2017/06/26
## added. backup savedata tool

1. at one's own risk.
2. Tool can't action keyboard. you use gameboy button.(L+A+B or R+A+B)
3. Please press the button at the same time. It often happens that timing does not match.
