#include "MallocMetadata.h"

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


/*int MallocMetadata::getOrder(size_t realBlockSize) {
    int order = 0;
    while((realBlockSize / 2) != 0){
        realBlockSize = realBlockSize/2;
        order++;
    }
        return order;
}*/

MallocMetadata* getLastMetadata(MallocMetadata* current){
    MallocMetadata* prev = current->getPrev();
    while(current){
        prev = current;
        current = current->getNext();
    }
    return prev;
}

MallocMetadata* getLastMetadataBySize(MallocMetadata* current, size_t size){
    MallocMetadata* prev = current->getPrev();
    while(current){
            if(current->getSize()>=size && current->getFree()){
                return current;
            }
            current = current->getNext();
    }
    return NULL;
}