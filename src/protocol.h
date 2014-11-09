#ifndef protocol_h
#define protocol_h

#include "gatling.h"

// Protocol definitions
#define GAT_SUB     0
#define GAT_USUB    1
#define GAT_PUB     2


// Parses a single message frame. A frame is of the form protocol (2 bytes),
// body size (4 bytes), body (body-size bytes), in network-byte order. If the
// frame isn't in this form, it's invalid and -1 is returned. Otherwise 0 is
// returned.
int parse_frame(int fd, frame_t* frame);

// Parses a message-frame body as a publish and populate the provided struct.
// Publish messages consist of the topic length (4 bytes) followed by the topic
// and message, in network-byte order. If the body is not of this form, -1 is
// returned. Otherwise 0 is returned.
int parse_frame_publish(const char* m, const unsigned int size, msg_t* msg);

// Serializes the message into a protocol frame for sending over the wire.
// Returns the size of the serialized message or -1 if the message couldn't be
// serialized. Caller is responsible for freeing the allocated buffer.
char* frame_buffer(const msg_t* msg, size_t* size);

#endif
