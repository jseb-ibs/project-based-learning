#include <iostream>
#include <unistd.h> // Includes sbrk()
#include <cstring>  // memset()
using std::size_t;

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
header_t *head{NULL}, *tail{NULL};

// creates mutex lock so that no two threads can allocate memory at the same time
pthread_mutex_t global_malloc_lock = PTHREAD_MUTEX_INITIALIZER;

header_t *get_free_block(size_t size){
    header_t *curr = head;
    while(curr) {
        if (curr->s.is_free && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

// malloc: sees whether we need to allocate memory from OS or from free blocks
void *malloc(size_t size){
    size_t total_size;
    void* block;
    header_t *header;
    if (!size)
        return NULL;
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if (header) {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    if (block == (void*) -1) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = (header_t *) block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if (!head)
        head = header;
    if (tail)
        tail->s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}

// free: frees the block to use it later when needed
void free (void *block){
    header_t *header, *tmp;
    void *programbreak;

    if (!block)
        return;
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block - 1;

    programbreak = sbrk(0);
    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

// calloc: allocates memory for an array of num elements and nsize bytes per element
void *calloc(size_t num, size_t nsize){
    size_t size;
    void* block;
    if (!num || !nsize)
        return NULL;
    size = num * nsize;
    /* check mul overflow */
    if (nsize != size / num)
        return NULL;
    block = malloc(size);
    if (!block)
        return NULL;
    memset(block, 0, size);
    return block;
}

// realloc:
void *realloc(void *block, size_t size){
    header_t *header;
    void *ret;
    if (!block || !size)
        return malloc(size);
    header = (header_t*)block - 1;
    if (header->s.size >= size)
        return block;
    ret = malloc(size);
    if (ret) {
        memcpy(ret,block, header->s.size);
        free(block);
    }
    return ret;
}