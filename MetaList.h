#ifndef META_LIST
#define META_LIST


#include "MallocMetadata.h"
#define MAX_ORDER 10


class MetaList {
    private:
    MallocMetadata* m_list [MAX_ORDER + 1];//cause you have both order 0 and order 10
    
    public:
        MetaList();

        void addBlock(MallocMetadata* Block);
        MallocMetadata* splitBlock(MallocMetadata* block, size_t requestedSize);
        MallocMetadata* getBuddy(MallocMetadata* block);
        void mergeBuddyBlocks(MallocMetadata* block);

      
};

#endif META_LIST