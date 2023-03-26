// This program is a test program for the telloc library.
// Connects to the tello and displays a video stream from the drone.
// Assumes you are connected to the tello over wifi.
//
#include "telloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Convert raw rgb frame to bi format for Windows display
void rgb_to_bi(const unsigned char* rgb, unsigned char* bi, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            bi[(i * width + j) * 3 + 0] = rgb[(i * width + j) * 3 + 2];
            bi[(i * width + j) * 3 + 1] = rgb[(i * width + j) * 3 + 1];
            bi[(i * width + j) * 3 + 2] = rgb[(i * width + j) * 3 + 0];
        }
    }
}

// test main function
int main(void) {
    // default connect on all interfaces 0.0.0.0
    telloc_connection *connection = telloc_connect();
    if (!connection) {
        return 1;
    }

    // send a command to the Tello drone
    if (telloc_send_command(connection, "streamon", 8, NULL, 0)) {
        return 1;
    }

    // create a window for displaying our dynamic video
    HWND hwnd = CreateWindowEx(0, "STATIC", "Tello Video", WS_OVERLAPPEDWINDOW, 0, 0, 960, 720, NULL, NULL, NULL, NULL);

    // create a device context frame buffer for the video
    // our video is 960x720 pixels, encoded as RGB 8-bit format
    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 960;
    bmi.bmiHeader.biHeight = -720;  // upside-down without this
    bmi.bmiHeader.biPlanes = 3;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    // get the device context for the window
    HDC hdc = GetDC(hwnd);

    // show the window
    ShowWindow(hwnd, SW_SHOW);

    unsigned char *image = malloc(TELLOC_VIDEO_SIZE);
    unsigned char *bi = malloc(TELLOC_VIDEO_SIZE);
    unsigned int image_bytes;
    unsigned int image_width;
    unsigned int image_height;
    while(1) {
        // read a video frame from the Tello drone
        if(!telloc_read_image(connection, image, TELLOC_VIDEO_SIZE, &image_bytes, &image_width, &image_height)) {
            // display the video frame using Windows API
            rgb_to_bi(image, bi, image_width, image_height);
            StretchDIBits(hdc, 0, 0, image_width, image_height, 0, 0, image_width, image_height, bi, &bmi, DIB_RGB_COLORS, SRCCOPY);
            printf("Image: %d bytes; %d x %d\n", image_bytes, image_width, image_height);
            Sleep(5);
        }
        // windows event loop
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            // exit on escape key
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                return 0;
            }
        }
    }

    // disconnect from the Tello drone
    if (telloc_disconnect(connection)) {
        return 1;
    }
    connection = NULL;

    // free memory
    free(image);
    free(bi);

    return 0;
}