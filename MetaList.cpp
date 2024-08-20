#include "MetaList.h"

#define MAX_BLOCK_SIZE 131072

MetaList::MetaList(){
    makeArrayNull();
}

void MetaList::makeArrayNull(){
    for(int i = 0; i < MAX_ORDER; i++) {
        m_list[i] = nullptr;
    }
}

MallocMetadata** MetaList::getList(){
    return m_list;
}

void MetaList::initialize(size_t sizeOfArray){
    size_t leftover = (32 * MAX_BLOCK_SIZE) - ((size_t)this + sizeOfArray) % (32 * MAX_BLOCK_SIZE);
    void* temp = sbrk(leftover + (32 * MAX_BLOCK_SIZE));
    if(temp == (void*) -1){
        return;
    }

    m_list[MAX_ORDER] = (MallocMetadata*)((size_t)temp + leftover);
    m_list[MAX_ORDER]->setSize(MAX_BLOCK_SIZE);
    m_list[MAX_ORDER]->setPrev(NULL);
    m_list[MAX_ORDER]->setNext(NULL);
    m_list[MAX_ORDER]->setFree(true);
    MallocMetadata* maxOrderMetadataList = m_list[MAX_ORDER];
    for(int i = 1; i < 32; i++)
    {
        maxOrderMetadataList->setNext((MallocMetadata*)((size_t)m_list[MAX_ORDER] + (i * MAX_BLOCK_SIZE)));
        maxOrderMetadataList->getNext()->setPrev(maxOrderMetadataList);
        maxOrderMetadataList->getNext()->setNext(NULL);
        maxOrderMetadataList->getNext()->setSize(MAX_BLOCK_SIZE);
        maxOrderMetadataList->getNext()->setFree(true);

        maxOrderMetadataList = maxOrderMetadataList->getNext();
    }
}

//notice blocks size needs to be with the metadata included
void MetaList::addBlock(MallocMetadata* block){
    size_t blockSize = block->getSize();
    int order = block->getOrder(blockSize);
    
    MallocMetadata* lastMetadata = getLastMetadata(m_list[order]);
    if(lastMetadata == NULL){
        m_list[order] = block;
        block->setPrev(NULL);
    }
    else{
        lastMetadata->setNext(block);
        block->setPrev(lastMetadata); 
    } 

    block->setNext(NULL);
    block->setFree(true);
}

void MetaList::removeBlock(MallocMetadata* block, bool forMerge){
    MallocMetadata* current = m_list[block->getOrder(block->getSize())];
    while(current){
        if(current == block){
            block->getPrev()->setNext(block->getNext());
            block->getNext()->setPrev(block->getPrev());
            if(forMerge == false){
                block->setFree(false);
            }
            break;
        }
        current = current->getNext();
    }
    //shouldnt reach here, means the block wasnt found
}

MallocMetadata* MetaList::splitBlock(MallocMetadata* block){
    size_t size = block->getSize() + sizeof(MallocMetadata);
    // while (remainingSize > requestedSize && block->getOrder(remainingSize) > 0) {
    //     size_t remainingSize = remainingSize / 2;
    //     if (remainingSize < requestedSize) {
    //         break;
    //     }
    // }
    MallocMetadata* buddy = (MallocMetadata*)((char*)(block + (size/2)));
    buddy->setSize((block->getSize())/2);
    addBlock(buddy);
    block->setSize(block->getSize()/2);
    block->setFree(false);
    block->setNext(NULL);
    block->setPrev(NULL);
    return block;
}

MallocMetadata* MetaList::getBuddy(MallocMetadata* block){
    return (MallocMetadata*)((size_t) block ^ block->getSize());
}

//not recursive!!!!!!! also check prior if max order
MallocMetadata* MetaList::mergeBuddyBlocks(MallocMetadata* block){
    MallocMetadata* buddy = getBuddy(block);
    if(buddy->getSize() == block->getSize()){
        if(buddy->getFree()){
            int order = block->getOrder(block->getSize());
            if(block > buddy){//removing the one with the bigger
                removeBlock(block, true);

                block->setSize(0);//not sure? for this whole thing
                block->setNext(NULL);
                block->setPrev(NULL);

                removeBlock(buddy, true);
                buddy->setSize(buddy->getSize() * 2);
                addBlock(buddy);
                return buddy;
            }
            else{
                removeBlock(buddy, true);

                buddy->setSize(0);//not sure? for this whole thing
                buddy->setNext(NULL);
                buddy->setPrev(NULL);

                removeBlock(block, true);
                block->setSize(block->getSize() * 2);
                addBlock(block);
                return block;
            }
        }
    }
    return NULL;
}
