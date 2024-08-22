#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <iostream>

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
    if(size < 128){
        //something?
    }
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
    if(first == NULL){
        //std::cout << "first in get last meta data was NULL" << std::endl;
        return NULL;
    }
    MallocMetadata* current = first;
    MallocMetadata* prev = current->getPrev();
    while(current){
        prev = current;
        current = current->getNext();
    }
    //std::cout << "while in get last meta data has finished" << std::endl;
    return prev;
}

int getOrder(size_t realBlockSize) {
    int order = 0;
    //std::cout << "this is the block size in get order: " << realBlockSize << std::endl;
    while((realBlockSize / 2) >= 128){
        realBlockSize = realBlockSize/2;
        order++;
        //std::cout << "and this is the new block size: " << realBlockSize << " and this is the new order: " << order << std::endl;

    }
    return order;
}


/****************************META LIST HEADER****************************/


class MetaList {
    private:
        MallocMetadata* m_list [MAX_ORDER + 1];//cause you have both order 0 and order 10
        size_t m_blocksAllocated;
        size_t m_bytesAllocated;
    
    public:
        MetaList();
        MallocMetadata** getList();
        void makeArrayNull();

        size_t getAllocatedBlocks();
        void changeAllocatedBlocks(size_t change);
        size_t getAllocatedBytes();
        void changeAllocatedBytes(size_t change);

        void addSmallBlock(MallocMetadata* smallBlock);
        MallocMetadata* removeBlock(int order);
        void removeBlockForMerge(MallocMetadata* block);
        MallocMetadata* getBuddy(MallocMetadata* block);
        MallocMetadata* mergeBuddyBlocks(MallocMetadata* block);
        void initialize(size_t sizeOfArray);
};



/***************************GLOBAL DECLARATIONS**************************/


//MallocMetadata* firstMetadata = NULL;
MetaList* metaDataList = NULL;
MallocMetadata* mapList = NULL;


/**********************META LIST IMPLEMENTATION**************************/


MetaList::MetaList() : m_blocksAllocated(0), m_bytesAllocated(0){
    makeArrayNull();
}

void MetaList::makeArrayNull(){
    for(int i = 0; i < MAX_ORDER; i++) {
        if(m_list[i] != NULL){
            m_list[i] = NULL;
        }
    }
}

size_t MetaList::getAllocatedBlocks(){
    return m_blocksAllocated; 
}

void MetaList::changeAllocatedBlocks(size_t change){
    m_blocksAllocated += change;
}

size_t MetaList::getAllocatedBytes(){
    return m_bytesAllocated;
}

void MetaList::changeAllocatedBytes(size_t change){
    m_bytesAllocated += change;
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
    changeAllocatedBlocks(1);
    changeAllocatedBytes(MAX_BLOCK_SIZE);

    MallocMetadata* maxOrderMetadataList = m_list[MAX_ORDER];
    for(int i = 1; i < 32; i++){//why dont we just do add small block?
        maxOrderMetadataList->setNext((MallocMetadata*)((size_t)m_list[MAX_ORDER] + (i * MAX_BLOCK_SIZE)));
        maxOrderMetadataList->getNext()->setPrev(maxOrderMetadataList);
        maxOrderMetadataList->getNext()->setNext(NULL);
        maxOrderMetadataList->getNext()->setSize(MAX_BLOCK_SIZE);
        maxOrderMetadataList->getNext()->setFree(true);
        changeAllocatedBlocks(1);
        changeAllocatedBytes(MAX_BLOCK_SIZE);

        maxOrderMetadataList = maxOrderMetadataList->getNext();

    }
}

//notice blocks size needs to be with the metadata included
void MetaList::addSmallBlock(MallocMetadata* smallBlock){
    //std::cout << "addSmallBlock has been entered" << std::endl;
    size_t blockSize = smallBlock->getSize();
    int order = getOrder(blockSize);
    //std::cout << "this is the order of the block being added: " << order << std::endl;
    MallocMetadata* lastMetadata = getLastMetadata(m_list[order]);
    //std::cout << "getLastMetadata has finished" << std::endl;
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

    metaDataList->changeAllocatedBlocks(1);
    metaDataList->changeAllocatedBytes(blockSize);
}

void addBigBlock(MallocMetadata* bigBlock){
    //bigBlock->setSize(bigBlock->getSize());//?????????????????????????????????????????????
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

    metaDataList->changeAllocatedBlocks(1);
    metaDataList->changeAllocatedBytes(bigBlock->getSize());
}

MallocMetadata* MetaList::removeBlock(int order){
    //std::cout << "entered remove block and order is: " << order << std::endl;
    MallocMetadata* current = m_list[order];
    if(current != NULL){
        //std::cout << "current isn't NULL" << std::endl;
        if(current->getFree() == true){ //should always be true but still :)
            //std::cout << "current is free" << std::endl;
            m_list[order] = current->getNext();
            if(current->getNext() != NULL){
                current->getNext()->setPrev(NULL);
            }
            current->setFree(false);
            return current;
        }
        //std::cout << "current isn't free" << std::endl;
    }
    return NULL;
}

void MetaList::removeBlockForMerge(MallocMetadata* block){
    //std::cout << "remove block merge was entered" << std::endl;
    int order = getOrder(block->getSize());
    MallocMetadata* current = m_list[order];
    while(current){
        if(current == block){
            if(block->getNext() == NULL && block->getPrev() == NULL){
                m_list[order] = NULL;
                //block->setFree(false);
            }
            if(block->getPrev() != NULL){
                block->getPrev()->setNext(block->getNext());
            }
            if(block->getNext() != NULL){
                block->getNext()->setPrev(block->getPrev());
            }
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
    //std::cout << "split block was entered ";
    size_t size = block->getSize();// + sizeof(MallocMetadata);
    //std::cout << "and this is the size: " << size << std::endl;
    if(size < 128){
        return NULL;
    }
    // while (remainingSize > requestedSize && block->getOrder(remainingSize) > 0) {
    //     size_t remainingSize = remainingSize / 2;
    //     if (remainingSize < requestedSize) {
    //         break;
    //     }
    // }
    MallocMetadata* buddy = (MallocMetadata*)((char*)(block) + (size/2));
    buddy->setSize((block->getSize())/2);
    metaDataList->addSmallBlock(buddy);
    //std::cout << "small block was added" << std::endl;
    block->setSize(block->getSize()/2);
    block->setFree(true);
    block->setNext(NULL);
    block->setPrev(NULL);
    //std::cout << "split block has finished" << std::endl;
    return block;
}

MallocMetadata* MetaList::getBuddy(MallocMetadata* block){
    return (MallocMetadata*)((size_t) block ^ block->getSize());
}

//not recursive!!!!!!! also check prior if max order
MallocMetadata* MetaList::mergeBuddyBlocks(MallocMetadata* block){
    //std::cout << "merge buddy blockes was entered" << std::endl;
    if(block->getSize() >= MAX_BLOCK_SIZE){
        return NULL;
    }
    MallocMetadata* buddy = getBuddy(block);
    //std::cout << "this is buddy's size: " << buddy->getSize() << " and this is it's order: " << getOrder(buddy->getSize()) << std::endl;
    //std::cout << "this is block's size: " << block->getSize() << " and this is it's order: " << getOrder(block->getSize()) << std::endl;
    if(getOrder(buddy->getSize()) == getOrder(block->getSize())){
        //std::cout << "is buddy free? " << buddy->getFree() << std::endl;
        if(buddy->getFree()){
            if(block > buddy){//removing the one with the bigger
                removeBlockForMerge(block);
                removeBlockForMerge(buddy);
                buddy->setSize(buddy->getSize() * 2);
                addSmallBlock(buddy);
                metaDataList->changeAllocatedBlocks(-2);
                metaDataList->changeAllocatedBytes(-(buddy->getSize()));//might actually be bigger
                return buddy;
            }
            else{
                removeBlockForMerge(buddy);
                removeBlockForMerge(block);
                block->setSize(block->getSize() * 2);
                addSmallBlock(block);
                metaDataList->changeAllocatedBlocks(-2);
                //std::cout << "this is the current allocated blocks value: " << getAllocatedBlocks() << std::endl;
                metaDataList->changeAllocatedBytes(-(block->getSize()));//might actually be bigger
                return block;
            }
        }
    }
    //std::cout << "merge has finished" << std::endl;
    return NULL;
}


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
            MallocMetadata* current = (metaDataList->getList())[i];
            while(current != NULL)
            {
                if(current->getFree() == true){
                    sum+= current->getSize() - sizeof(MallocMetadata);
                }
                current = current->getNext();
            }
        }
    return sum;
}

size_t _num_allocated_blocks(){
    return metaDataList->getAllocatedBlocks();
}

size_t _num_allocated_bytes(){
    return metaDataList->getAllocatedBytes();
    /*MallocMetadata* current;
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

    return sum;*/
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
        //std::cout << "first smalloc if was entered" << std::endl;
        size_t sizeOfArray = sizeof(MallocMetadata*) * (MAX_ORDER + 1);
        metaDataList = (MetaList*)sbrk(sizeOfArray);
        if(metaDataList == (void*) -1){
            return NULL;
        }
        //std::cout << "about to make array null" << std::endl;
        metaDataList->makeArrayNull();
        //std::cout << "about to initialize array" << std::endl;
        metaDataList->initialize(sizeOfArray);
        //std::cout << "first smalloc if was finished" << std::endl;
    }
    
    if(size == 0 || size > MAX_SIZE) {
        return NULL;
    }

    if(size + _size_meta_data() <= MAX_BLOCK_SIZE){
        //std::cout << "size was smaller than MAX_BLOCK_SIZE" << std::endl;
        MallocMetadata* removed;
        MallocMetadata newBlock (size + _size_meta_data(), NULL);
        int order = getOrder((&newBlock)->getSize());
        for(int i = order; i <= MAX_ORDER; i++){
            //std::cout << "first for was entered and the order is: " << order << std::endl;
            removed = metaDataList->removeBlock(i);
            if(removed == NULL){
                //std::cout << "removed was NULL" << std::endl;
                continue;
            }
            else{
                if(i == order){
                    //std::cout << "i equalled order" << std::endl;
                    return (void*)((char*)removed + _size_meta_data());
                }
                else{
                    //std::cout << "second else was entered after smaller than MAX_BLOCK_SIZE" << std::endl;
                    MallocMetadata* toSplit = removed;
                    for(int j = 1; j <= i - order; j++){
                        toSplit = splitBlock(toSplit);
                        if(toSplit == NULL){
                            return NULL;//should'nt be possible, means we split to many times
                        }
                        //metaDataList->addSmallBlock(toSplit);
                    }
                    //std::cout << "smalloc has finished" << std::endl;
                    toSplit->setFree(false);
                    return (void*)((char*)toSplit + _size_meta_data());
                }
            }  
        }
        return NULL;//there was no room
    }
    else{//size is bigger than MAX_BLOCK_SIZE
        //std::cout << "size was bigger than MAX_BLOCK_SIZE" << std::endl;
        MallocMetadata* bigBlock = (MallocMetadata*)mmap(NULL, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if(bigBlock == (void*) -1){
            //std::cout << "mmap returned null " << std::endl;
            return NULL;
        }
        //std::cout << "mmap was successful " << std::endl;
        bigBlock->setSize(size + _size_meta_data());
        addBigBlock(bigBlock);
        //std::cout << "big block was added " << std::endl;
        return (void*)((size_t) bigBlock + _size_meta_data());
    }
    //std::cout << "smalloc has finished" << std::endl;
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
    //std::cout << "sfree was entered" << std::endl;
    if(p == NULL){
        return;
    }

    MallocMetadata* MetadataToRemove = (MallocMetadata*)((size_t)p - _size_meta_data());//////this doesnt work for big meta data
    if(MetadataToRemove == NULL){
        //std::cout << "metadata is NULL in sfree" << std::endl;
    }
    //std::cout << "sfree is about to get the size of metadata" << std::endl;
    size_t MetadataToRemoveSize = MetadataToRemove->getSize();
    //std::cout << "first if is about to be check in sfree" << std::endl;
    if(MetadataToRemoveSize > MAX_BLOCK_SIZE){
        //std::cout << "the size is bigger than MAX_BLOCK_SIZE " << std::endl;
        if(mapList == NULL){
            return;
        }
        if(MetadataToRemove->getNext() != NULL){
            if(MetadataToRemove->getPrev() != NULL){
                MetadataToRemove->getPrev()->setNext(MetadataToRemove->getNext());
                MetadataToRemove->getNext()->setPrev(MetadataToRemove->getPrev());
            }
            else{//MetadataToRemove->getPrev() == NULL
                MetadataToRemove->getNext()->setPrev(NULL);
                mapList = MetadataToRemove->getNext();
            } 
        }
        else{ //MetadataToRemove->getNext() == NULL
            if(MetadataToRemove->getPrev() != NULL){
                MetadataToRemove->getPrev()->setNext(NULL);
            }
            else{//emptying map list because both prev and next are NULL
                //std::cout << "the list was emptied" << std::endl;
                mapList == NULL;
            }
        }
        MetadataToRemove->setFree(true);
        munmap(MetadataToRemove, MetadataToRemoveSize);//its says not to consider as free in notes about mmap? idk....
        metaDataList->changeAllocatedBlocks(-1);
        metaDataList->changeAllocatedBytes(-MetadataToRemoveSize);
        return;
    }
    else{ //MetadataToRemovesize <= MAX_BLOCK_SIZE
        if(MetadataToRemove->getFree() == true){
            //std::cout << "it was free already" << std::endl;
            return;
        }
        //std::cout << "we're freeing a small block" << std::endl;
        if(MetadataToRemoveSize == MAX_BLOCK_SIZE){
            metaDataList->addSmallBlock(MetadataToRemove);
            //std::cout << "small block was added" << std::endl;
            return;
        }
        else{
            //std::cout << "is ptr1 free?: " << MetadataToRemove->getFree() << std::endl;
            while(MetadataToRemoveSize < MAX_BLOCK_SIZE){
                MallocMetadata* tmp = MetadataToRemove;
                MetadataToRemove = metaDataList->mergeBuddyBlocks(MetadataToRemove);
                if(MetadataToRemove == NULL){
                    metaDataList->addSmallBlock(tmp);
                    metaDataList->changeAllocatedBlocks(-1);
                    metaDataList->changeAllocatedBytes(tmp->getSize());
                    break;
                }
                MetadataToRemoveSize = MetadataToRemove->getSize();
            }
        } 
    }
    //std::cout << "end of sfree was somehow reached" << std::endl;
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


/*int main(){
    void* ptr1 = smalloc(40);
    void *ptr2 = smalloc(MAX_BLOCK_SIZE + 100);
    void *ptr3 = smalloc(50);
    /*MallocMetadata* current;
    int counter = 0;
    for(int i = 0; i <= MAX_ORDER; i++){
        current = (metaDataList->getList())[i];
        while(current != NULL){
            current = current->getNext();
            counter++;
        }
        std::cout << "this was the order: " << i <<" and this was the counter: " << counter << std::endl;
        counter = 0;
    }*/
    /*std::cout <<"this is the num of allocated blocks before freeing: " << _num_allocated_blocks() << std::endl;
    sfree(ptr1);
    std::cout <<"this is the num of allocated blocks after freeing 1: " << _num_allocated_blocks() << std::endl;
    void *ptr4 = smalloc(40);
    std::cout <<"this is the num of allocated blocks after freeing 1 and allocating 1 more: " << _num_allocated_blocks() << std::endl;
    sfree(ptr3);
    sfree(ptr4);
    std::cout <<"this is the num of allocated blocks after freeing 3: " << _num_allocated_blocks() << std::endl;

    sfree(ptr2);
    std::cout <<"this is the num of allocated blocks after freeing 4: " << _num_allocated_blocks() << std::endl;



    return 0;
}*/









