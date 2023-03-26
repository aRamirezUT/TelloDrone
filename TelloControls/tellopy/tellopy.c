#include <Python.h>
#include "telloc.h"

static telloc_connection *connection=NULL;

static PyObject *tellopy_connect(PyObject *self, PyObject *args)
{
    if(connection) {
        // already connected
        return PyBool_FromLong(1);
    }

    connection = telloc_connect();
    if (!connection) {
        // connect failed
        return PyBool_FromLong(0);
    }

    return PyBool_FromLong(1);
}

static PyObject *tellopy_send_command(PyObject *self, PyObject *args)
{
    PyObject *connection_ptr;
    const char *command;
    unsigned int length;
    char *response=malloc(sizeof(char) * 1024);
    unsigned int response_length=1024;

    if (!PyArg_ParseTuple(args, "s", &command)) {
        PyErr_SetString(PyExc_TypeError, "tellopy.send_command() takes one string argument");
        return NULL;
    }
    length = strlen(command);

    if (telloc_send_command(connection, command, length, response, response_length)) {
        return PyBool_FromLong(0);
    }
    PyObject *response_obj = PyUnicode_FromString(response);
    free(response);
    return response_obj;
}

static PyObject *tellopy_read_state(PyObject *self, PyObject *args)
{
    PyObject *connection_ptr;
    char *state_buffer = malloc(sizeof(char) * TELLOC_STATE_SIZE);
    unsigned int state_buffer_length = TELLOC_STATE_SIZE;

    if (telloc_read_state(connection, state_buffer, state_buffer_length)) {
        return Py_None;
    }

    // convert buffer to python string
    PyObject *state = PyUnicode_FromString(state_buffer);
    free(state_buffer);
    return state;
}

static PyObject *tellopy_read_image(PyObject *self, PyObject *args)
{
    PyObject *connection_ptr;
    unsigned char *image_buffer=malloc(sizeof(unsigned char) * TELLOC_VIDEO_SIZE);
    unsigned int image_buffer_length=TELLOC_VIDEO_SIZE;
    unsigned int image_bytes=0;
    unsigned int image_width=0;
    unsigned int image_height=0;
    if (telloc_read_image(connection, image_buffer, image_buffer_length, &image_bytes, &image_width, &image_height)) {
        return Py_None;
    }
    // nobody knows how to use byes...
    // so convert buffer to a python list of integers
    PyObject *image = PyList_New(image_bytes);
    for (unsigned int i=0; i<image_bytes; i++) {
        PyList_SetItem(image, i, PyLong_FromLong(image_buffer[i]));
    }
    free(image_buffer);

    // return tuple of (image_width, image_height, image_buffer)
    PyObject *image_tuple = PyTuple_New(3);
    PyTuple_SetItem(image_tuple, 0, PyLong_FromLong(image_width));
    PyTuple_SetItem(image_tuple, 1, PyLong_FromLong(image_height));
    PyTuple_SetItem(image_tuple, 2, image);
    return image_tuple;
}

static PyObject *tellopy_disconnect(PyObject *self, PyObject *args)
{
    if (connection == NULL) {
        // already disconnected
        return PyBool_FromLong(1);
    }

    if (telloc_disconnect(connection)) {
        // disconnect failed... something is wrong
        PyErr_SetString(PyExc_TypeError, "tellopy_disconnect() failed");
        return NULL;
    }
    connection = NULL;

    return PyBool_FromLong(1);
}

// disconnect on free
static void tellopy_free(void *module)
{
    tellopy_disconnect(NULL, NULL);
}


static PyMethodDef tellopy_methods[] = {
    {"connect", tellopy_connect, METH_VARARGS, "Connect to the Tello drone using the default address"},
    {"send_command", tellopy_send_command, METH_VARARGS, "Send a command to the Tello drone and receive a response"},
    {"read_state", tellopy_read_state, METH_VARARGS, "Receive the most recent state of the Tello drone"},
    {"read_image", tellopy_read_image, METH_VARARGS, "Receive an RGB format video frame from the Tello drone"},
    {"disconnect", tellopy_disconnect, METH_VARARGS, "Disconnect from the Tello drone"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef tellopy_module = {
    PyModuleDef_HEAD_INIT,
    "tellopy",
    "Python interface to the Tello drone",
    -1,
    tellopy_methods,
    NULL,
    NULL,
    NULL,
    &tellopy_free
};

PyMODINIT_FUNC PyInit_tellopy(void)
{
    return PyModule_Create(&tellopy_module);
}

PyMODINIT_FUNC PyInit_libtellopy(void)
{
    return PyModule_Create(&tellopy_module);
}
