#include <stdio.h>

int foo(int y, int z) {
    int s = 0;
    int x = y + z;
    int i;
    for (i = x; i <= 100; i++) {
        s = 1 + x + s;
    }
    return s;
}

int main() {
    return foo(1, 2);
}