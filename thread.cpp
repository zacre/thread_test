#include <cstdio>
#include <cstdint>
#include <ctime>
#include <pthread.h>

#include "threadhelper.hpp"
#include "thread.hpp"

#define MUTEX_RETRIES 

uint8_t counter_init = 3;

const struct timespec millisleep = {0, 1000000}; // 1ms
const struct timespec hundredmillisleep = {0, 100000000}; // 100ms

int AWSAM_thread_run(struct subthread_data **ISOR_thread_data) {
    // Producer
    // Make a 255-counter
    // Each upcount is a "produce"
    // The highest bits of the buffers are the "updated" flag (i.e. (&ISOR_thread_data)[0]->main_output_buffer & 0xFF000000)
    pthread_mutex_t *AWSAM_mutex_in = ISOR_thread_data[0]->main_input_mutex;
    pthread_mutex_t *AWSAM_mutex_out = ISOR_thread_data[0]->main_output_mutex;
    uint32_t *AWSAM_buffer_in = ISOR_thread_data[0]->main_input_buffer;
    uint32_t *AWSAM_buffer_out = ISOR_thread_data[0]->main_output_buffer;
    
    uint8_t counter = counter_init;
    int res = 0;
    int mut_lock = 0;
    uint32_t running = 0;

    // Get output mutex
    mut_lock = pthread_mutex_trylock(AWSAM_mutex_out);
    if (mut_lock != 0) {
        // Error getting mutex lock
        printf("Error getting AWSAM output mutex lock\r\n");
    }
    // Write 0xFF000000 | counter to main output buffer
    *AWSAM_buffer_out = 0xFF000000 | counter;
    res = pthread_mutex_unlock(AWSAM_mutex_out);
    if (res != 0) {
        printf("Error releasing AWSAM output mutex lock\r\n");
    }

    while (running != 0xFFFFFFFF) {
        // Delay
        nanosleep(&millisleep, NULL);
        // Get AWSAM input mutex
        mut_lock = pthread_mutex_trylock(AWSAM_mutex_in);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting AWSAM input mutex lock\r\n");
            continue;
        }
        // Check value of main input buffer
        running = *AWSAM_buffer_in;
        // Clear flag
        *AWSAM_buffer_in = *AWSAM_buffer_in & 0x00FFFFFF;
        // Release input mutex
        res = pthread_mutex_unlock(AWSAM_mutex_in);
        if (res != 0) {
            printf("Error releasing AWSAM input mutex lock\r\n");
        }

        if (running & 0xFF000000 == 0) {
            // ISOR thread hasn't received input yet
            continue;
        }
        // else:

        // Get AWSAM output mutex
        mut_lock = pthread_mutex_trylock(AWSAM_mutex_out);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting AWSAM output mutex lock\r\n");
            continue;
        }
        // Increment counter
        counter++;
        // Write 0xFF000000 | counter to main output buffer
        *AWSAM_buffer_out = 0xFF000000 | counter;
        // Release AWSAM output mutex
        res = pthread_mutex_unlock(AWSAM_mutex_out);
        if (res != 0) {
            printf("Error releasing AWSAM output mutex lock\r\n");
        }
    }
    printf("AWSAM thread terminating\r\n");
    while (running) {
        // Get AWSAM output mutex
        mut_lock = pthread_mutex_trylock(AWSAM_mutex_out);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting AWSAM output mutex lock\r\n");
            continue;
        }
        // Write 0xFFFFFFFF to terminate other thread
        *AWSAM_buffer_out = 0xFFFFFFFF;
        running = 0;
        // Release AWSAM output mutex
        res = pthread_mutex_unlock(AWSAM_mutex_out);
        if (res != 0) {
            printf("Error releasing AWSAM output mutex lock\r\n");
        }
    }
    printf("AWSAM thread termination successful\r\n");
}

int ISOR_thread_run(struct subthread_data **ISOR_thread_data) {
    // Consumer thread
    pthread_mutex_t *AWSAM_mutex_in = ISOR_thread_data[0]->main_input_mutex;
    pthread_mutex_t *AWSAM_mutex_out = ISOR_thread_data[0]->main_output_mutex;
    uint32_t *AWSAM_buffer_in = ISOR_thread_data[0]->main_input_buffer;
    uint32_t *AWSAM_buffer_out = ISOR_thread_data[0]->main_output_buffer;

    uint8_t counter = counter_init;
    int res = 0;
    int mut_lock = 0;
    uint32_t running = 0;
    uint8_t error = 0;

    while (running != 0xFFFFFFFF) {
        // Delay
        nanosleep(&millisleep, NULL);
        // Get AWSAM output mutex
        mut_lock = pthread_mutex_trylock(AWSAM_mutex_out);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting ISOR input mutex lock\r\n");
            continue;
        }
        // Check value of main input buffer
        running = *AWSAM_buffer_out;
        // Clear flag
        *AWSAM_buffer_out = *AWSAM_buffer_out & 0x00FFFFFF;
        // Release AWSAM output mutex
        res = pthread_mutex_unlock(AWSAM_mutex_out);
        if (res != 0) {
            printf("Error releasing ISOR input mutex lock\r\n");
        }

        if (running & 0xFF000000 == 0) {
            // AWSAM thread hasn't sent data yet
            continue;
        } else if (running == 0xFFFFFFFF) {
            // ISOR thread will terminate
            continue;
        }
        // else:
        running = running & 0x00FFFFFF;

        // Get AWSAM input mutex
        mut_lock = pthread_mutex_trylock(AWSAM_mutex_in);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting ISOR output mutex lock\r\n");
            continue;
        }
        // Increment internal counter
        counter++;
        if (running == 63) {
            // Terminate thread
            *AWSAM_buffer_in = 0xFFFFFFFF;
        } else {
            if (running == counter) {
                // Success, counter == counter
                error = 0;
            } else {
                // Error; counter desync
                error = 1;
            }
            *AWSAM_buffer_in = 0xFF000000 | error;
        }
        // Release AWSAM input mutex
        res = pthread_mutex_unlock(AWSAM_mutex_in);
        if (res != 0) {
            printf("Error releasing ISOR output mutex lock\r\n");
        }
        if (error) {
            printf("Oh, no, error: internal count %d != received %d\r\n", counter, running);
        } else {
            printf("Success, internal %d == %d\r\n", counter, running);
        }
    } // End main loop
    printf("ISOR thread termination successful\r\n");
}

int main(void) {
    int res = 0;
    int mut_lock = 0;
    bool AWSAMrunning = true;
    bool ISORrunning = true;

    // Create NID buffers
    volatile uint32_t AWSAM_buffer_in = 0;
    volatile uint32_t AWSAM_buffer_out = 0;

    // Create NID mutexes
    printf("Initialising mutex locks\r\n");
    pthread_mutex_t AWSAM_mutex_in;
    pthread_mutex_t AWSAM_mutex_out;
    res = pthread_mutex_init(&AWSAM_mutex_in, NULL);
    if (res != 0) {
        printf("Error initialising AWSAM in mutex\r\n");
        return -1;
    }
    res = pthread_mutex_init(&AWSAM_mutex_out, NULL);
    if (res != 0) {
        printf("Error initialising AWSAM out mutex\r\n");
        return -1;
    }

    // Create NID data structures
    struct subthread_data AWSAM_thread_data;
    AWSAM_thread_data.main_input_buffer = (uint32_t *)&AWSAM_buffer_in;
    AWSAM_thread_data.main_output_buffer = (uint32_t *)&AWSAM_buffer_out;
    AWSAM_thread_data.main_input_mutex = &AWSAM_mutex_in;
    AWSAM_thread_data.main_output_mutex = &AWSAM_mutex_out;

    // Create NID threads
    pthread_t AWSAM_thread;
    printf("Creating AWSAM thread\r\n");
    void *(*AWSAM_thread_function)(void *) = (void *(*)(void *))&AWSAM_thread_run;
    res = pthread_create(&AWSAM_thread, NULL, AWSAM_thread_function, &AWSAM_thread_data);
    if (res != 0) {
        printf("Error creating AWSAM thread\r\n");
        return -1;
    }

    // Create main thread data structure
    struct subthread_data *ISOR_thread_data[1];
    ISOR_thread_data[0] = &AWSAM_thread_data;

    // Create main thread
    pthread_t ISOR_thread;
    printf("Creating ISOR thread\r\n");
    void *(*ISOR_thread_function)(void *) = (void *(*)(void *))&ISOR_thread_run;
    res = pthread_create(&ISOR_thread, NULL, ISOR_thread_function, &ISOR_thread_data);
    if (res != 0) {
        printf("Error creating ISOR thread\r\n");
        return -1;
    }

    ///////TODO: check whether both threads are still running
    while(AWSAMrunning && ISORrunning) {
        nanosleep(&hundredmillisleep, NULL);

        // Get AWSAM output mutex
        mut_lock = pthread_mutex_trylock(&AWSAM_mutex_out);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting AWSAM output mutex lock\r\n");
            continue;
        }
        // Check value of main input buffer
        if (AWSAM_buffer_out == 0xFFFFFFFF) {
            printf("Detected AWSAM thread terminating\r\n");
            AWSAMrunning = false;
        }
        // Release AWSAM output mutex
        res = pthread_mutex_unlock(&AWSAM_mutex_out);
        if (res != 0) {
            printf("Error releasing ISOR input mutex lock\r\n");
        }

        // Get AWSAM input mutex
        mut_lock = pthread_mutex_trylock(&AWSAM_mutex_in);
        if (mut_lock != 0) {
            // Error getting mutex lock
            printf("Error getting ISOR output mutex lock\r\n");
            continue;
        }
        if (AWSAM_buffer_in == 0xFFFFFFFF) {
            printf("Detected ISOR thread terminating\r\n");
            ISORrunning = false;
        }
        // Release AWSAM input mutex
        res = pthread_mutex_unlock(&AWSAM_mutex_in);
        if (res != 0) {
            printf("Error releasing ISOR output mutex lock\r\n");
        }
    }

    printf("cancelling AWSAM thread\r\n");
    res = pthread_cancel(AWSAM_thread);
    if (res != 0) {
        printf("cancelling AWSAM thread failed\r\n");
    }
    printf("Destroying AWSAM mutex\r\n");
    res = pthread_mutex_destroy(&AWSAM_mutex_in);
    if (res != 0) {
        printf("Destroying AWSAM in mutex failed\r\n");
    }
    res = pthread_mutex_destroy(&AWSAM_mutex_out);
    if (res != 0) {
        printf("Destroying AWSAM out mutex failed\r\n");
    }
    return 0;
}
