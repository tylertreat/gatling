#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "hashmap.h"
#include "protocol.h"
#include "gatling.h"
#include "list.h"


map_t subscriptions;
pthread_mutex_t mu;


// Initializes subscriptions structure. Returns -1 on failure, 0 on success.
int subscriptions_init()
{
    subscriptions = hashmap_new();
    if (!subscriptions)
    {
        perror("Failed to allocate subscriptions");
        return -1;
    }

    if (pthread_mutex_init(&mu, NULL) != 0)
    {
        perror("Failed to initialize mutex");
        return -1;
    }

    return 0;
}

// Frees subscriptions resources.
void subscriptions_dispose()
{
    hashmap_free(subscriptions);
    pthread_mutex_destroy(&mu);
}

// Registers a topic subscription for a peer. Returns -1 on failure, 0 on
// success.
int subscribe(char* topic, int fd)
{
    map_t* subscribers;
    pthread_mutex_lock(&mu);
    if (hashmap_get(subscriptions, topic, (void**) &subscribers) != MAP_OK)
    {
        subscribers = hashmap_new();
        if (!subscribers)
        {
            pthread_mutex_unlock(&mu);
            return -1;
        }

        char* key = malloc(strlen(topic) + 1);
        strcpy(key, topic);
        hashmap_put(subscriptions, key, subscribers);
    }

    int err = 0;
    char* fd_str = malloc(sizeof(char) * 10);
    sprintf(fd_str, "%d", fd);
    if (hashmap_put(subscribers, fd_str, NULL) != MAP_OK)
    {
        err = -1;
    }

    pthread_mutex_unlock(&mu);
    return err;
}

// Unregisters a topic subscription for a peer. Returns -1 on failure, 0 on
// success.
int unsubscribe(char* topic, int fd)
{
    int err = 0;
    char fd_str[10];
    sprintf(fd_str, "%d", fd);
    map_t* subscribers;

    pthread_mutex_lock(&mu);
    if (hashmap_get(subscriptions, topic, (void**) &subscribers) == MAP_OK)
    {
        hashmap_remove(subscribers, fd_str);
    }
    else
    {
        err = -1;
    }

    pthread_mutex_unlock(&mu);
    return err;
}

// Publishes the given message to all peers subscribing to the message topic.
// Returns -1 on failure, 0 on success.
int subscriber_publish(msg_t* msg)
{
    map_t* subs;
    pthread_mutex_lock(&mu);
    if (hashmap_get(subscriptions, msg->topic, (void**) &subs) == MAP_OK)
    {
        char** keys = malloc(sizeof(char*) * hashmap_length(subs));
        pthread_mutex_unlock(&mu);
        int length = hashmap_keys(subs, keys);
        if (length > 0)
        {
            int i;
            size_t size;
            char* frame = frame_buffer(msg, &size);
            if (size == 0)
            {
                return -1;
            }

            int err = 0;
            for (i = 0; i < length; i++)
            {
                if (send(atoi(keys[i]), frame, size, 0) == -1)
                {
                    err = -1;
                }
            }
            free(frame);
            return err;
        }
    }
    pthread_mutex_unlock(&mu);

    return 0;
}

