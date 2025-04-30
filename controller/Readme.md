
root@de1-soc:~/test_gamepad# gcc -o test_hid.c usbkeyboard.c -lusb-1.0          /usr/lib/gcc/arm-linux-gnueabihf/5/../../../arm-linux-gnueabihf/crt1.o: In function `_start':
(.text+0x28): undefined reference to `main'
collect2: error: ld returned 1 exit status



/usr/lib/gcc/arm-linux-gnueabihf/5/../../../arm-linux-gnueabihf/crt1.o: In function `_start':
(.text+0x28): undefined reference to `main'
collect2: error: ld returned 1 exit status
root@de1-soc:~/test_gamepad# 
