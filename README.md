# TelloDrone: Into the Unknown
This code allows you to control a drone using keyboard inputs and view its live video feed. The drone used in this code is connected via a telloc_connection, and OpenCV library is used to capture and display the video feed.
# Dependencies

    OpenCV 2.4 or higher
    telloc.h (included in the code)

# Running the code

    Connect the drone via telloc_connection.
    Compile the code and run the executable.
    The live video feed from the drone will be displayed on a window titled "Drone Feed".
    Use the keyboard inputs to control the drone movement and perform actions.

# Keyboard Inputs

The following keyboard inputs are used to control the drone:

    'g' - Takeoff the drone.
    'q' - Land the drone.
    'w' - Move the drone forward.
    'a' - Move the drone left.
    's' - Move the drone back.
    'd' - Move the drone right.
    'r' - Move the drone up.
    'f' - Move the drone down.
    '.' - Rotate the drone clockwise.
    ',' - Rotate the drone anti-clockwise.
    ' ' - Emergency land the drone.
    '=' - Quit the program.

# Saving Screenshots

The code also saves a screenshot of the drone's live video feed at an interval of 1/4th of a second. These images are saved in the "images" folder in the same directory as the code.
Troubleshooting

    If the drone feed does not display, check that the drone is connected correctly and that the OpenCV library is installed properly.
    If there are issues with controlling the drone or receiving a response, check the connection with the drone and the telloc_connection.
