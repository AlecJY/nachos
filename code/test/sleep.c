#include "syscall.h"
main() {
    int i = 10;
    while (i --> 0) {
        Sleep(i);
        PrintInt(i);
    }
}
