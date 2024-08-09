#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#define BUFFER_SIZE 10
#define NUM_ITEMS 20

int buffer1[BUFFER_SIZE];
int buffer1_count = 0;
int buffer1_in = 0;
int buffer1_out = 0;
pthread_mutex_t buffer1_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t buffer1_full;
sem_t buffer1_empty;

int buffer2[BUFFER_SIZE];
int buffer2_count = 0;
int buffer2_in = 0;
int buffer2_out = 0;
pthread_mutex_t buffer2_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t buffer2_full;
sem_t buffer2_empty;

void produce(int* buffer, int* buffer_count, int* buffer_in, pthread_mutex_t* buffer_mutex, sem_t* buffer_full, sem_t* buffer_empty, int item) {
    sem_wait(buffer_empty);
    pthread_mutex_lock(buffer_mutex);

    buffer[*buffer_in] = item;
    *buffer_in = (*buffer_in + 1) % BUFFER_SIZE;
    (*buffer_count)++;

    pthread_mutex_unlock(buffer_mutex);
    sem_post(buffer_full);
}

int consume(int* buffer, int* buffer_count, int* buffer_out, pthread_mutex_t* buffer_mutex, sem_t* buffer_full, sem_t* buffer_empty) {
    sem_wait(buffer_full);
    pthread_mutex_lock(buffer_mutex);

    int item = buffer[*buffer_out];
    *buffer_out = (*buffer_out + 1) % BUFFER_SIZE;
    (*buffer_count)--;

    pthread_mutex_unlock(buffer_mutex);
    sem_post(buffer_empty);
    return item;
}

void* producer(void* arg) {
    int producer_id = (int)arg;
    for (int i = 0; i < NUM_ITEMS; i++) {
        int item = producer_id * 100 + i;
        produce(buffer1, &buffer1_count, &buffer1_in, &buffer1_mutex, &buffer1_full, &buffer1_empty, item);
        printf("Producer ID: %d, Produced: %d into Buffer 1\n", producer_id, item);
        usleep((rand() % 500) * 1000);
    }
    return NULL;
}

void* first_level_consumer_producer(void* arg) {
    int consumer_producer_id = (int)arg;
    for (int i = 0; i < NUM_ITEMS; i++) {
        int item = consume(buffer1, &buffer1_count, &buffer1_out, &buffer1_mutex, &buffer1_full, &buffer1_empty);
        printf("Consumer ID: %d , Consumed: %d from Buffer 1\n", consumer_producer_id, item);
        int new_item = item * 10 + consumer_producer_id;
        produce(buffer2, &buffer2_count, &buffer2_in, &buffer2_mutex, &buffer2_full, &buffer2_empty, new_item);
        printf("Producer ID: %d , Produced: %d into Buffer 2\n", consumer_producer_id, new_item);
        usleep((rand() % 500) * 1000);
    }
    return NULL;
}

void* second_level_consumer(void* arg) {
    int consumer_id = (int)arg;
    for (int i = 0; i < NUM_ITEMS; i++) {
        int item = consume(buffer2, &buffer2_count, &buffer2_out, &buffer2_mutex, &buffer2_full, &buffer2_empty);
        printf("Consumer ID: %d , Consumed: %d from Buffer 2\n", consumer_id, item);
        usleep((rand() % 500) * 1000);
    }
    return NULL;
}

int main() {
    srand(time(NULL));

    // Declare threads for all types of producers and consumers
    // We need two threads for each type, for that we create arrays of two threads
    pthread_t producers[2];
    pthread_t consumer_producers[2];
    pthread_t consumers[2];

    // Initialize semaphores
    // We need two buffers, each has two semaphores, one for full slots and one for empty slots
    sem_init(&buffer1_full, 0, 0);
    sem_init(&buffer1_empty, 0, BUFFER_SIZE);
    sem_init(&buffer2_full, 0, 0);
    sem_init(&buffer2_empty, 0, BUFFER_SIZE);

    // Create threads
    for (int i = 0; i < 2; i++) { // For loop to create two threads for each type
        pthread_create(&producers[i], NULL, producer, (void*)(i + 1)); // Calling producer and give IDs 1 and 2
        pthread_create(&consumer_producers[i], NULL, first_level_consumer_producer, (void*)(i + 3)); // IDs are 3 and 4
        pthread_create(&consumers[i], NULL, second_level_consumer, (void*)(i + 5)); // IDs are 5 and 6
    }

    // Join threads
    for (int i = 0; i < 2; i++) { // For loop to join all the threads that were created
        pthread_join(producers[i], NULL); // Using pthread_join with arguments, of the thread and NULL to ignore exit status
        pthread_join(consumer_producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&buffer1_full); // Using sem_destroy and sending each of the semaphores as argument to destroy them
    sem_destroy(&buffer1_empty);
    sem_destroy(&buffer2_full);
    sem_destroy(&buffer2_empty);

    printf("All producers and consumers have finished.\n");

    return 0;
}
