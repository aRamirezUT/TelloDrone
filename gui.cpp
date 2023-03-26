#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <windows.h>
extern "C" {
#include "telloc.h"
}
using namespace cv;

int main() {
    telloc_connection *connection=telloc_connect();
    char *command=(char*)malloc(TELLOC_STATE_SIZE);
    sprintf(command, "streamon");
    char *response=(char*)malloc(TELLOC_STATE_SIZE);
    int ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
    printf("Response was %s\n", response);
    
    unsigned char *image = (unsigned char*)malloc(TELLOC_VIDEO_SIZE);
    unsigned int image_bytes;
    unsigned int image_width;
    unsigned int image_height;

    char *state = (char *)malloc(TELLOC_STATE_SIZE);
    int ret_state = telloc_read_state(connection, state, TELLOC_STATE_SIZE);
    if (ret_state==0)
    {
        printf("State: %s\n", state);
    }

    unsigned long i = 0;
    unsigned int long imgCount = 0;
    char *fileName = (char*)malloc(TELLOC_STATE_SIZE);
    unsigned long message_sent_i=0;

    while (true)
    {
        // read and check for video input from rone
        int ret_video = telloc_read_image(connection, image, TELLOC_VIDEO_SIZE, &image_bytes, &image_width, &image_height);
        if (ret_video !=0)
        {
            continue;
        }
        
        // convert the image from the drone into cv
        auto frame = cv::Mat((int) image_height, (int) image_width, CV_8UC3, image);
        cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

        // If frame is empty, break loop
        if (frame.empty())
            continue;

        // Display the resulting frame
        imshow("Drone Feed", frame);

        // Wait for 1/4 of 5 miliseconds and save a screenshot of the video image
        i+=1;
        if (i%32 == 0) {
            snprintf(fileName, TELLOC_STATE_SIZE, "%s%d%s", "images/img_", imgCount, ".jpg");
            printf("Saving image: %d\n", imgCount);
            imwrite(fileName, frame);
            imgCount += 1;
        }
        
        int ch = waitKey(5);
        // printf("ch is %d\n", ch);
        switch (ch) 
        {
            case 'g':
                sprintf(command, "takeoff");
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
            case '.': // ASCII code for [<-]
                sprintf(command, "cw 15");
                break;
            case ',': // ASCII code for [->]
                sprintf(command, "ccw 15");
                break;
            case ' ': // emergency land
                sprintf(command, "emergency");
                break;
            case '=': // quit program
                sprintf(command, "emergency");
                ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
                if(ret_command == 0) {
                    printf("Response was %s\n", response);
                }
                Sleep(30);
                exit(1);
            default:
                ch = -1;
                break;
        }
        // printf("if1\n");
        if (ch != -1 && (i-message_sent_i) > 8) {
            printf("Command is: %s\n", command);
            // send the inputted key to the drone
            ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
            if(ret_command == 0) {
                printf("Response was %s\n", response);
            }
            message_sent_i = i;
        }
        // printf("if2\n");
        ret_state = telloc_read_state(connection, state, TELLOC_STATE_SIZE);
        if (ret_state==0 && i%10 ==0)
        {
            printf("State: %s\n", state);
        }
        Sleep(1);
        // printf("endsleep\n");
    }

    // Release the VideoCapture object and close all windows
    destroyAllWindows();

    return 0;
}
