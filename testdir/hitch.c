#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
  
int main() {
        char buf;
        int bytes;
        while((bytes = read(0, &buf, 1)) > 0) {
                printf("%c", buf);
        }
        printf("\n");
        return 0;
}