#ifndef MALLOC_META_DATA
#define MALLOC_META_DATA


#include <stddef.h>

class MallocMetadata {
    private:
        size_t m_size;
        bool m_isFree;
        MallocMetadata* m_next;
        MallocMetadata* m_prev;
        MallocMetadata* m_buddy;
    
    public:
        size_t getSize();
        void setSize(size_t size);

        bool getFree();
        void setFree(bool free);

        MallocMetadata* getNext();
        void setNext(MallocMetadata* next);

        MallocMetadata* getPrev();
        void setPrev(MallocMetadata* prev);

        MallocMetadata* getBuddy();
        void setBuddy(MallocMetadata* buddy);

        int getOrder(size_t realBlockSize);


        MallocMetadata(size_t size, MallocMetadata* prev);
        ~MallocMetadata() = default;
};

MallocMetadata* getLastMetadata(MallocMetadata* first);

MallocMetadata* getFirstMetadataBySize(MallocMetadata* first, size_t size);

MallocMetadata* getBestFitMetadata(MallocMetadata* first, size_t size);

#endif MALLOC_META_DATA