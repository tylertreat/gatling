#include <arpa/inet.h>
#include <errno.h>
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

#define GAT_PORT    9999


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

void* publish_loop()
{
    while (1)
    {
        msg_t* msg;
        chan_recv(pub_chan, (void*) &msg);
        publish(msg);
        msg_dispose(msg);
    }

    return NULL;
}

// Asynchronously publishes a message.
int msg_publish(msg_t* msg)
{
    return chan_send(pub_chan, msg);
}

// Handles a single message frame: subscribe, unsubscribe, or publish. Return 0
// on success, -1 on failure.
int frame_dispatch(int fd, frame_t* frame)
{
    switch (frame->proto)
    {
        case GAT_SUB:
            // Subscribe
            printf("subscribe: %s\n", frame->body);
            return subscribe(frame->body, fd);
        case GAT_USUB:
            // Unsubscribe
            printf("unsubscribe: %s\n", frame->body);
            return unsubscribe(frame->body, fd);
        case GAT_PUB:
            // Publish
            printf("publish: %s\n", frame->body);
            msg_t* msg = malloc(sizeof(msg_t));
            if (msg == NULL)
            {
                perror("Failed to allocate message");
                return -1;
            }
            if (parse_frame_publish(frame->body, frame->size, msg) == -1)
            {
                free(msg);
                return -1;
            }
            return msg_publish(msg);
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
        errno = ENOMEM;
        close(client_fd);
        return NULL;
    }

    while (parse_frame(client_fd, frame) == 0)
    {
        frame_dispatch(client_fd, frame);
        frame_dispose(frame);
        frame = malloc(sizeof(frame_t));
        if (frame == NULL)
        {
            errno = ENOMEM;
            close(client_fd);
            return NULL;
        }
    }

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
        client_fd = accept(sock_fd, (struct sockaddr*) &client_addr, (uint *) &addrlen);
        printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t th;
        pthread_create(&th, NULL, client_loop, (void *) (intptr_t) client_fd);
    }
}

int start()
{
    int sock_fd;
    struct sockaddr_in server;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(errno);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(GAT_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_fd, (struct sockaddr*) &server, sizeof(server)) != 0)
    {
        perror("Failed to bind socket");
        exit(errno);
    }

    if (listen(sock_fd, 20) != 0)
    {
        perror("Failed to listen");
        exit(errno);
    }

    // Start the publisher thread.
    pthread_t th;
    pthread_create(&th, NULL, publish_loop, NULL);

    // Start processing messages.
    accepter_loop(sock_fd); 

    close(sock_fd);
    return 0;
}

int main()
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

    int err = start();

    chan_dispose(pub_chan);
    subscriptions_dispose();
    return err;
}

