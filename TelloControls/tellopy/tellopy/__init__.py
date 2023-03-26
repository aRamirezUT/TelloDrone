from . import libtellopy


def connect():
    """
    Connect to Tello drone, returning True if successful and False otherwise.
    :return: success
    """
    return libtellopy.connect()


def send_command(command):
    """
    Send a command to the Tello drone, returning the response.
    :param command: string command from Tello SDK api commands (e.g. 'battery?', 'takeoff', 'land', 'streamon', 'streamoff')
    :return: string response
    """
    return libtellopy.send_command(command)


def read_image():
    """
    Read a frame from the video stream, returning a tuple of (height, width, flat) where flat is a list of RGB values.
    :return: height, width, rgblist
    """
    return libtellopy.read_image()


def read_state():
    """
    Read the state of the drone, returning a dictionary of key-value pairs.
    :return: string of comma-separated state information. Should be easy to turn into a dictionary.
    """
    return libtellopy.read_state()


def disconnect():
    """
    Disconnect from the Tello drone.
    Throws and exception if there is a problme disconnecting. But honestly, you are likely to segfault in that case.
    :return: True if disconnected or disconnection successful.
    """
    return libtellopy.disconnect()

