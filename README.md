# TelloDrone: Into the Unknown 🤖🚀
TelloDrone is a program that allows you to control a drone using keyboard inputs and view its live video feed. The drone used in this code is connected via a telloc_connection, and OpenCV library is used to capture and display the video feed. Afterwards you can take the screenshots of the live video and feed it into a Photogrammatry or Structure From Motion (SFM) application to create a mapping of your 'Unknown Environment'

# Dependencies

    OpenCV 2.4 or higher
    telloc.h (included in the code)
    A SFM or Photogrammatry app, we used "3D Zephyr"

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

TelloDrone saves a screenshot of the drone's live video feed at an interval of 1/4th of a milisecond. These images are saved in the "images" folder in the same directory as the code.

# Troubleshooting

    If the drone feed does not display, check that the drone is connected correctly and that the OpenCV library is installed properly.
    If there are issues with controlling the drone or receiving a response, check the connection with the drone and the telloc_connection.
    
# License
The Unlicense

```
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
```
