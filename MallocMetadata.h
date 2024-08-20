#ifndef MALLOC_META_DATA
#define MALLOC_META_DATA


#include <stddef.h>

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

        int getOrder(size_t realBlockSize);//should this be here? afterall the size in metadata is without the metadata
        //also why have it here as a method if it works for any size an not the instance's specific size?

        MallocMetadata(size_t size, MallocMetadata* prev);
        ~MallocMetadata() = default;
};

MallocMetadata* getLastMetadata(MallocMetadata* first);

MallocMetadata* getFirstMetadataBySize(MallocMetadata* first, size_t size);

MallocMetadata* getBestFitMetadata(MallocMetadata* first, size_t size);

#endif MALLOC_META_DATA