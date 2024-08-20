#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "MallocMetadata.h"
#include "MetaList.h"



#define MAX_SIZE 100000000

MallocMetadata* firstMetadata = NULL;
MetaList* metaDataList = NULL;



/*****************************STATS METHODS******************************/

size_t _num_free_blocks(){
    size_t count =0;
    MallocMetadata* current = firstMetadata;
    while(current){
        if(current->getFree()){
            count++;
        }
        current = current->getNext();
    }
    return count;

}

size_t _num_free_bytes(){
    size_t size = 0;
    MallocMetadata* current = firstMetadata;
    while(current){
        if(current->getFree()){
            size += current->getSize();
        }
        current = current->getNext();
    }
    return size;//8 bytes in size_t, do we need to multiply? i think not cause the value in the MMD is per bytes
}

size_t _num_allocated_blocks(){
    size_t count =0;
    MallocMetadata* current = firstMetadata;
    while(current){
        count++;
        current = current->getNext();
    }
    return count;
}

size_t _num_allocated_bytes(){
    size_t size = 0;
    
    MallocMetadata* current = firstMetadata;
    while(current){
        size += current->getSize();
        current = current->getNext();
    }
    return size;
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
    
}



/******************MALLOCS AND SHIT*************************************/


void* smalloc(size_t size){
    if(size == 0 || size > MAX_SIZE) {
        return NULL;
    }

    if(metaDataList == NULL){
        size_t sizeOfArray = sizeof(MallocMetadata*) * (MAX_ORDER + 1);
        void* listToAllocate = (MetaList*)sbrk(sizeOfArray);
        if(listToAllocate == (void*) -1){
            return;
        }
        ((MetaList*) listToAllocate)->makeArrayNull();
        metaDataList = (MetaList*) listToAllocate;
        metaDataList->initialize(sizeOfArray);
    }

























    
    if(firstMetadata == NULL){
        void* blockToAllocate = sbrk(size +_size_meta_data());
        if( blockToAllocate == (void*)(-1)){
            return NULL;
        }
        ((MallocMetadata*) blockToAllocate)->setSize(size);
        ((MallocMetadata*) blockToAllocate)->setFree(false);
        ((MallocMetadata*) blockToAllocate)->setPrev(NULL);
        ((MallocMetadata*) blockToAllocate)->setNext(NULL);
        firstMetadata = (MallocMetadata*) blockToAllocate;
        return (void*)((char*)blockToAllocate + _size_meta_data());
    } 

    MallocMetadata* current = firstMetadata;
    
    if(_num_free_blocks() != 0 && _num_free_bytes() >= size){
        MallocMetadata* validMetadata = getBestFitMetadata(firstMetadata, size);
        if(validMetadata != NULL){
            validMetadata -> setFree(false);
                    return (void*)((char*)validMetadata + _size_meta_data());

        }
    }   
    MallocMetadata* lastMetadata = getLastMetadata(firstMetadata); 
    void* blockToAllocate = sbrk(size +_size_meta_data());
    if( blockToAllocate == (void*)(-1)){
        return NULL;
    }
    ((MallocMetadata*) blockToAllocate)->setSize(size);
    ((MallocMetadata*) blockToAllocate)->setFree(false);
    ((MallocMetadata*) blockToAllocate)->setPrev(lastMetadata);
    lastMetadata->setNext((MallocMetadata*)blockToAllocate);
    ((MallocMetadata*) blockToAllocate)->setNext(NULL);
    return (void*)((char*)blockToAllocate + _size_meta_data());
}




void* scalloc(size_t num, size_t size){
    size_t total_size = num * size;
    if (num == 0 || size == 0 || total_size > MAX_SIZE) {
        return NULL;
    }
    
    void* ptr = smalloc(total_size);
    if (!ptr) {
        return NULL;
    }

    memset(ptr, 0, total_size);

    return ptr;
}




void sfree(void* p){
    if(p == NULL){
        return;
    }
    MallocMetadata* Metadata = (MallocMetadata*)(((char *)p) - (_size_meta_data()));
    Metadata->setFree(true);
}




void* srealloc(void* oldp, size_t size){
    if(size == 0 || size >= MAX_SIZE){/////////////////////////////// the tests think its equal or bigger but i think thats a mistake
        return NULL;
    }
    if(oldp == NULL){
        return smalloc(size);
    }

    MallocMetadata* oldMetadata = (MallocMetadata*)((char*)oldp - _size_meta_data());/////////////////??????? might be wrong
    if(size <= oldMetadata->getSize()){
        return oldp;
    }
    else{
        void *newMetadata = smalloc(size);
        if (newMetadata == NULL)
        {
            return NULL;
        }
        memmove(newMetadata, oldp, oldMetadata->getSize());
        sfree(oldp);
        return newMetadata;
    }
}

/*int main(){
    return 0;
}*/