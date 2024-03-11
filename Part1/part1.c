#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    // System Call 1: write
    write(1, "Hello, System Call 1!\n", 22);

    // System Call 2: read
    char buffer[50];
    read(0, buffer, 50);

    // System Call 3: open
    int file_descriptor = open("testfile.txt", O_CREAT | O_WRONLY, 0644);

    // System Call 4: close
    close(file_descriptor);

    // System Call 5: getpid
    pid_t process_id = getpid();

    return 0;
}


