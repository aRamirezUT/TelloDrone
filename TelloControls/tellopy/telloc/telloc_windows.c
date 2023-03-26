// Contains the Windows implementation of the telloc library
//
#include "telloc.h"

#include <stdio.h>

// include for Windows UDP networking and threading
#include <winsock2.h>
#include <process.h>
#include "video.h"

struct telloc_connection_ {
    // thread synchronization
    unsigned alive;

    // Tello command data socket
    SOCKET command_socket;
    HANDLE command_mutex;

    // Tello state data
    SOCKET state_socket;
    HANDLE state_mutex;
    char* state_buffer;
    unsigned state_size;

    // Tello video data
    SOCKET video_socket;
    HANDLE video_mutex;
    char* video_buffer;
    unsigned video_size;
    telloc_video_decoder video_decoder;

    // Threads
    HANDLE state_thread;
    HANDLE video_thread;
    HANDLE keepalive_thread;
};


// thread function to recieve state data from a UDP socket.
// this function is run in a thread
// argument: telloc_connection *connection
unsigned __stdcall thread_state(void *arg) {
    printf("State thread started\n");

    // get the connection from the argument
    telloc_connection *connection = (telloc_connection *) arg;

    // check if the state socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; State thread not starting.\n");
        printf("Call telloc_connect() before starting state thread.\n");
        return 1;
    }

    // get the socket from the connection
    SOCKET sock = connection->state_socket;

    // while alive, receive data on the socket
    while (connection->alive) {
        // receive data on the socket, assume UDP max buffer size
        char buffer[TELLOC_STATE_SIZE];

        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received == SOCKET_ERROR) {
            printf("State thread: error recieving data\n");
            // sleep for 5ms
            Sleep(5);
            continue;
        }

        // Windows acquire handle to the mutex
        WaitForSingleObject(connection->state_mutex, INFINITE);

        // copy the data string to the connection's state buffer
        // zero out the state buffer
        memset(connection->state_buffer, 0, TELLOC_STATE_SIZE);
        // copy the data string to the state buffer with memcpy_s
        memcpy_s(connection->state_buffer, TELLOC_STATE_SIZE, buffer, bytes_received);
        connection->state_size = bytes_received;

        // release the mutex
        ReleaseMutex(connection->state_mutex);

    }

    // close the socket
    closesocket(sock);

    return 0;
}


// Function to return a copy of the most recent state data
// argument: telloc_connection *connection
// argument: char *buffer
// argument: unsigned buffer_size
int telloc_read_state(telloc_connection *connection, char *state_buffer, unsigned state_buffer_length) {
    // clear the state buffer
    memset(state_buffer, 0, state_buffer_length);

    // check if the state socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; State not recieved.\n");
        printf("Call telloc_connect() before recieving state.\n");
        return 1;
    }

    if (state_buffer_length < connection->state_size) {
        printf("Buffer size too small to hold state data\n");
        return 1;
    }

    // check if there is new state data
    if (connection->state_size == 0) {
        return 1;
    }

    // Windows acquire handle to the mutex
    WaitForSingleObject(connection->state_mutex, INFINITE);

    // copy the data to the connection's state buffer
    memcpy(state_buffer, connection->state_buffer, connection->state_size);
    connection->state_size = 0;

    // release the mutex
    ReleaseMutex(connection->state_mutex);

    return 0;
}


// main function to recieve video data from a UDP socket.
// this function is run in a thread
// argument: telloc_connection *connection
unsigned __stdcall thread_video(void *arg) {
    printf("Video thread started\n");

    // get the connection from the argument
    telloc_connection *connection = (telloc_connection *) arg;

    // check if the video socket is open
    if (!connection->alive) {
        printf("Connection not initialized; Video thread not starting.\n");
        printf("Call telloc_connect() before starting video thread.\n");
        return 1;
    }

    // get the socket from the connection
    SOCKET sock = connection->video_socket;

    // allocate a buffer for the video data
    char *udp_buffer = malloc(65507);

    // allocate a buffer for multiple video packets
    unsigned char *h264_buffer = (unsigned char *) malloc(65507 * 10);
    unsigned h264_buffer_size = 0;

    // while alive, receive data on the socket
    while (connection->alive) {
        // receive data on the socket
        int bytes_received = recvfrom(sock, udp_buffer, 65507, 0, NULL, NULL);
        if (bytes_received == SOCKET_ERROR) {
            printf("Error receiving data: %d\n", WSAGetLastError());
            // sleep for 5ms
            Sleep(5);
            continue;
        }

        // check if the udp packet is a valid h264 start frame
        int new_frame = telloc_video_decoder_is_start_code((unsigned char*) udp_buffer, bytes_received);
        if (new_frame) {
            // check if there is a previous frame
            if (h264_buffer_size > 0) {
                // send the completed video packet to the video decoder
                telloc_video_decoder_decode(&connection->video_decoder, h264_buffer, h264_buffer_size);
            }
            // set buffer to the new frame's data
            memcpy(h264_buffer, udp_buffer, bytes_received);
            h264_buffer_size = bytes_received;

        } else if (h264_buffer_size > 0) {
            // not a new frame, but we already have data; append the data to the h264 buffer
            memcpy(h264_buffer + h264_buffer_size, udp_buffer, bytes_received);
            h264_buffer_size += bytes_received;
        }

        // check if there is a new frame
        if (new_frame && connection->video_decoder.frame_ready) {

            // Windows acquire handle to the mutex
            WaitForSingleObject(connection->video_mutex, INFINITE);

            // copy the data to the connection's video buffer
            memcpy(connection->video_buffer, connection->video_decoder.frame_buffer, connection->video_decoder.frame_size);
            connection->video_decoder.frame_ready = 0;
            connection->video_size = connection->video_decoder.frame_size;

            // release the mutex
            ReleaseMutex(connection->video_mutex);
        }
    }

    // close the socket
    closesocket(sock);

    // free the buffers
    free(udp_buffer);
    free(h264_buffer);

    return 0;
}


// function to read the most recent video frame
// argument: telloc_connection *connection
// argument: unsigned char *buffer
// argument: unsigned buffer_size
int telloc_read_image(telloc_connection *connection, unsigned char* image, unsigned int image_buffer_size, unsigned int* image_bytes, unsigned int* image_width, unsigned int* image_height) {
    // check if the video socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; Video not received.\n");
        printf("Call telloc_connect() before reading an image.\n");
        return 1;
    }

    // acquire the mutex
    WaitForSingleObject(connection->video_mutex, INFINITE);

    if (image_buffer_size < connection->video_size) {
        printf("Buffer size too small to hold video data\n");
        goto error;
    }

    // check if there is new video data
    if (connection->video_size == 0) {
        goto error;
    }

    // copy the data from the connection's video buffer
    memcpy(image, connection->video_buffer, connection->video_size);
    *image_bytes = connection->video_size;
    connection->video_size = 0;
    *image_width = connection->video_decoder.frame_width;
    *image_height = connection->video_decoder.frame_height;

    // release the mutex
    ReleaseMutex(connection->video_mutex);

    return 0;

error:
    // release the mutex
    ReleaseMutex(connection->video_mutex);
    return 1;
}


// thread to repeatedly send keepalive
// argument: telloc_connection *connection
unsigned __stdcall thread_keepalive(void* arg) {
    printf("Keepalive thread started\n");

    // get the connection from the argument
    telloc_connection *connection = (telloc_connection *) arg;

    // check if the command socket is open
    if (!connection->alive) {
        printf("Connection not initialized; Keepalive thread not starting.\n");
        printf("Call telloc_connect() before starting keepalive thread.\n");
        return 1;
    }

    // get the socket from the connection
    SOCKET sock = connection->command_socket;

    char buffer[1024];
    const char* query = "battery?";

    // use win32 to get current time in seconds
    unsigned int keepalive_time = GetTickCount() / 1000;

    // while alive, send keepalive
    while (connection->alive) {

        // if time since last keepalive is greater than 5 seconds, send keepalive
        if (GetTickCount() / 1000 - keepalive_time > 5) {
            // send the keepalive command
            telloc_send_command(connection, query, (unsigned int) strlen(query), buffer, sizeof(buffer));
            keepalive_time = GetTickCount() / 1000;
        }
        // wait 50 ms
        Sleep(50);
    }

    closesocket(sock);

    return 0;
}


// function to bind a socket to an address and port
int telloc_bind_udp_socket(SOCKET *sock, const char* address, unsigned short port) {
    // create a Windows UDP socket
    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (*sock == INVALID_SOCKET) {
        printf("Error creating socket: %d\n" , WSAGetLastError());
        return 1;
    }

    // bind the Windows socket to listen to an address and port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address);
    if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Error binding socket: %d\n", WSAGetLastError());
        return 1;
    }

    return 0;
}


// function to connect to the Tello drone on a specified interface address
telloc_connection *telloc_connect_interface(const char *interface_address) {
    printf("Connecting to Tello on interface %s\n", interface_address);

    // initialize Windows networking
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Error initializing Windows networking\n");
        return NULL;
    }

    // allocate a connection pointer
    telloc_connection *connection = malloc(sizeof(telloc_connection));

    // set the connection's alive flag to 0 to stop any threads
    connection->alive = 0;

    SOCKET command_sock = 0;
    SOCKET state_sock = 0;
    SOCKET video_sock = 0;

    // create the command mutex (only one command can be sent at a time)
    connection->command_mutex = CreateMutex(NULL, FALSE, NULL);
    // bind the command socket to our interface and port
    if (telloc_bind_udp_socket(&command_sock, interface_address, TELLOC_COMMAND_PORT) != 0) {
        goto error;
    }

    // bind the state socket to our interface and port
    if (telloc_bind_udp_socket(&state_sock, interface_address, TELLOC_STATE_PORT) != 0) {
        goto error;
    }

    // bind the video socket to our interface and port
    if (telloc_bind_udp_socket(&video_sock, interface_address, TELLOC_VIDEO_PORT) != 0) {
        goto error;
    }

    // set the connection's sockets
    connection->alive = 1;
    connection->command_socket = command_sock;
    connection->state_socket = state_sock;
    connection->video_socket = video_sock;

    // Send a command and get a response. This is to initialize the connection.
    // set the response timeout
    int timeout = TELLOC_RESPONSE_TIMEOUT;
    if (setsockopt(command_sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) == SOCKET_ERROR) {
        printf("Error setting socket timeout: %d\n", WSAGetLastError());
        goto error;
    }

    // send connection command
    char command[] = "command";
    char response[1024];
    printf("Sending command: %s\n", command);
    if (telloc_send_command(connection, command, (unsigned int) strlen(command), response, 1024) != 0) {
        goto error;
    }

    // initialize the video decoder
    if (telloc_video_decoder_init(&connection->video_decoder) != 0) {
        telloc_video_decoder_free(&connection->video_decoder);
        goto error;
    }

    // initialize the state and video data
    connection->state_mutex = CreateMutex(NULL, FALSE, NULL);
    connection->state_buffer = malloc(TELLOC_STATE_SIZE);
    connection->state_size = 0;

    connection->video_mutex = CreateMutex(NULL, FALSE, NULL);
    connection->video_buffer = malloc(TELLOC_VIDEO_SIZE);
    connection->video_size = 0;

    // start the video, state, and keepalive threads
    connection->state_thread = (HANDLE) _beginthreadex(NULL, 0, &thread_state, connection, 0, NULL);
    connection->video_thread = (HANDLE) _beginthreadex(NULL, 0, &thread_video, connection, 0, NULL);
    connection->keepalive_thread = (HANDLE) _beginthreadex(NULL, 0, &thread_keepalive, connection, 0, NULL);

    return connection;

error:
    // close the sockets
    closesocket(command_sock);
    closesocket(state_sock);
    closesocket(video_sock);

    // free the connection
    free(connection);

    return NULL;
}


// default interface to all 0.0.0.0
telloc_connection *telloc_connect(void) {
    return telloc_connect_interface("0.0.0.0");
}


// function to send a command to the drone
int telloc_send_command(telloc_connection *connection, const char* command, unsigned length, char* response, unsigned int response_length) {
    // check if the command socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; Command not sent.\n");
        printf("Call telloc_connect() before sending commands.\n");
        return 1;
    }

    if(length > 1024) {
        printf("Command too long; Command not sent.\n");
        return 1;
    }

    // acquire the command mutex
    WaitForSingleObject(connection->command_mutex, INFINITE);

    // get the socket from the connection
    SOCKET sock = connection->command_socket;

    // send the command to the drone on the command socket port 8889, ip address 192.168.10.1
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TELLOC_COMMAND_PORT);
    addr.sin_addr.s_addr = inet_addr(TELLOC_ADDRESS);
    int bytes_sent = sendto(sock, command, (int) length, 0, (struct sockaddr *) &addr, sizeof(addr));
    if (bytes_sent == SOCKET_ERROR) {
        printf("Error sending command: %d\n", WSAGetLastError());
        goto error;
    }

    // recieve a response from the drone on port 8889
    char response_buffer[256];
    int bytes_recieved = recvfrom(sock, response_buffer, sizeof(response_buffer), 0, NULL, NULL);
    if (bytes_recieved == SOCKET_ERROR) {
        printf("Response timeout: %d\n", WSAGetLastError());
        goto error;
    }

    // Check if the response is null
    if (response != NULL) {
        // zero out the response string
        memset(response, 0, response_length);
        // copy the response buffer into the response string with strcpy_s
        memcpy_s(response, response_length, response_buffer, bytes_recieved);
    }

    // flush the udp socket
    // this is to prevent the socket from filling up with old data
    char flush[1024];
    while (recvfrom(sock, flush, sizeof(flush), 0, NULL, NULL) != SOCKET_ERROR);

    // release the command mutex
    ReleaseMutex(connection->command_mutex);

    return 0;

error:
    // release the command mutex
    ReleaseMutex(connection->command_mutex);

    return 1;
}


// function to disconnect from the drone
int telloc_disconnect(telloc_connection *connection) {
    // check if the connection is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; Disconnect not completed.\n");
        printf("Call telloc_connect() before disconnecting.\n");
        return 1;
    }

    // set the connection's alive flag to 0 to stop any threads
    connection->alive = 0;

    // WAIT FOR THREADS TO EXIT
    // wait for the state thread to exit
    WaitForSingleObject(connection->state_thread, INFINITE);
    CloseHandle(connection->state_thread);
    // wait for the video thread to exit
    WaitForSingleObject(connection->video_thread, INFINITE);
    CloseHandle(connection->video_thread);
    // wait for the keepalive thread to exit
    WaitForSingleObject(connection->keepalive_thread, INFINITE);
    CloseHandle(connection->keepalive_thread);

    // close the sockets
    closesocket(connection->command_socket);
    closesocket(connection->state_socket);
    closesocket(connection->video_socket);

    // free the state and video buffers
    free(connection->state_buffer);
    free(connection->video_buffer);

    // close the state and video mutexes
    CloseHandle(connection->state_mutex);
    CloseHandle(connection->video_mutex);

    // close the command mutex
    CloseHandle(connection->command_mutex);

    // unititialize the video decoder
    telloc_video_decoder_free(&connection->video_decoder);

    // cleanup Windows networking
    WSACleanup();

    // free the connection
    free(connection);

    return 0;
}
