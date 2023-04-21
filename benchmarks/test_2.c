#include <stdio.h>

int main() {
    int y = 1;
    int z = 2;
    int s = 0;
    int x = y + z;
    int i;
    for (i = x; i <= 5; i++) {
        s = 1 + x + s;
    }
    return s;
}