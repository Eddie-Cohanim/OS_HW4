#include <unistd.h> 
#include <exception>

#define MAX_SIZE = 100000000


void* smalloc(size_t size){
    if(size==0 || size > MAX_SIZE) {
        return NULL;
    }
    if(sbrk(size) == (void*)(-1)){
        return NULL;
    }
    return sbrk(0);
} 
