# Keyboard-LEDs-driver
A driver that gets a string of characters from users and displays its corresponding binary values on keyboard LEDs :)

It includes the following functions

A function to get inputs form users.

A function which converts the input string to a linked list of characters. This function
 uses the kernel facilities for linked list.

A function for converting a character to the corresponding binary value.

A function to display a binary value on the Keyboard LEDs.


To use, you must create a special device file in /dev directory using mknod command and then echo a string to the special device 
file 
