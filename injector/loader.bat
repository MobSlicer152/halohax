@echo off
set "output=%*loader.bin"
echo Assembling %~dp0loader.asm into %output%
nasm %~dp0loader.asm -fbin -o %output%
exit /b %=ExitCode%
