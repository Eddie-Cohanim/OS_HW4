#include <unistd.h> 

#define MAX_SIZE 100000000


void* smalloc(size_t size){
    if(size==0){ 
        return NULL;
    }
    if(size > MAX_SIZE) {
        return NULL;
    }
    void* newAlloc = sbrk(size);
    if(newAlloc == (void*)(-1)){
        return NULL;
    }
    return newAlloc;
} 
