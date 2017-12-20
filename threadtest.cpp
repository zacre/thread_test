#include <pthread.h>

#include "threadhelper.h"

int main(void) {
    // main constructs the other 2 threads and all the shared data structures (buffers and mutexes)
}

int producer(struct subthread_data **ISOR_thread_data) {
    // producer is run as a thread, and produces the data to send to the consumer, as per the readme
}

int consumer(struct subthread_data **ISOR_thread_data) {
    // consumer is run as a thread, and consumes the data sent to it by the producer thread
}