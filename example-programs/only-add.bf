+++ trailing plus characters are ignored due to the way buildIR is implemented
    compiling this program will, create an executable that does not have the add instructions in the asm
    to test it do ./bfc example-programs/only-add -o add -a 
