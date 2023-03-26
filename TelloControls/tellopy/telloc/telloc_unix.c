// Contains a unix implementation of the telloc library.
//
#include "telloc.h"
#include "video.h"

// include unix libraries for receiving udp data over a network
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// include unix libraries for threading
#include <pthread.h>

// include unix libraries for timing
#include <time.h>
#include <unistd.h>

#include <stdlib.h>
#include <fcntl.h>

// struct to hold the state of the telloc library
struct telloc_connection_ {
    // thread synchronization
    unsigned alive;

    // Tello command data socket
    int command_socket;
    pthread_mutex_t command_mutex;

    // Tello state data
    int state_socket;
    pthread_mutex_t state_mutex;
    char* state_buffer;
    unsigned state_size;

    // Tello video data
    int video_socket;
    pthread_mutex_t video_mutex;
    char* video_buffer;
    unsigned video_size;

    telloc_video_decoder video_decoder;

    // Threads
    pthread_t state_thread;
    pthread_t video_thread;
    pthread_t keepalive_thread;
};


// thread to receive state data from the Tello drone over UDP
void* thread_state(void* arg) {
    printf("State thread started\n");

    // get the connection from the argument
    telloc_connection* connection = (telloc_connection*) arg;

    // check if the state socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; State thread not starting.\n");
        printf("Call telloc_connect() before starting state thread.\n");
        return NULL;
    }

    // get the socket from the connection
    int sock = connection->state_socket;

    // while alive, receive data on the socket
    while (connection->alive) {
        // receive data on the socket, assume UDP max buffer size
        char buffer[TELLOC_STATE_SIZE];

        // receive UDP data from the socket using recvfrom
        int bytes_received = (int) recvfrom(sock, buffer, TELLOC_STATE_SIZE, 0, NULL, NULL);
        if (bytes_received < 0) {
            // sleep for 5 ms
            nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
            continue;
        }

        // Windows acquire handle to the mutex
        pthread_mutex_lock(&connection->state_mutex);

        // copy the data string to the connection's state buffer
        // zero out the state buffer
        memset(connection->state_buffer, 0, TELLOC_STATE_SIZE);
        // copy the data string to the state buffer with memcpy_s
        memcpy(connection->state_buffer, buffer, bytes_received);
        connection->state_size = bytes_received;

        // release the mutex
        pthread_mutex_unlock(&connection->state_mutex);

    }

    // close the socket
    close(sock);

    return NULL;
}


// thread to read the most recent state data
int telloc_read_state(telloc_connection *connection, char* state_buffer, unsigned state_buffer_length) {
    // clear the state buffer
    memset(state_buffer, 0, state_buffer_length);

    // check if the state socket is open
    if (connection == NULL || !connection->alive) {
        printf("Connection not initialized; State thread not starting.\n");
        printf("Call telloc_connect() before starting state thread.\n");
        return 1;
    }

    // check if the buffer is large enough
    if (state_buffer_length < connection->state_size) {
        printf("Buffer size is too small to hold state data.\n");
        return 1;
    }

    // check if there is state data
    if (connection->state_size == 0) {
        return 1;
    }

    // acquire handle to the mutex
    pthread_mutex_lock(&connection->state_mutex);

    // copy the data string to the state buffer with memcpy
    memcpy(state_buffer, connection->state_buffer, connection->state_size);
    connection->state_size = 0;

    // release the mutex
    pthread_mutex_unlock(&connection->state_mutex);

    return 0;
}


// thread to receive video data from the Tello drone over UDP
void* thread_video(void* arg) {
    printf("Video thread started\n");

    // get the connection from the argument
    telloc_connection *connection = (telloc_connection *) arg;

    // check if the video socket is open
    if (!connection->alive) {
        printf("Connection not initialized; Video thread not starting.\n");
        printf("Call telloc_connect() before starting video thread.\n");
        return NULL;
    }

    // get the socket from the connection
    int sock = connection->video_socket;

    // allocate a buffer for the video data
    unsigned char *udp_buffer = (unsigned char *) malloc(65507);

    // allocate a buffer for multiple video packets
    unsigned char *h264_buffer = (unsigned char *) malloc(65507 * 10);
    unsigned h264_buffer_size = 0;

    // while alive, receive data on the socket
    while (connection->alive) {
        // receive data on the socket on the desired interface
        int bytes_received = (int) recvfrom(sock, udp_buffer, 65507, 0, NULL, NULL);
        if (bytes_received == -1) {
            // sleep for 1 ms
            nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);
            continue;
        }

        // check if the udp packet is a valid h264 start frame (end of previous frame)
        int new_frame = telloc_video_decoder_is_start_code(udp_buffer, bytes_received);
        if (new_frame) {
            // if there is a (hopefully) completed frame in the h264 buffer, send it to the video decoder
            if (h264_buffer_size > 0) {
                // send the completed video packet to the video decoder
                telloc_video_decoder_decode(&connection->video_decoder, h264_buffer, h264_buffer_size);
            }
            // copy the udp packet to the h264 buffer
            memcpy(h264_buffer, udp_buffer, bytes_received);
            h264_buffer_size = bytes_received;
        } else if (h264_buffer_size > 0) {  // not a start frame, but there is some data in the h264 buffer
            //  copy the data to the end of the h264 buffer
            memcpy(h264_buffer + h264_buffer_size, udp_buffer, bytes_received);
            h264_buffer_size += bytes_received;
        }

        // check if the video decoder has a completed frame
        if (new_frame && connection->video_decoder.frame_ready) {
            // acquire handle to the mutex
            pthread_mutex_lock(&connection->video_mutex);

            // copy the data to the connection's video buffer
            memcpy(connection->video_buffer, connection->video_decoder.frame_buffer, connection->video_decoder.frame_size);
            connection->video_decoder.frame_ready = 0;
            connection->video_size = connection->video_decoder.frame_size;

            // release the mutex
            pthread_mutex_unlock(&connection->video_mutex);
        }
    }

    // close the socket
    close(sock);

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

    // acquire the mutex unix
    pthread_mutex_lock(&connection->video_mutex);

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
    pthread_mutex_unlock(&connection->video_mutex);

    return 0;

    error:
    // release the mutex
    pthread_mutex_unlock(&connection->video_mutex);
    return 1;
}


// thread to repeatedly send keepalive (tello connection will timeout after 15 seconds).
// argument: telloc_connection *connection
void* thread_keepalive(void* arg) {
    printf("Keepalive thread started\n");

    // get the connection from the argument
    telloc_connection *connection = (telloc_connection *) arg;

    // check if the command socket is open
    if (!connection->alive) {
        printf("Connection not initialized; Keepalive thread not starting.\n");
        printf("Call telloc_connect() before starting keepalive thread.\n");
        return NULL;
    }

    // get the socket from the connection
    int sock = connection->command_socket;

    char buffer[1024];
    char const* query = "battery?";

    // get current time using time.h
    time_t last_keepalive_time = time(NULL);

    // while alive, send keepalive
    while (connection->alive) {
        // send the keepalive command
        // if time since last keepalive command is greater than 10 seconds, send the keepalive command
        if (time(NULL) - last_keepalive_time > 10) {
            telloc_send_command(connection, query, (unsigned int) strlen(query), buffer, sizeof(buffer));
            last_keepalive_time = time(NULL);
        }
        // sleep for 5 ms
        nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
    }

    close(sock);

    return NULL;
}


// function to bind a UDP socket to an address and port
int telloc_bind_udp_socket(int *sock, const char* address, unsigned short port) {
    // create a UNIX UDP socket
    *sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (*sock == -1) {
        printf("Error creating socket: %d\n" , errno);
        return 1;
    }

    // set the socket timeout to 1 second
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        printf("Error setting socket timeout: %d\n", errno);
        return 1;
    }

    // bind the UNIX socket to listen to an address and port
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address);
    if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("Error binding socket: %d\n", errno);
        return 1;
    }

    return 0;
}


// function to connect to the Tello drone on a specified interface address
telloc_connection * telloc_connect_interface(const char *interface_address) {
    printf("Connecting to Tello on interface %s\n", interface_address);

    // allocate a connection pointer
    telloc_connection *connection = malloc(sizeof(telloc_connection));
    if (connection == NULL) {
        printf("Error allocating connection pointer memory\n");
        return NULL;
    }

    // set the connection's alive flag to 0 to stop any threads
    connection->alive = 0;

    // create sockets
    int command_sock = 0;
    int state_sock = 0;
    int video_sock = 0;

    // create the command mutex (only one command can be sent at a time)
    connection->command_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
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
    // set the socket timeout to TELLOC_RESPONSE_TIMEOUT milliseconds
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TELLOC_RESPONSE_TIMEOUT * 1000;
    if (setsockopt(command_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        printf("Error setting socket timeout\n");
        goto error;
    }

    // send connection command
    char command[] = "command";
    char response[1024];
    printf("Sending command: %s\n", command);
    if (telloc_send_command(connection, command, (unsigned int) strlen(command), response, 1024) != 0) {
        goto error;
    }
    printf("Response: %s\n", response);

    // initialize the video decoder
    if (telloc_video_decoder_init(&connection->video_decoder) != 0) {
        telloc_video_decoder_free(&connection->video_decoder);
        goto error;
    }

    // initialize the state and video data
    connection->state_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    connection->state_buffer = malloc(TELLOC_STATE_SIZE);
    connection->state_size = 0;

    connection->video_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    connection->video_buffer = malloc(TELLOC_VIDEO_SIZE);
    connection->video_size = 0;

    // start the video, state, and keepalive threads using unix threading functionality
    pthread_create(&connection->video_thread, NULL, thread_video, connection);
    pthread_create(&connection->state_thread, NULL, thread_state, connection);
    pthread_create(&connection->keepalive_thread, NULL, thread_keepalive, connection);

    return connection;

error:
    // close the sockets
    close(command_sock);
    close(state_sock);
    close(video_sock);

    // free the connection
    free(connection);

    // reset the connection pointer
    return NULL;
}


// function to connect to the Tello drone
telloc_connection * telloc_connect(void) {
    return telloc_connect_interface("0.0.0.0");
}


// function to send a command to the drone
int telloc_send_command(telloc_connection *connection, const char* command, unsigned int length, char* response, unsigned int response_length) {
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
    pthread_mutex_lock(&connection->command_mutex);

    // get the socket from the connection
    int sock = connection->command_socket;

    // send the command to the drone on the command socket port 8889, ip address 192.168.10.1
    // uses UNIX posx api functions for sending data over UDP
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TELLOC_COMMAND_PORT);
    addr.sin_addr.s_addr = inet_addr(TELLOC_ADDRESS);
    int bytes_sent = (int) sendto(sock, command, length, 0, (struct sockaddr*) &addr, sizeof(addr));
    if (bytes_sent == -1) {
        printf("Command not sent: %d\n", errno);
        goto error;
    }

    // recieve a response from the drone on port 8889
    char response_buffer[256];
    int bytes_recieved = (int) recvfrom(sock, response_buffer, sizeof(response_buffer), 0, NULL, NULL);
    if (bytes_recieved == -1) {
        printf("Response timeout: %d\n", errno);
        goto error;
    }

    // Check if the response is null
    if (response != NULL) {
        // zero out the response string
        memset(response, 0, response_length);
        // copy the response buffer into the response string with memcpy
        memcpy(response, response_buffer, bytes_recieved);
    }

    // flush the udp socket
    // this is to prevent the socket from filling up with old data
    char flush[1024];
    while (recvfrom(sock, flush, sizeof(flush), 0, NULL, NULL) > 0);

    // release the command mutex
    pthread_mutex_unlock(&connection->command_mutex);

    return 0;

error:
    // release the command mutex
    pthread_mutex_unlock(&connection->command_mutex);

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

    // WAIT FOR THREADS TO EXIT; use pthread_join() for unix
    pthread_join(connection->video_thread, NULL);
    pthread_join(connection->state_thread, NULL);
    pthread_join(connection->keepalive_thread, NULL);

    // close the sockets
    close(connection->command_socket);
    close(connection->state_socket);
    close(connection->video_socket);

    // free the state and video buffers
    free(connection->state_buffer);
    free(connection->video_buffer);

    // close the state and video mutexes
    pthread_mutex_destroy(&connection->state_mutex);
    pthread_mutex_destroy(&connection->video_mutex);
    pthread_mutex_destroy(&connection->command_mutex);

    // unititialize the video decoder
    telloc_video_decoder_free(&connection->video_decoder);

    // free the connection
    free(connection);

    return 0;
}
