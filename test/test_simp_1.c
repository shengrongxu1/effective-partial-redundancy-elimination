#include <stdio.h>

int main() {
    int a = 10;
    int b = 0;
    int c = 9;
    int d = 7;
    int e = 0;

    if (b == 0) {
        e = a + d;
    }
    else{
        e = a + c;
    }
    e = a + d;
    return e;
}