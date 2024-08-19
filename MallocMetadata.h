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
        int getOrder(size_t realBlockSize);

        MallocMetadata(size_t size, MallocMetadata* prev);
        ~MallocMetadata() = default;
};


MallocMetadata* getLastMetadata(MallocMetadata* current);

MallocMetadata* getLastMetadataBySize(MallocMetadata* current, size_t size);
