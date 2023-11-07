#!/bin/bash

# Compile metadata.c
gcc -o prog metadata.c

# Call the compiled file with "imagine.bmp" argument
./prog imagine.bmp
