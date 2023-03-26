#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

int main() {
    // Read an image from file
    cv::Mat image = cv::imread("C:\\Users\\Shado\\OneDrive\\Documents\\drone.png");

    // Check if the image was loaded successfully
    if (image.empty()) {
        std::cout << "Error: Could not read the image file" << std::endl;
        return 1;
    }

    // Create a window to display the image
    cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE);

    // Show the image in the window
    cv::imshow("Display Image", image);

    // Wait for a key press and then close the window
    cv::waitKey(0);

    return 0;
}