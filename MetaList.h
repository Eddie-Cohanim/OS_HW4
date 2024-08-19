#include "MallocMetadata.h"
#define MAX_ORDER 10


class MetaList {
    private:
    MallocMetadata* m_list;
    
    public:
        MetaList();
        void addBlock(MallocMetadata* Block);
        MallocMetadata* splitBlock(MallocMetadata* block, size_t requestedSize);
        MallocMetadata* getBuddy(MallocMetadata* block);
        void mergeBuddyBlocks(MallocMetadata* block);

      
};