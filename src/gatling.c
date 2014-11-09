#include <arpa/inet.h>
#include <pthread.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chan.h"
#include "gatling.h"
#include "protocol.h"
#include "subscriptions.h"

#define GAT_PORT 9999


chan_t* pub_chan;


// Frees the frame resources and zeroes out the pointer.
void frame_dispose(frame_t* frame)
{
    free(frame->body);
    free(frame);
    frame = NULL;
}

// Frees the message resources and zeroes out the pointer.
void msg_dispose(msg_t* msg)
{
    free(msg->topic);
    free(msg->body);
    free(msg);
    msg = NULL;
}

// Daemon thread which pulls messages from the channel and publishes them to
// interested subscribers.
void* publish_loop()
{
    while (1)
    {
        msg_t* msg;
        chan_recv(pub_chan, (void*) &msg);
        subscriber_publish(msg);
        msg_dispose(msg);
    }

    return NULL;
}

// Asynchronously publishes a message to subscribers. Returns 0 on success, -1
// on failure.
int publish(frame_t* frame)
{
    msg_t* msg = malloc(sizeof(msg_t));
    if (!msg)
    {
        perror("Failed to allocate message");
        return -1;
    }

    if (parse_frame_publish(frame->body, frame->size, msg) == -1)
    {
        free(msg);
        return -1;
    }

    return chan_send(pub_chan, msg);
}

// Handles a single message frame: subscribe, unsubscribe, or publish. Returns
// 0 on success, -1 on failure.
int frame_dispatch(int fd, frame_t* frame)
{
    switch (frame->proto)
    {
        case GAT_SUB:
            // Subscribe
            return subscribe(frame->body, fd);
        case GAT_USUB:
            // Unsubscribe
            return unsubscribe(frame->body, fd);
        case GAT_PUB:
            // Publish
            return publish(frame);
    }

    perror("Invalid frame protocol");
    return -1;
}

// Begins processing client messages.
void* client_loop(void* fd)
{
    int client_fd = (int) fd;
    frame_t* frame = malloc(sizeof(frame_t));
    if (frame == NULL)
    {
        perror("Failed to allocate frame");
        close(client_fd);
        return NULL;
    }

    while (read_frame(client_fd, frame) == 0)
    {
        frame_dispatch(client_fd, frame);
        frame_dispose(frame);
        frame = malloc(sizeof(frame_t));
        if (frame == NULL)
        {
            perror("Failed to allocate frame");
            close(client_fd);
            return NULL;
        }
    }

    printf("Client disconnected\n");
    free(frame);
    close(client_fd);
    return NULL;
}


// Begins accepting client connections.
void accepter_loop(int sock_fd)
{
    while (1)
    {
        int client_fd;
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);

        // Accept client connections.
        client_fd = accept(sock_fd, (struct sockaddr*) &client_addr,
            (uint *) &addrlen);
        printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        // Start a thread to handle client messages.
        pthread_t th;
        pthread_create(&th, NULL, client_loop, (void *) (intptr_t) client_fd);
    }
}

// Starts the message broker on the given port.
int start(int port)
{
    int sock_fd;
    struct sockaddr_in server;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        return -1;
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_fd, (struct sockaddr*) &server, sizeof(server)) != 0)
    {
        perror("Failed to bind socket");
        return -1;
    }

    if (listen(sock_fd, 20) != 0)
    {
        perror("Failed to listen");
        return -1;
    }

    // Start the publisher thread.
    pthread_t th;
    pthread_create(&th, NULL, publish_loop, NULL);

    printf("Broker started on port %d\n", port);

    // Start processing messages.
    accepter_loop(sock_fd); 

    close(sock_fd);
    return 0;
}

int main(int argc, char** argv)
{
    pub_chan = chan_init(10000); // Probably tweak this...
    if (pub_chan == NULL)
    {
        return -1;
    }

    if (subscriptions_init() != 0)
    {
        return -1;
    }

    int port = GAT_PORT;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    int err = start(port);

    chan_dispose(pub_chan);
    subscriptions_dispose();
    return err;
}

