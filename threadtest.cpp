#include <cstdio>
#include <pthread.h>

#include "threadhelper.hpp"

void *producer_fn(void *);
void *consumer_fn(void *);

int main(void) {
    // result var for error-checking for things that may fail
    int res = 0;

    // main constructs the other 2 threads and all the shared data structures (buffers and mutexes)
    pthread_t producer_thread;
    pthread_t consumer_thread;

    printf("Creating threads\n");
    res = pthread_create(&producer_thread, NULL, producer_fn, NULL);
    if (res != 0) {
        printf("Error creating Producer thread\r\n");
        return -1;
    }

    res = pthread_create(&consumer_thread, NULL, consumer_fn, NULL);
    if (res != 0) {
        printf("Error creating Consumer thread\r\n");
        return -1;
    }

    // wait for P and C to terminate
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    printf("Show's over\n");
    return 0;
}

void *producer_fn(void *thread_data) {
    // producer is run as a thread, and produces the data to send to the consumer, as per the readme
    printf("P\n");
    return NULL;
}

void *consumer_fn(void *thread_data) {
    // consumer is run as a thread, and consumes the data sent to it by the producer thread
    printf("C\n");
    return NULL;
}