#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
void disableRawMode(){
    
}
void enableRawMode(){ // typing password in terminal
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO); // c_lflag - local flags, ECHO - bitflag
    // In general bitflags store many settings in one number. Here the echo functionality is the bit 4. So turn it off we do bit manipulation

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // TCSAFLUSH - tells when to apply the change
}

int main(){

    enableRawMode();

    char c;
    while(read(STDIN_FILENO, &c, 1)==1 && c!='q'); //read 1 byte from i/p into c until no bytes are left
    return 0;
}