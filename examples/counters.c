#include "../scoro.h"
#ifdef _WIN32
#   include <windows.h>
#elif defined(__unix__)
#   include <unistd.h>
#endif

void counter(Cr* c, int id) {
    cr_begin(c);
    int counter = id;
    while (1) {
        printf("[%d] counter: %d\n", id, counter++);
        cr_yield(c);
    }
    cr_end(c);
}

int main() {
    Cr thread1 = {0};
    Cr thread2 = {0};

    while(1) {
        counter(&thread1, 1);
        counter(&thread2, 2);
        puts("tick");
        #ifdef _WIN32
        Sleep(1000);
        #elif defined(__unix__)
        sleep(1);
        #endif
    }
}
