#pragma once

#include <cstdint>
#include <pthread.h>

struct subthread_data {
    uint32_t *main_input_buffer;
    uint32_t *main_output_buffer;
    pthread_mutex_t *main_input_mutex;
    pthread_mutex_t *main_output_mutex;
};
