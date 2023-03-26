#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include "tellopy/telloc/telloc.h"

int controls()
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return ch;
}

int main()
{
    telloc_connection *connection=telloc_connect();

    char *command=malloc(TELLOC_STATE_SIZE);
    sprintf(command, "streamon");
    char *response=malloc(TELLOC_STATE_SIZE);
    int ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
    printf("Response was %s\n", response);

    unsigned char *image = malloc(TELLOC_VIDEO_SIZE);
    unsigned int image_bytes;
    unsigned int image_width;
    unsigned int image_height;
    int ret_video = telloc_read_image(connection, image, TELLOC_VIDEO_SIZE, &image_bytes, &image_width, &image_height);
    if (ret_video==0)
    {
        printf("Image: %d bytes; %d x %d\n", image_bytes, image_width, image_height);
    }

    char *state = malloc(TELLOC_STATE_SIZE);
    int ret_state = telloc_read_state(connection, state, TELLOC_STATE_SIZE);
    if (ret_state==0)
    {
        printf("State: %s\n", state);
    }

    // ret_command = telloc_send_command(connection, "takeoff", strlen("takeoff"), response, TELLOC_STATE_SIZE);
    // sleep(10);

    // ret_command = telloc_send_command(connection, "land", strlen("land"), response, TELLOC_STATE_SIZE);

    while (1)
    {
        int ch = controls();
        if (ch == 27) // emergency stop w/ [esc]
        {
            break;
        }
        switch (ch) 
        {
            case 'g':
                sprintf(command, "takeoff");
                sleep(2);
                break;
            case 'q':
                sprintf(command, "land");
                break;
            case 'w':
                sprintf(command, "forward 40");
                break;
            case 'a':
                sprintf(command, "left 40");
                break;
            case 's':
                sprintf(command, "back 40");
                break;
            case 'd':
                sprintf(command, "right 40");
                break;
            case 'r':
                sprintf(command, "up 40");
                break;
            case 'f': // ASCII code for [shift]
                sprintf(command, "down 40");
                break;
            case ',': // ASCII code for [<-]
                sprintf(command, "cw 150");
                break;
            case '.': // ASCII code for [->]
                sprintf(command, "ccw 150");
                break;
            case ' ': // emergency land
                sprintf(command, "emergency");
                break;
            default:
                continue;
        }
        // send command here
        printf("command: %s\n", command);
        ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
        if(ret_command == 0 && response != NULL) {
            printf("Response was %s\n", response);
        }
        

        //reset command
        sprintf(command, "");
        fflush(stdin);
        fpurge(stdin);

        telloc_read_state(connection, state, TELLOC_STATE_SIZE);
        printf("State: %s\n", state);
        sleep(1);
    }
    free(command);
    free(response);
    telloc_disconnect(connection);

    return 0;
}