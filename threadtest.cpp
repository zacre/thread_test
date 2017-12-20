#include <cstdint>
#include <cstdio>
#include <ctime>
#include <pthread.h>

#include "threadhelper.hpp"

void *producer_fn(void *);
void *consumer_fn(void *);

const struct timespec hundredmillisleep = {0, 100000000}; // 100ms

int main(void) {
    // result var for error-checking for things that may fail
    int res = 0;

    // main constructs the other 2 threads and all the shared data structures (buffers and mutexes)
    pthread_mutex_t p_c_mutex;
    pthread_mutex_t c_p_mutex;
    uint32_t p_c_buffer = 0;
    uint32_t c_p_buffer = 0;
    pthread_t producer_thread;
    pthread_t consumer_thread;

    // init mutex
    printf("Initialising mutexes\n");
    res = pthread_mutex_init(&p_c_mutex, NULL);
    if (res != 0) {
        printf("Error initialising P->C mutex\r\n");
        return -1;
    }
    res = pthread_mutex_init(&c_p_mutex, NULL);
    if (res != 0) {
        printf("Error initialising C->P mutex\r\n");
        return -1;
    }

    struct subthread_data threaddata;
    threaddata.main_input_buffer = &p_c_buffer;
    threaddata.main_input_mutex = &p_c_mutex;
    threaddata.main_output_buffer = &c_p_buffer;
    threaddata.main_output_mutex = &c_p_mutex;

    // create threads
    printf("Creating threads\n");
    res = pthread_create(&producer_thread, NULL, producer_fn, &threaddata);
    if (res != 0) {
        printf("Error creating Producer thread\r\n");
        return -1;
    }
    res = pthread_create(&consumer_thread, NULL, consumer_fn, &threaddata);
    if (res != 0) {
        printf("Error creating Consumer thread\r\n");
        return -1;
    }

    // wait for threads to terminate
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    printf("Show's over\n");
    return 0;
}

void *producer_fn(void *thread_data) {
    // producer is run as a thread, and produces the data to send to the consumer, as per the readme
    int res = 0;
    int running = 1;
    int produced = 0;
    struct subthread_data *threaddata = (struct subthread_data *)thread_data;
    uint8_t counter = 33;

    while(running) {
        while(!produced) {
            // get mutex
            res = pthread_mutex_trylock(threaddata->main_input_mutex);
            if (res != 0) {
                // Error getting mutex lock
                printf("P: Error getting mutex lock\n");
                continue;
            }
            // put data in buffer
            *(threaddata->main_input_buffer) = counter;
            produced = 1;
            // release mutex
            res = pthread_mutex_unlock(threaddata->main_input_mutex);
            if (res != 0) {
                printf("P: Error releasing mutex lock\r\n");
            }
        }
        counter++;
        produced = 0;
    }
    // return
    return NULL;
}

void *consumer_fn(void *thread_data) {
    // consumer is run as a thread, and consumes the data sent to it by the producer thread
    int res = 0;
    int running = 1;
    int consumed = 0;
    struct subthread_data *threaddata = (struct subthread_data *)thread_data;
    uint32_t localbuffer = 0;

    while(running) {
        while(!consumed) {
            // get mutex
            res = pthread_mutex_trylock(threaddata->main_input_mutex);
            if (res != 0) {
                // Error getting mutex lock
                printf("C: Error getting mutex lock\n");
                continue;
            }
            // check for data in buffer
            // if data is in buffer, copy locally
            if (*(threaddata->main_input_buffer) != 0) {
                localbuffer = *(threaddata->main_input_buffer);
                // TODO: Uncomment this to wait until fresh data is available. Currently not helpful because P is producing as fast as possible - no feedback from C.
                //*(threaddata->main_input_buffer) = 0;
                consumed = 1;
            }
            // release mutex
            res = pthread_mutex_unlock(threaddata->main_input_mutex);
            if (res != 0) {
                printf("C: Error releasing mutex lock\r\n");
            }
        }
        // if data was copied locally, print data
        printf("Data was %d\n", localbuffer);
        consumed = 0;
    }
    // return
    return NULL;
}