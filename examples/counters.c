#include "../scoro.h"
#include <unistd.h>

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
        usleep(1000000);
    }
}
