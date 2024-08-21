#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>



#define MAX_SIZE 100000000

/********************************MallocMetadata**************************/


class MallocMetadata {
    private:
        size_t m_size;
        bool m_isFree;
        MallocMetadata* m_next;
        MallocMetadata* m_prev;
    
    public:
        size_t getSize();
        void setSize(size_t size);

        bool getFree();
        void setFree(bool free);

        MallocMetadata* getNext();
        void setNext(MallocMetadata* next);

        MallocMetadata* getPrev();
        void setPrev(MallocMetadata* prev);

        MallocMetadata(size_t size, MallocMetadata* prev);
        ~MallocMetadata() = default;
};

MallocMetadata::MallocMetadata(size_t size, MallocMetadata* prev): m_size(size), m_isFree(false), m_next(nullptr), m_prev(prev){

}

size_t MallocMetadata::getSize(){
    return m_size;
}

void MallocMetadata::setSize(size_t size){
    m_size = size;
}

bool MallocMetadata::getFree(){
    return m_isFree;
}

void MallocMetadata::setFree(bool free){
    m_isFree = free;
}

MallocMetadata* MallocMetadata::getNext(){
    return m_next;
}

void MallocMetadata::setNext(MallocMetadata* next)
{
    m_next = next;
}

MallocMetadata* MallocMetadata::getPrev(){
    return m_prev;
}

void MallocMetadata::setPrev(MallocMetadata* prev)
{
    m_prev = prev;
}

MallocMetadata* getLastMetadata(MallocMetadata* first){
    MallocMetadata* current = first;
    MallocMetadata* prev = current->getPrev();
    while(current){
        prev = current;
        current = current->getNext();
    }
    return prev;
}

MallocMetadata* getFirstMetadataBySize(MallocMetadata* first, size_t size){
    MallocMetadata* current = first;
    while(current){
            if(current->getSize()>=size && current->getFree()){
                return current;
            }
            current = current->getNext();
    }
    return NULL;
}

MallocMetadata* getBestFitMetadata(MallocMetadata* first, size_t size){
    MallocMetadata* current = first;
    MallocMetadata* bestFit;
    while(current){
            if(current->getSize()>=size && current->getFree() ){
                if(bestFit == NULL)
                {
                    bestFit = current;
                }
                else{
                    if(current->getSize() < bestFit->getSize()){
                        bestFit = current;
                    }
                }
            }
            current = current->getNext();
    }
    return bestFit;//will return NULL if there isnt a block
}

int getOrder(size_t realBlockSize) {
    int order = 0;
    while((realBlockSize / 2) != 0){
        realBlockSize = realBlockSize/2;
        order++;
    }
    return order;
}


/**********************FIRSTMETADATA DECLARATION**************************/


MallocMetadata* firstMetadata = NULL;


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
    
    if(_num_free_blocks() != 0 && _num_free_bytes() >= size){
        MallocMetadata* validMetadata = getFirstMetadataBySize(firstMetadata, size);
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

    // Set all bytes to 0
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






/*****************************************************************************************************************************************************/




