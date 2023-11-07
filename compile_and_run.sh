#!/bin/bash

# Compile metadata.c with -Wall option
gcc -Wall -o prog metadata.c

# Check for compilation errors
if [ $? -ne 0 ]; then
    echo "Compilation failed. Exiting..."
    exit 1
fi

# Call the compiled file with "imagine.bmp" argument
./prog imagine.bmp

