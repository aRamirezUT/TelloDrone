// This program is a test program for the telloc library.
// Connects to the tello and prints out video size and state information
// Assumes you are connected to the tello over wifi.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "telloc.h"

// test main function
int main(void) {
    // default connect on all interfaces 0.0.0.0
    telloc_connection *connection=telloc_connect();

//    // connect on a specific interface
//    telloc_connection *connection= = telloc_connect_interface(""192.168.10.2");

    // check if connection was made successfully
    if (!connection) {
        printf("Connection failed\n");
        return 1;
    }

    // send a command to the Tello drone
    if (telloc_send_command(connection, "streamon", 8, NULL, 0)) {
        return 1;
    }

    // state is a string
    char *state = malloc(TELLOC_STATE_SIZE);

    // our video is 960x720 pixels, encoded as RGB 8-bit format
    unsigned char *image = malloc(TELLOC_VIDEO_SIZE);
    unsigned int image_bytes;
    unsigned int image_width;
    unsigned int image_height;


    // check if user is pressing any key asynchronously
    while (1) {

        // read a video frame from the Tello drone
        if(!telloc_read_image(connection, image, TELLOC_VIDEO_SIZE, &image_bytes, &image_width, &image_height)) {
            printf("Image: %d bytes; %d x %d\n", image_bytes, image_width, image_height);
        }
        // try to read state now
        if(!telloc_read_state(connection, state, TELLOC_STATE_SIZE)) {
            printf("State: %s\n", state);
        }
        sleep(1);
    }

    // disconnect from the Tello drone
    if (telloc_disconnect(connection)) {
        return 1;
    }
    connection = NULL;

    return 0;
}
