#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
extern "C" {
#include "telloc.h"
}
using namespace cv;
int main() {

// Create a VideoCapture object to capture video from the default camera (index 0)
    VideoCapture cap(0);

    telloc_connection *connection=telloc_connect();
    char *command=(char*)malloc(TELLOC_STATE_SIZE);
    sprintf(command, "streamon");
    char *response=(char*)malloc(TELLOC_STATE_SIZE);
    int ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
    printf("Response was %s\n", response);
    
    // Check if the camera is opened successfully
    if (!cap.isOpened())
    {
        std::cout << "Cannot open camera" << std::endl;
        return -1;
    }

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

    unsigned int i =0;

    while (true)
    {
        // printf("entering while loop");
        // Capture frame-by-frame
        // Mat frame;
        // cap.read(frame);
        int ret_video = telloc_read_image(connection, image, TELLOC_VIDEO_SIZE, &image_bytes, &image_width, &image_height);
        if (ret_video==0)
        {
            // printf("Image: %d bytes; %d x %d\n", image_bytes, image_width, image_height);
        }

        else {
            continue;
        }
        // printf("Just read from drone");
        auto frame = cv::Mat((int) image_height, (int) image_width, CV_8UC3, image);
        cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);

        // If frame is empty, break loop
        if (frame.empty())
            continue;

        // Display the resulting frame
        imshow("Drone Feed", frame);

        // Wait for 1 millisecond and check for user input to exit
        // if (waitKey(1) == 'q')
        //     break;

        if (i%16 != 0)
            {
                waitKey(5);
                i+=1;
                continue;
            }
        // printf("continuing");
        int ch = waitKey(1);
        if (ch == 27) // emergency stop w/ [esc]
        {
            break;
        }
        // printf("switch statement:%d ", ch);
        switch (ch) 
        {
            case 'g':
                sprintf(command, "takeoff");
                // sleep(2);
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
                waitKey(30);
                exit(1);
            default:
                continue;

            }

            // send command here
            printf("command: %s\n", command);

            ret_command = telloc_send_command(connection, command, strlen(command), response, TELLOC_STATE_SIZE);
            if(ret_command == 0 && response != NULL) {
               printf("Response was %s\n", response);
            }
        i+=1;   
    }

    // Release the VideoCapture object and close all windows
    cap.release();
    destroyAllWindows();

    return 0;
}



    // // Read an image from file
    // cv::Mat image = cv::imread("C:\\Users\\Shado\\OneDrive\\Documents\\drone.png");

    // // Check if the image was loaded successfully
    // if (image.empty()) {
    //     std::cout << "Error: Could not read the image file" << std::endl;
    //     return 1;
    // }

    // // Create a window to display the image
    // cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE);

    // // Show the image in the window
    // cv::imshow("Display Image", image);

    // // Wait for a key press and then close the window
    // cv::waitKey(0);