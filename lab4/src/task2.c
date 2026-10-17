#include <stdio.h>
#include <unistd.h>

int main(void) {
    if (fork() == 0) {
        _exit(0);          /* ребёнок сразу умирает -> зомби */
    }
    sleep(30);             /* родитель НЕ вызывает wait() */
    return 0;
}

//ps -ef | grep defunct