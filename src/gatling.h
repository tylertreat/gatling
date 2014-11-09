#ifndef gatling_h
#define gatling_h

// A message published to a topic.
typedef struct msg_t
{
    unsigned int topic_size;
    char*        topic;
    unsigned int body_size;
    char*        body;
} msg_t;


// Frame is an atomic message, consisting of a protocol, size, and body.
typedef struct frame_t
{
    unsigned short proto;
    unsigned int   size;
    char*          body;
} frame_t;

// Frees the frame resources and zeroes out the pointer.
void frame_dispose(frame_t* frame);

// Frees the message resources and zeroes out the pointer.
void msg_dispose(msg_t* msg);

// Serializes the message into protocol form for sending over the wire. Returns
// the size of the serialized message.
size_t msg_buffer(msg_t* msg);

#endif
