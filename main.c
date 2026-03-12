/*** INCLUDES ***/
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>



/*** DEFINES ***/

#define CTRL_KEY(k) ((k) & 0x1f) //macro sets upper 3 bits to 0


/*** DATA ***/
struct editorConfig{ // GLOBAL STATE
    int screenrows;
    int screencols;
    struct termios orig_termios;// original attributes
};

struct editorConfig E;

/*** TERMINAL ***/

void die(const char *s){
    // Won't use atexit to exit screen because we won't be able to see the error
    write(STDIN_FILENO, "\x1b[2J", 4);
    write(STDIN_FILENO, "\x1b[H", 3);

    
    perror(s); // returns global error with desc. Also mentiones the string that worked well before the error to give context
    exit(1); //non-zero value-failure
}

void disableRawMode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios)==-1){
        die("txcsetattr");
    };
}
void enableRawMode(){ // typing password in terminal
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1){ //Send echo text|main.c || ./main <main.c
        die("tcgetattr");
    };
    atexit(disableRawMode); // resets back to original config after terminating the prog by exit() or return from main()

    struct termios raw = E.orig_termios; //copy the struct into raw from orig

    // tcgetattr(STDIN_FILENO, &raw);
    // raw.c_lflag &= ~(ECHO); // c_lflag - local flags, ECHO - bitflag
    // In general bitflags store many settings in one number. Here the echo functionality is the bit 4. So turn it off we do bit manipulation
    raw.c_iflag &= ~(BRKINT|INPCK|ISTRIP); // not used nowadays but the tradition of doing this still follows (misc flags)
    raw.c_iflag &= ~(ICRNL|IXON); // IXON - turn off Ctrl-S and Ctrl-Q (used in earlier days to stop communication to a device (like printer) and start communicating again)
    //ICRNL - turn Ctrl-M off
    raw.c_oflag &= ~(OPOST); // OPOST - turn off all output processing steps like moving cursor to next line,scrolling if necessary,etc
    raw.c_cflag |= ~(CS8); //misc flag
    raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG); // ICANON - turn off canonical mode (get input byte by byte instead of line by line)
    // ISIG - turn off SIGINT(Ctrl-C) and SIDTSTP(Ctrl-z)
    //IEXTEN - turn off Ctrl-V in linux and Ctrl-O in macOS
    // even using IEXTEN flag we won't get the proper output. SO we have to use the below line to print 22 for Ctrl+v
    
    //c_cc - control characters
    raw.c_cc[VMIN] = 0; // min number of bytes for read to return
    raw.c_cc[VTIME] = 10;// max amt of time to wait before read returns. If timeout -> 0 returns. Measured in tenths of sec. We set here 1/10th sec
    raw.c_cc[VLNEXT] = _POSIX_VDISABLE; // In wsl since ctrl +V is binded to pasting in terminal we can't see 22.
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)==-1){
        die("tcsetattr");
    }; // TCSAFLUSH - tells when to apply the change
}

char editorReadKey(){ // Waits and reads each keypress
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c,1))!=1){
        if(nread == -1 &&errno != EAGAIN){
            die("read");
        }
        return c;
    }
} // Deals with low-level terminal i/p

int getWindowSize(int *rows, int *cols){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col ==0){
        return -1;
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}


/*** OUTPUT ***/

//Drawing > for beginning of line
void editorDrawRows(){
    int y;
    for(y=0;y<E.screenrows;y++){
        write(STDOUT_FILENO, ">\r\n", 3);
    }
}

//VT100 escape seq
//escape sequence pattern - \x1b[... - used for colouring text,moving the cursor, refreshing the screen
void editorRefreshScreen(){ // \x1b - escape char / 27 in ASCII, J - erase in display, [2(used here),1,0(default)] - Clear [entire,screen up to cursor,cursor to end of screen]
    //Positions the cursor in the bottom of screen
    write(STDOUT_FILENO, "\x1b[2J",4); // writing 4 bytes out to the terminal
    // H - row & col arguments
    write(STDOUT_FILENO, "\x1b[H", 3); //escape seq - 3bytes , H -cursor position

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H",3);//repositioning cursor to top-left corner
}

/*** INPUT ***/

void editorProcessKeypress(){ // Waits for reply and handles it based on the keypressed
    char c = editorReadKey();

    switch (c){
        case CTRL_KEY('q'):
            write(STDIN_FILENO, "\x1b[2J", 4);
            write(STDIN_FILENO, "\x1b[H", 3);

            exit(0);
            break;
    }
}


/*** INIT ***/

void initEditor(){
    if(getWindowSize(&E.screenrows, &E.screencols)==-1){
        die("getWindowSize");
    }
}

int main(){

    enableRawMode();
    initEditor();
    // Below method is directly used by the kernal interface
    // read(STDIN_FILENO, &c, 1)==1 && c!='q' initially used inside while before adding time attribute
    // while(1){ //read 1 byte from i/p into c until no bytes are left
    //     char c = '\0';
    //     if(read(STDIN_FILENO, &c, 1) == -1 && errno!=EAGAIN){
    //         die("read");
    //     };
    //     if(iscntrl(c)){ // from ctype; tests whether c is control char(non printable ones)(ASCII 0-31,127)
    //         printf("%d\r\n",c); // \r - carriage return - necessary for the kernel to move the cursor back to starting of the line, else the program will just print in slanting way. This occurs when we use OPOST.
    //     }
    //     else{
    //         printf("%d ('%c')\r\n",c,c);
    //     }

    //     if(c == 'q')break;
    // } 

    while(1){
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}