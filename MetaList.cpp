#include "MetaList.h"


MetaList::MetaList(){
    for (int i = 0; i < MAX_ORDER; ++i) {
            m_list[i] = nullptr;
        }
}



void MetaList::addBlock(MallocMetadata* Block){
    size_t blockSize = Block->getSize();
    size_t realBlockSize = blockSize;
    int order = Block->getOrder(realBlockSize);
    
    MallocMetadata* lastMetadata = getLastMetadata(m_list[order]);
    if(lastMetadata == NULL){
        m_list[order] = Block;
        Block->setPrev(NULL);
        return;
    }
    else{
        lastMetadata->setNext(Block);
        Block->setPrev(lastMetadata);
    } 

    Block->setNext(NULL);
    Block->setFree(false);
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

MallocMetadata* getBuddy(MallocMetadata* block){

}
