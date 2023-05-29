// Dummy test program for injector to target, so I don't need to have Halo running in the background

#include <iostream>

#include <windows.h>

int main(int argc, char* argv[])
{
    std::cout << std::endl << "Press any key to continue . . . " << std::endl;

    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
    DWORD consoleMode = 0;
    GetConsoleMode(console, &consoleMode);
    consoleMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(console, consoleMode);

    std::cin.get();

    consoleMode &= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
    SetConsoleMode(console, consoleMode);
}