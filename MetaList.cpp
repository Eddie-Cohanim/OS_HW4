#include "MetaList.h"

#define MAX_BLOCK_SIZE 131072


MetaList::MetaList(){
    for(int i = 0; i < MAX_ORDER; ++i) {
            m_list[i] = nullptr;
        }
    for(int i = 1; i <= 32; ++i){//setting the first 32 blocks as instructed
        getLastMetadata(m_list[MAX_ORDER])->setPrev(sbrk(MAX_BLOCK_SIZE));
        getLastMetadata(m_list[MAX_ORDER])->setNext(sbrk(0));
    }
}

void MetaList::addBlock(MallocMetadata* block){
    size_t blockSize = block->getSize();
    size_t realBlockSize = blockSize;
    int order = block->getOrder(realBlockSize);
    
    MallocMetadata* lastMetadata = getLastMetadata(m_list[order]);
    if(lastMetadata == NULL){
        m_list[order] = block;
        block->setPrev(NULL);
        block->setNext(NULL);//not sure if we need this
        return;
    }
    else{
        lastMetadata->setNext(block);
        block->setPrev(lastMetadata);
        block->setNext(NULL);//not sure if we need this
    } 

    block->setNext(NULL);
    //block->setFree(false);//why?
    block->setFree(false);

}


MallocMetadata* MetaList::splitBlock(MallocMetadata* block, size_t requestedSize){
    size_t remainingSize = block->getSize();
    while (remainingSize > requestedSize && block->getOrder(remainingSize) > 0) {
        size_t remainingSize = remainingSize / 2;
        if (remainingSize < requestedSize) {
            break;
        }
    }

    MallocMetadata* buddy = (MallocMetadata*)((char*)block + remainingSize); ////// i have no idea if this is correct
    block->setSize(remainingSize);
    buddy->setNext(block->getNext());
    if(block->getNext()){
        block->getNext()->setPrev(buddy);
    }
    block->setNext(buddy);

    buddy->setFree(true);

    addBlock(buddy);
}

MallocMetadata* MetaList::getBuddy(MallocMetadata* block){

}

void MetaList::mergeBuddyBlocks(MallocMetadata* block){
    if(block->getBuddy()->getFree()){
        //merge
        //mergeBuddyBlocks(merged block) //so its recursive
    }
}
