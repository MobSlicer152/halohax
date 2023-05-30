@echo off
set "output=%2%3%4%5%6loader.bin"
echo Assembling %~dp0loader.asm into %output% for %1
nasm %~dp0loader.asm -D%1 -fbin -o %output%
exit /b %=ExitCode%
