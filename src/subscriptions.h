#ifndef subscriptions_h
#define subscriptions_h

// Initializes subscriptions structure. Returns 0 on success, -1 on failure.
int subscriptions_init();

// Frees subscriptions resources.
void subscriptions_dispose();

// Registers a topic subscription for a peer. Returns 0 on success, -1 on
// failure.
int subscribe(char* topic, int fd);

// Unregisters a topic subscription for a peer. Returns 0 on success, -1 on
// failure.
int unsubscribe(char* topic, int fd);

// Publishes the given message to all peers subscribing to the message topic.
// Returns -1 on failure, 0 on success.
int subscriber_publish(msg_t* msg);

#endif
