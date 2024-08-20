#ifndef META_LIST
#define META_LIST


#include "MallocMetadata.h"
#define MAX_ORDER 10


class MetaList {
    private:
        MallocMetadata* m_list [MAX_ORDER + 1];//cause you have both order 0 and order 10
    
    public:
        MetaList();
        MallocMetadata** getList();
        void makeArrayNull();

        void addBlock(MallocMetadata* block);
        void removeBlock(MallocMetadata* block, bool forMerge);
        MallocMetadata* splitBlock(MallocMetadata* block);
        MallocMetadata* getBuddy(MallocMetadata* block);
        MallocMetadata* mergeBuddyBlocks(MallocMetadata* block);
        void initialize(size_t sizeOfArray);

      
};

#endif META_LIST