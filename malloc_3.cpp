#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>

#define MAX_SIZE 100000000
#define MAX_ORDER 10
#define MAX_BLOCK_SIZE 131072


/***************************MALLOCMETADATA HEADER************************/


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


/***********************MALLOCMETADATA IMPLEMENTATION********************/


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

int getOrder(size_t realBlockSize) {
    int order = 0;
    while((realBlockSize / 2) != 0){
        realBlockSize = realBlockSize/2;
        order++;
    }
    return order;
}


/****************************META LIST HEADER****************************/


class MetaList {
    private:
        MallocMetadata* m_list [MAX_ORDER + 1];//cause you have both order 0 and order 10
    
    public:
        MetaList();
        MallocMetadata** getList();
        void makeArrayNull();

        void addSmallBlock(MallocMetadata* smallBlock);
        MallocMetadata* removeBlock(int order);
        void removeBlockForMerge(MallocMetadata* block);
        MallocMetadata* getBuddy(MallocMetadata* block);
        MallocMetadata* mergeBuddyBlocks(MallocMetadata* block);
        void initialize(size_t sizeOfArray);
};


/**********************META LIST IMPLEMENTATION**************************/

MallocMetadata* mapList = NULL;

MetaList::MetaList(){
    makeArrayNull();
}

void MetaList::makeArrayNull(){
    for(int i = 0; i < MAX_ORDER; i++) {
        if(m_list[i] != NULL){
            m_list[i] = NULL;
        }
    }
}

MallocMetadata** MetaList::getList(){
    return m_list;
}

void MetaList::initialize(size_t sizeOfArray){
    size_t leftover = (32 * MAX_BLOCK_SIZE) - ((size_t)this + sizeOfArray) % (32 * MAX_BLOCK_SIZE);
    void* tmp= sbrk(leftover + (32 * MAX_BLOCK_SIZE));
    if(tmp == (void*) -1){
        return;
    }

    m_list[MAX_ORDER] = (MallocMetadata*)((size_t)tmp + leftover);
    m_list[MAX_ORDER]->setSize(MAX_BLOCK_SIZE);
    m_list[MAX_ORDER]->setPrev(NULL);
    m_list[MAX_ORDER]->setNext(NULL);
    m_list[MAX_ORDER]->setFree(true);
    MallocMetadata* maxOrderMetadataList = m_list[MAX_ORDER];
    for(int i = 1; i < 32; i++){
        maxOrderMetadataList->setNext((MallocMetadata*)((size_t)m_list[MAX_ORDER] + (i * MAX_BLOCK_SIZE)));
        maxOrderMetadataList->getNext()->setPrev(maxOrderMetadataList);
        maxOrderMetadataList->getNext()->setNext(NULL);
        maxOrderMetadataList->getNext()->setSize(MAX_BLOCK_SIZE);
        maxOrderMetadataList->getNext()->setFree(true);

        maxOrderMetadataList = maxOrderMetadataList->getNext();
    }
}

//notice blocks size needs to be with the metadata included
void MetaList::addSmallBlock(MallocMetadata* smallBlock){
    size_t blockSize = smallBlock->getSize();
    int order = getOrder(blockSize);

    MallocMetadata* lastMetadata = getLastMetadata(m_list[order]);
    if(lastMetadata == NULL){
        m_list[order] = smallBlock;
        smallBlock->setPrev(NULL);
    }
    else{
        lastMetadata->setNext(smallBlock);
        smallBlock->setPrev(lastMetadata); 
    } 

    smallBlock->setNext(NULL);
    smallBlock->setFree(true);
    
    
}

void addBigBlock(MallocMetadata* bigBlock){
    bigBlock->setSize(bigBlock->getSize());
    bigBlock->setPrev(NULL);
    bigBlock->setFree(false);
    if(mapList == NULL)
    {
        bigBlock->setNext(NULL);
    }
    else
    {
        bigBlock->setNext(mapList);
        mapList->setPrev(bigBlock);
    }
    mapList = bigBlock;
}

MallocMetadata* MetaList::removeBlock(int order){
    MallocMetadata* current = m_list[order];
    if(current != NULL){
        if(current->getFree() == true){ //should always be true but still :)
            m_list[order] = current->getNext();
            if(current->getNext() != NULL){
                current->getNext()->setPrev(NULL);
            }
            current->setFree(false);
            return current;
        }
    }
    return NULL;
}

void MetaList::removeBlockForMerge(MallocMetadata* block){
    MallocMetadata* current = m_list[getOrder(block->getSize())];
    while(current){
        if(current == block){
            block->getPrev()->setNext(block->getNext());
            block->getNext()->setPrev(block->getPrev());
            /*if(forMerge == false){
                block->setFree(false);
                block->setSize(0);//not sure? for this whole thing
                block->setNext(NULL);
                block->setPrev(NULL);
            }*/
            break;
        }
        current = current->getNext();
    }
    //shouldnt reach here, means the block wasnt found
}

MallocMetadata* splitBlock(MallocMetadata* block){
    size_t size = block->getSize();// + sizeof(MallocMetadata);
    // while (remainingSize > requestedSize && block->getOrder(remainingSize) > 0) {
    //     size_t remainingSize = remainingSize / 2;
    //     if (remainingSize < requestedSize) {
    //         break;
    //     }
    // }
    MallocMetadata* buddy = (MallocMetadata*)((char*)(block + (size/2)));
    buddy->setSize((block->getSize())/2);
    metaDataList->addSmallBlock(buddy);
    block->setSize(block->getSize()/2);
    block->setFree(true);
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
            if(block > buddy){//removing the one with the bigger
                removeBlockForMerge(block);
                removeBlockForMerge(buddy);
                buddy->setSize(buddy->getSize() * 2);
                addSmallBlock(buddy);
                return buddy;
            }
            else{
                removeBlockForMerge(buddy);
                removeBlockForMerge(block);
                block->setSize(block->getSize() * 2);
                addSmallBlock(block);
                return block;
            }
        }
    }
    return NULL;
}


/***************************GLOBAL DECLARATIONS**************************/


//MallocMetadata* firstMetadata = NULL;
MetaList* metaDataList = NULL;


/*****************************STATS METHODS******************************/


size_t _num_free_blocks(){
    size_t count = 0;
    if(metaDataList != NULL){
        for(int i = 0; i < MAX_ORDER; i++){
            MallocMetadata* current = (metaDataList->getList())[i];
            while(current != NULL){
                if(current->getFree() == true){
                    count++;
                }
                current = current->getNext();
            }
        }
    }

    return count;
}

size_t _num_free_bytes(){
    int sum = 0;
    if(metaDataList != NULL)
        for(unsigned int i = 0; i <= MAX_ORDER; i++)
        {
            MallocMetadata* runner = (metaDataList->getList())[i];
            while(runner != NULL)
            {
                if(runner->getFree() == true){
                    sum+= runner->getSize() - sizeof(MallocMetadata);
                }
                runner = runner->getNext();
            }
        }
    return sum;
}


size_t _num_allocated_blocks(){
    MallocMetadata* current;
    int counter = 0;
    if(metaDataList != NULL)
        for(int i = 0; i <= MAX_ORDER; i++)
        {
            current = (metaDataList->getList())[i];
            while(current != NULL)
            {
                counter++;
                current = current->getNext();
            }
        }
    current = mapList;
    while(current != NULL)
    {
        counter++;
        current = current->getNext();
    }

    return counter;

}

size_t _num_allocated_bytes(){
    MallocMetadata* current;
    int sum = 0;
    if(metaDataList != NULL)
        for(unsigned int i = 0; i <= MAX_ORDER; i++)
        {
            current = (metaDataList->getList())[i];
            while(current != NULL)
            {
                sum += current->getSize() - sizeof(MallocMetadata);
                current = current->getNext();
            }
        }

    current = mapList;
    while(current != NULL)
    {
        sum += current->getSize() - sizeof(MallocMetadata);
        current = current->getNext();
    }

    return sum;

}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes(){
    MallocMetadata* current;
    int sum = 0;
    if(metaDataList != NULL)
        for(unsigned int i = 0; i <= MAX_ORDER; i++)
        {
            current = (metaDataList->getList())[i];
            while(current != NULL)
            {
                sum += sizeof(MallocMetadata);
                current = current->getNext();
            }
        }
    
    current = mapList;
    while(current != NULL)
    {
        sum += sizeof(MallocMetadata);
        current = current->getNext();
    }

    return sum;
}


/******************MALLOCS AND SHIT*************************************/


void* smalloc(size_t size){
    if(metaDataList == NULL){
        size_t sizeOfArray = sizeof(MallocMetadata*) * (MAX_ORDER + 1);
        metaDataList = (MetaList*)sbrk(sizeOfArray);
        if(metaDataList == (void*) -1){
            return NULL;
        }
        metaDataList->makeArrayNull();
        metaDataList->initialize(sizeOfArray);
    }
    
    if(size == 0 || size > MAX_SIZE) {
        return NULL;
    }
    if(size + _size_meta_data() <= MAX_BLOCK_SIZE){
        MallocMetadata* removed;
        MallocMetadata newBlock (size + _size_meta_data(), NULL);
        int order = getOrder((&newBlock)->getSize());
        for(int i = order; i <= MAX_ORDER; i++){
            removed = metaDataList->removeBlock(i);
            if(removed == NULL){
                continue;
            }
            
            
            
        }



        return (void*)((char*)&newBlock + _size_meta_data());
    }
    else{
        MallocMetadata* bigBlock = (MallocMetadata*)mmap(NULL, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(bigBlock == (void*) -1){
            return NULL;
        }
        bigBlock->setSize(size + _size_meta_data());
        addBigBlock(bigBlock);
        return ((void*) bigBlock);
    }
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

    MallocMetadata* MetadataToRemove = (MallocMetadata*)(((char *)p) - (_size_meta_data()));
    size_t MetadataToRemoveSize = MetadataToRemove->getSize();

    if(MetadataToRemoveSize > MAX_BLOCK_SIZE){
        if(MetadataToRemove->getNext() != NULL){
            if(MetadataToRemove->getPrev() != NULL){
                MetadataToRemove->getPrev()->setNext(MetadataToRemove->getNext());
                MetadataToRemove->getNext()->setPrev(MetadataToRemove->getPrev());
            }
            else{
                MetadataToRemove->getNext()->setPrev(NULL);
                mapList = MetadataToRemove->getNext();
            } 
        }
        else{
            if(MetadataToRemove->getPrev() != NULL){
                MetadataToRemove->getPrev()->setNext(NULL);
            }
            else{
                mapList = MetadataToRemove->getNext();
            }
        }
        munmap(MetadataToRemove, MetadataToRemoveSize);
        return;
    }
    else{
        if(MetadataToRemoveSize == MAX_BLOCK_SIZE){
            metaDataList->addSmallBlock(MetadataToRemove);
            return;
        }
        else{
            while(MetadataToRemoveSize < MAX_BLOCK_SIZE){
                //MallocMetadata* MetadataToRemoveBuddy = metaDataList->getBuddy(MetadataToRemove);
                MetadataToRemove = metaDataList->mergeBuddyBlocks(MetadataToRemove);
                MetadataToRemoveSize = MetadataToRemove->getSize();
                }
            } 
    }
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












