dae2taascene
============

author:  Thomas Atwood (tatwood.net)
date:    2011
license: see UNLICENSE.txt

This is a utility for converting collada files into a intermediate taascene
file. The taascene file can then be read, processed, and converted into a
final production format.

Usage
=====
    Required arguments
      -dae                Path to input collada file
      -o [FILE]           Path to outputtaascene file

    Options
      -up [Y|Z]           Up axis for exported obj file

Building
========

## Linux ###
The the following dependencies are required to build on Linux:
    ../libdae
    ../taamath
    ../taascene
    ../taasdk
    ../extern/expat
    -lm

### Windows ###
The the following dependencies are required to build on Microsoft Windows:
    ../libdae
    ../taamath
    ../taascene
    ../taasdk
    ../extern/expat
