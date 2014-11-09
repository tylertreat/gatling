#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "gatling.h"
#include "protocol.h"


// Parses a single message-frame body. This reads the body size (4 bytes)
// followed by the body itself (n bytes), in network-byte order. If the frame
// body is not of this form, -1 is returned. Otherwise 0 is returned.
int parse_frame_body(int fd, frame_t* frame)
{
    int len = read(fd, &frame->size, 4);
    if (len != 4)
    {
        printf("failed to read frame size\n");
        return -1;
    }

    frame->size = ntohl(frame->size);

    frame->body = malloc(frame->size);
    if (!frame->body)
    {
        errno = ENOMEM;
        return -1;
    }

    len = recv(fd, frame->body, frame->size, 0);
    if (len != frame->size)
    {
        printf("failed to read frame body\n");
        free(frame->body);
        return -1;
    }

    return 0;
}

// Parses a single message frame. A frame is of the form protocol (2 bytes),
// body size (4 bytes), body (body-size bytes), in network-byte order. If the
// frame isn't in this form, it's invalid and -1 is returned. Otherwise 0 is
// returned.
int parse_frame(int fd, frame_t* frame)
{
    unsigned short proto;
    int len = read(fd, &proto, 2);
    if (len != 2)
    {
        return -1;
    }

    frame->proto = ntohs(proto);
    return parse_frame_body(fd, frame);
}

// Parses a message-frame body as a publish and populate the provided struct.
// Publish messages consist of the topic length (4 bytes) followed by the topic
// and message, in network-byte order. If the body is not of this form, -1 is
// returned. Otherwise 0 is returned.
int parse_frame_publish(const char* m, const unsigned int size, msg_t* msg)
{
    if (size < 4)
    {
        perror("Invalid publish frame");
        return -1;
    }

    msg->topic_size = (m[0] << 24) + (m[1] << 16) + (m[2] << 8) + m[3];

    if (size < 4 + msg->topic_size)
    {
        perror("Invalid publish frame");
        return -1;
    }

    msg->topic = malloc(msg->topic_size);
    strncpy(msg->topic, &m[4], msg->topic_size);

    msg->body_size = size - msg->topic_size - 4;
    msg->body = malloc(msg->body_size);
    strncpy(msg->body, &m[4 + msg->topic_size], msg->body_size);

    return 0;
}

// Serializes the message into a protocol frame for sending over the wire.
// Returns the size of the serialized message or 0 if the message couldn't be
// serialized. Caller is responsible for freeing the allocated buffer.
char* frame_buffer(const msg_t* msg, size_t* size)
{
    // 2 bytes for protocol, 4 for frame size, 4 for topic size, m for topic,
    // and n for message.
    *size = 10 + msg->topic_size + msg->body_size;

    char* buf = malloc(*size);
    if (!buf)
    {
        perror("Failed to allocate protocol frame");
        return 0;
    }

    unsigned int frame_size = 4 + msg->topic_size + msg->body_size;

    // First 2 bytes are protocol.
    buf[0] = (GAT_PUB >> 8) & 0xff;
    buf[1] = GAT_PUB & 0xff;

    // Next 4 bytes are frame size.
    buf[2] = (frame_size >> 24) & 0xFF;
    buf[3] = (frame_size >> 16) & 0xFF;
    buf[4] = (frame_size >> 8) & 0xFF;
    buf[5] = frame_size & 0xFF;

    // Next 4 bytes are topic size.
    buf[6] = (msg->topic_size >> 24) & 0xFF;
    buf[7] = (msg->topic_size >> 16) & 0xFF;
    buf[8] = (msg->topic_size >> 8) & 0xFF;
    buf[9] = msg->topic_size & 0xFF;

    // Next m bytes are topic.
    int i;
    for (i = 0; i < msg->topic_size; i++)
    {
        buf[i + 10] = msg->topic[i];
    }

    // Next n bytes are message.
    for (i = 0; i < msg->body_size; i++)
    {
        buf[i + 10 + msg->topic_size] = msg->body[i];
    }
    {
    }

    return buf;
}

