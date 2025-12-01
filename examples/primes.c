#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#ifdef _WIN32
#   include <windows.h>
#elif defined(__unix__)
#   include <fcntl.h>
#   include <unistd.h>
#endif
#include "../scoro.h"

void flush_stdin();
void clear_line();
int set_stdin_nonblocking();
int set_stdin_blocking();
bool nonblocking_scanf(int* ret, const char* fmt, ...);

void calc_prime(Cr* c, long n_param, long* ret) {
    cr_begin(c);
    long n = n_param;
    long possible_prime = n;
    try_again:
    for (long i = possible_prime/2; i>1; --i) {
        cr_yield(c);
        if (possible_prime % i == 0) {
            possible_prime += 1;
            goto try_again;
        }
    }
    *ret = possible_prime;

    clear_line();
    printf("Closest prime after %ld is %ld\n", n, *ret);
    printf("Find closest prime after: ");

    cr_end(c);
}

Cr threads[10] = {0};
size_t next_inactive = 0;


void input_thread_func(Cr* c, long* n) {
    cr_begin(c);
    while (1) {
        printf("Find closest prime after: ");
        int ret;

        cr_label(c, CR_STATUS_BLOCKED);
        bool got_input = nonblocking_scanf(&ret, "%ld", n);
        if (!got_input) return;

        if (ret == 0) {
            if (getchar() == 'q') {
                set_stdin_blocking();
                exit(0);
            }
            flush_stdin();
            printf("Invalid input.\n");
        } else if (ret < 1) {
            perror("Tha fuck");
            exit(1);
        } else {
            threads[next_inactive++] = (Cr){0};
        }
    }
    cr_end(c);
}

#ifdef __unix__
int mini_sleep() { return usleep(1); }
#elif defined(_WIN32)
int mini_sleep() { Sleep(1); return 0; }
#endif



int main() {
    set_stdin_nonblocking();
    Cr input_thread = {0};

    puts("Input 'q' to quit.");
    while (1) {
        long n;
        input_thread_func(&input_thread, &n);
        for (size_t i = 0; i < next_inactive; ++i) {
            long ret = -1;
            calc_prime(&threads[i], n, &ret);
            if (threads[i].status == CR_STATUS_FINISHED) {
                threads[i] = threads[--next_inactive];
            }
        }
        if (next_inactive == 0) mini_sleep();
    }
    return 1;
}


void flush_stdin() { for (int c=' ';  c!='\n' && c!=EOF; c=getchar()); }

#if defined(_WIN32)

void clear_line() {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console, &csbi);
    COORD beggining_of_line = {.X=0, .Y=csbi.dwCursorPosition.Y};
    DWORD _;
    FillConsoleOutputCharacter(console, ' ', csbi.dwSize.X, beggining_of_line, &_);
    SetConsoleCursorPosition(console, beggining_of_line);
}

bool nonblocking_scanf(int* ret, const char* fmt, ...) {
    #define SEQ_INPUT_EVENT_BUF_SIZE 128
    INPUT_RECORD event_buffer[SEQ_INPUT_EVENT_BUF_SIZE];

    static DWORD event_count, already_read;
    HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);
    PeekConsoleInput(std_input, event_buffer, SEQ_INPUT_EVENT_BUF_SIZE, &event_count);

    static char strbuff[1024]; //TODO: I'm not sure what to do about this
    static int strbuff_index = 0;

    for (int i = already_read; i < event_count; ++i) {
        if (event_buffer[i].EventType == KEY_EVENT && event_buffer[i].Event.KeyEvent.bKeyDown) {
            char c = event_buffer[i].Event.KeyEvent.uChar.AsciiChar;
            switch (c) {
            case 0: break;
            case '\r':
                strbuff[strbuff_index] = '\0';
                printf("\r\n");

                va_list args;
                va_start(args, fmt);
                    *ret = vsscanf(strbuff, fmt, args);
                va_end(args);

                strbuff_index = 0;
                already_read  = 0;
                if (*ret != 0) {
                    FlushConsoleInputBuffer(std_input);
                }
                return true;

            case '\b':
                if (strbuff_index > 0) {
                    strbuff_index--;
                    printf("\b \b");
                }
                break;

            default: 
                strbuff[strbuff_index++] = c;
                putchar(c);
            }
        }
    }
    already_read = event_count;
    return false;
}

int set_stdin_nonblocking() { return 0; }
int set_stdin_blocking() { return 0; }

#elif defined(__unix__)
void clear_line() { printf("\r\033[K"); }

int set_stdin_nonblocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) return 1;
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) return 1;
    return 0;
}

int set_stdin_blocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) return 1;
    if (fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) == -1) return 1;
    return 0;
}

bool nonblocking_scanf(int* ret, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    *ret = vscanf(fmt, args);
    va_end(args);

    if (*ret < 0 && errno == EAGAIN) return false;

    return true;
}

#endif
