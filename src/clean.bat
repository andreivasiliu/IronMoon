@echo off

PATH=%PATH%;C:\Program Files\Dev-Cpp\bin
PATH=%PATH%;C:\Dev-Cpp\bin
PATH=%PATH%;D:\Dev-Cpp\bin

make -f Makefile clean OS=Win32
pause
