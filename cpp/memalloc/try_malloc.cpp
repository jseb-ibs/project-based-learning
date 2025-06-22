#include <iostream>
#include <unistd.h> // Includes sbrk()
using std::size_t;

// Allocates size memory to the heap
void *malloc(size_t size){
    void *block;
    block = sbrk(size);
    if (block == (void*) -1)
        return NULL;
    return block;
}

// Allows us to use blocks marked as free
struct header_t {
    size_t size;
    unsigned is_free;
    struct header_t *next;
};

// We want to align the end to 16 bytes of information (no idea why)
typedef char ALIGN[16];

union header{
    struct{
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
};
typedef union header header_t;

// head and tail pointer to know the extension of the list
header_t *head, *tail;

// creates mutex lock so that no two threads can allocate memory at the same time
pthread_mutex_t global_malloc_lock;

