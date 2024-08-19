#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "MallocMetadata.h"



#define MAX_SIZE 100000000

MallocMetadata* firstMetadata = NULL;


/*****************************STATS METHODS******************************/

size_t _num_free_blocks(){
    size_t count =0;
    MallocMetadata* current = firstMetadata;
    while(current){
        if(current->getFree()){
            count++;
        }
        current = current->getNext();
    }
    return count;

}

size_t _num_free_bytes(){
    size_t size = 0;
    MallocMetadata* current = firstMetadata;
    while(current){
        if(current->getFree()){
            size += current->getSize();
        }
        current = current->getNext();
    }
    return size;//8 bytes in size_t, do we need to multiply? i think not cause the value in the MMD is per bytes
}

size_t _num_allocated_blocks(){
    size_t count =0;
    MallocMetadata* current = firstMetadata;
    while(current){
        count++;
        current = current->getNext();
    }
    return count;
}

size_t _num_allocated_bytes(){
    size_t size = 0;
    
    MallocMetadata* current = firstMetadata;
    while(current){
        size += current->getSize();
        current = current->getNext();
    }
    return size;
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes(){
    return _num_allocated_blocks() * _size_meta_data();
    
}



/******************MALLOCS AND SHIT*************************************/


void* smalloc(size_t size){
    if(size == 0 || size > MAX_SIZE) {
        return NULL;
    }

    
    if(firstMetadata == NULL){
        void* blockToAllocate = sbrk(size +_size_meta_data());
        if( blockToAllocate == (void*)(-1)){
            return NULL;
        }
        ((MallocMetadata*) blockToAllocate)->setSize(size);
        ((MallocMetadata*) blockToAllocate)->setFree(false);
        ((MallocMetadata*) blockToAllocate)->setPrev(NULL);
        ((MallocMetadata*) blockToAllocate)->setNext(NULL);
        firstMetadata = (MallocMetadata*) blockToAllocate;
        return (void*)((char*)blockToAllocate + _size_meta_data());
    } 

    MallocMetadata* current = firstMetadata;
    
    if(_num_free_blocks() != 0 && _num_free_bytes() >= size){
        MallocMetadata* validMetadata = getLastMetadataBySize(firstMetadata, size);
        if(validMetadata != NULL){
            validMetadata -> setFree(false);
                    return (void*)((char*)validMetadata + _size_meta_data());

        }
    }   
    MallocMetadata* lastMetadata = getLastMetadata(firstMetadata); 
    void* blockToAllocate = sbrk(size +_size_meta_data());
    if( blockToAllocate == (void*)(-1)){
        return NULL;
    }
    ((MallocMetadata*) blockToAllocate)->setSize(size);
    ((MallocMetadata*) blockToAllocate)->setFree(false);
    ((MallocMetadata*) blockToAllocate)->setPrev(lastMetadata);
    lastMetadata->setNext((MallocMetadata*)blockToAllocate);
    ((MallocMetadata*) blockToAllocate)->setNext(NULL);
    return (void*)((char*)blockToAllocate + _size_meta_data());
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

    // Set all bytes to 0
    memset(ptr, 0, total_size);

    return ptr;
}




void sfree(void* p){
    if(p == NULL){
        return;
    }
    MallocMetadata* Metadata = (MallocMetadata*)(((char *)p) - (_size_meta_data()));
    Metadata->setFree(true);
}




void* srealloc(void* oldp, size_t size){
    if(size == 0 || size >= MAX_SIZE){/////////////////////////////// the tests think its equal or bigger but i think thats a mistake
        return NULL;
    }
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




void* GET_METADATA_PTR(void* ptr){
    return ((char*)ptr - _size_meta_data());
}

#include <iostream>
#include <cassert>
using namespace std;
int md_size = sizeof(MallocMetadata);

#define RUN_TEST(test) do { \
    std::cout << "Running " << #test << "... "; \
    if (test()) { \
        std::cout << "[PASS]" << std::endl; \
    } else { \
        std::cout << "[FAIL]" << std::endl; \
    } \
} while(0)


// Helper function to reset allocator state (if needed)
void reset_allocator() {
    //blocks = BlocksList();
    firstMetadata = NULL;
}

bool test_smalloc_basic() {
    reset_allocator();
    void* ptr = smalloc(10);
    assert(ptr != nullptr);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 10);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    return true;
}

bool test_smalloc_zero_size() {
    reset_allocator();
    void* ptr = smalloc(0);
    assert(ptr == nullptr);
    return true;
}

bool test_smalloc_large_size() {
    reset_allocator();
    void* ptr = smalloc(MAX_SIZE + 1);
    assert(ptr == nullptr);
    return true;
}

bool test_scalloc_basic() {
    reset_allocator();
    void* ptr = scalloc(5, 4);
    assert(ptr != nullptr);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 20);

    // Check if memory is zeroed
    char* char_ptr = static_cast<char*>(ptr);
    for (int i = 0; i < 20; ++i) {
        assert(char_ptr[i] == 0);
    }
    return true;
}

bool test_sfree_basic() {
    reset_allocator();
    void* ptr = smalloc(10);
    assert(ptr != nullptr);
    sfree(ptr);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 10);
    return true;
}

bool test_srealloc_grow() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    assert(ptr1 != nullptr);
    void* ptr2 = srealloc((void*)(ptr1), 20);
    assert(ptr2 != nullptr);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 30);
    return true;
}

bool test_srealloc_shrink() {
    reset_allocator();
    void* ptr1 = smalloc(20);
    assert(ptr1 != nullptr);
    void* ptr2 = srealloc(ptr1, 10);
    assert(ptr1 == ptr2);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 20);  // Size should not change when shrinking
    return true;
}

bool test_multiple_allocations() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    void* ptr2 = smalloc(20);
    void* ptr3 = smalloc(30);
    assert(ptr1 != nullptr && ptr2 != nullptr && ptr3 != nullptr);
    assert(_num_allocated_blocks() == 3);
    assert(_num_allocated_bytes() == 60);
    return true;
}

bool test_allocate_freed_block() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    assert(ptr1 != nullptr);
    sfree(ptr1);
    void* ptr2 = smalloc(10);
    assert(ptr1 == ptr2);
    assert(_num_allocated_blocks() == 1);
    assert(_num_free_blocks() == 0);
    return true;
}

bool test_meta_data_size() {
    reset_allocator();
    size_t metaDataSize = _size_meta_data();
    assert(metaDataSize > 0);
    void* ptr = smalloc(10);
    assert(ptr != nullptr);
    assert(_num_meta_data_bytes() == metaDataSize);
    return true;
}
bool test_fragmentation() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    void* ptr2 = smalloc(20);
    void* ptr3 = smalloc(30);
    sfree(ptr2);
    void* ptr4 = smalloc(15);
    assert(ptr4 == ptr2);
    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    return true;
}

bool test_realloc_complex() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    void* ptr2 = smalloc(20);
    void* ptr3 = smalloc(30);
    sfree(ptr2);
    void* ptr4 = srealloc(ptr1, 25);
    assert(ptr4 == ((char*)ptr3 + 30 + md_size));
    assert(_num_allocated_blocks() == 4);
    assert(_num_free_blocks() == 2);
    assert(_num_free_bytes() == 30);
    return true;
}

bool test_stress() {
    reset_allocator();
    const int NUM_ALLOCS = 1000;
    void* ptrs[NUM_ALLOCS];

    for (int i = 0; i < NUM_ALLOCS; ++i) {
        ptrs[i] = smalloc((i % 100) + 1);
        assert(ptrs[i] != nullptr);
    }

    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        sfree(ptrs[i]);
    }

    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        ptrs[i] = smalloc((i % 100) + 1);
        assert(ptrs[i] != nullptr);
    }

    assert(_num_allocated_blocks() == NUM_ALLOCS);
    assert(_num_free_blocks() == 0);
    return true;
}

bool test_scalloc_large() {
    reset_allocator();
    void* ptr = scalloc(10000, 10001);
    assert(ptr == nullptr);
    assert(_num_allocated_blocks() == 0);
    return true;
}

bool test_srealloc_null() {
    reset_allocator();
    void* ptr = srealloc(nullptr, 10);
    assert(ptr != nullptr);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 10);
    return true;
}

bool test_integration_all_functions() {
    reset_allocator();
    void* ptr1 = smalloc(10);
    void* ptr2 = scalloc(5, 4);
    void* ptr3 = smalloc(30);

    sfree(ptr2);

    void* ptr4 = srealloc(ptr1, 20);
    assert(ptr4 == ptr2);

    void* ptr5 = smalloc(10);
    assert(ptr5 == ptr1);

    sfree(ptr2);
    sfree(ptr3);

    void* ptr6 = smalloc(25);
    assert(ptr6 == ptr3);
    void* ptr7 = smalloc(18);
    assert(ptr7 == ptr2);

    assert(_num_allocated_blocks() == 3);
    assert(_num_free_blocks() == 0);
    assert(_num_allocated_bytes() == 60);
    assert(_num_free_bytes() == 0);

    return true;
}

bool smalloc_unit_test() {
    reset_allocator();
    assert(_size_meta_data() == md_size);

    void* initial = sbrk(0);
    void* ptr = smalloc(10);
    assert((char *)ptr - (char*)initial == md_size);
    MallocMetadata* block = (MallocMetadata*)initial;
    assert(block->getSize() == 10);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 10);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == md_size);
    assert((char*)sbrk(0) - (char*)ptr == 10);
    assert((char*)sbrk(0) - (char*)initial == 10 + md_size);

    void* ptr2 = smalloc(20);
    block = (MallocMetadata*)((char*) ptr2 - md_size);
    assert((char*)ptr2 - (char*)ptr == md_size + 10);
    assert((char*)ptr2 - (char*)initial == md_size + md_size + 10);
    assert(block->getSize() == 20);
    assert(block->getFree() == false);
    if(firstMetadata->getNext() == NULL){
        std::cout << std::endl << "its null and its size is:" << firstMetadata->getSize() << std::endl;
    }
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 30);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == 2 * md_size);
    assert((char*)sbrk(0) - (char*)ptr2 == 20);
    assert((char*)sbrk(0) - (char*)initial == 20 + 10 + (2 * md_size));
    return true;
}

bool scalloc_unit_test() {
    reset_allocator();
    void* initial = sbrk(0);
    void* ptr = scalloc(20, 4);
    char* zeros = static_cast<char*>(ptr);
    for (int i = 0; i < (20 * 4) - sizeof(MallocMetadata); ++i) {
        assert(zeros[i] == 0);
    }
    MallocMetadata* block = (MallocMetadata*) ((char*) ptr - sizeof(MallocMetadata));
    assert((char*)ptr - (char*)initial == md_size);
    assert(block->getSize() == 80);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 80);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == md_size);
    assert((char*)sbrk(0) - (char*)ptr == 20 * 4);
    return true;
}

bool test_scalloc_zeros() {
    reset_allocator();
    void* ptr = scalloc(5, 20);
    char* ptr_data = (char*) ptr;
    for (int i = 0; i < 100; ++i) {
        assert(ptr_data[i] == 0);
    }
    MallocMetadata* block = (MallocMetadata*) GET_METADATA_PTR(ptr);
    assert((char*)ptr - (char*)block == md_size);
    assert(block->getSize() == 100);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 100);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == md_size);
    assert((char*)sbrk(0) - (char*)ptr == 20 * 5);
    return true;
}

bool sfree_unit_test() {
    reset_allocator();
    void* initial = sbrk(0);
    void* ptr = smalloc(26);

    sfree(ptr);
    assert((char*)sbrk(0) - (char*)initial == md_size + 26);
    MallocMetadata* block = (MallocMetadata*)initial;
    assert(block->getSize() == 26);
    assert(block->getFree() == true);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 26);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 26);
    assert(_num_meta_data_bytes() == md_size);

    // inserting another one in the same block
    void* ptr2 = smalloc(25);
    assert((char *)ptr2 - (char*)initial == md_size);
    block = (MallocMetadata*)initial;
    assert(block->getSize() == 26);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 26);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == md_size);
    return true;
}

bool srealloc_unit_test() {
    reset_allocator();
    void* ptr = scalloc(30,3);
    void* ptr2 = srealloc(ptr, 80);
    assert(ptr == ptr2);
    MallocMetadata* block = (MallocMetadata*) GET_METADATA_PTR(ptr);
    assert(block->getSize() == 90);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 1);
    assert(_num_allocated_bytes() == 90);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == md_size);

    void* ptr3 = srealloc(ptr2, 120);
    assert(block->getFree() == true);
    assert(ptr3 == (void*)((char*)ptr + 90 + md_size));
    block = (MallocMetadata*) GET_METADATA_PTR(ptr3);
    assert(block->getSize() == 120);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 210);
    assert(_num_free_blocks() == 1);
    assert(_num_free_bytes() == 90);
    assert(_num_meta_data_bytes() == 2 * md_size);

    // this will go to block 1 which is free since realloc
    void* ptr4 = srealloc(nullptr, 20);
    assert(ptr4 == ptr);
    assert(block->getFree() == false);
    block = (MallocMetadata*) GET_METADATA_PTR(ptr4);
    assert(block->getSize() == 90);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 210);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == 2 * md_size);

    void* ptr5 = srealloc(ptr4, MAX_SIZE);
    assert(ptr5 == nullptr);
    assert(block->getFree() == false);
    assert(block->getSize()  == 90);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 210);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == 2 * md_size);

    void* ptr6 = srealloc(ptr4, 0);
    assert(ptr6 == nullptr);
    assert(block->getFree() == false);
    assert(block->getSize()  == 90);
    assert(block->getFree() == false);
    assert(_num_allocated_blocks() == 2);
    assert(_num_allocated_bytes() == 210);
    assert(_num_free_blocks() == 0);
    assert(_num_free_bytes() == 0);
    assert(_num_meta_data_bytes() == 2 * md_size);

    return true;
}

bool test_scalloc_data_move() {
    reset_allocator();
    // set 80 bytes where data[i]=i
    void* ptr1 = smalloc(80);
    char* ptr1_data = (char*) ptr1;
    for (int i = 0; i < 80; ++i) {
        ptr1_data[i] = i;
    }

    void* ptr2 = scalloc(5, 15);
    char* ptr2_data = (char*) ptr2;
    for (int i = 0; i < 75; ++i) {
        assert(ptr2_data[i] == 0);
    }

    sfree(ptr1);
    void* ptr3 = srealloc(ptr2, 80);
    assert(ptr3 == ptr1);
    for (int i = 0; i < 75; ++i) {
        cout << i << endl;
        assert(ptr1_data[i] == 0);
    }
    for (int i = 75; i < 80; ++i) {
        assert(ptr1_data[i] == i);
    }
    return true;
}

bool test_smalloc_data_move() {
    reset_allocator();
    // set 80 bytes where data[i]=i
    void* ptr1 = smalloc(80);
    char* ptr1_data = (char*) ptr1;
    for (int i = 0; i < 80; ++i) {
        ptr1_data[i] = i;
    }

    void* ptr2 = scalloc(5, 20);
    char* ptr2_data = (char*) ptr2;
    for (int i = 0; i < 100; ++i) {
        assert(ptr2_data[i] == 0);
    }
    sfree(ptr2);
    // check that data wasn't changed
    for (int i = 0; i < 100; ++i) {
        assert(ptr2_data[i] == 0);
    }

    // reallocating 1st block to size 90 will move it to 2nd block (freed). checking data movement
    void* ptr3 = srealloc(ptr1, 90);
    assert(ptr3 == ptr2);
    for (int i = 0; i < 80; ++i) {
        assert(ptr2_data[i] == i);
    }
    // checking that padding left from scalloc is still nullified
    for (int i = 80; i < 100; ++i) {
        assert(ptr2_data[i] == 0);
    }
    return true;
}

int main() {
    RUN_TEST(smalloc_unit_test);
    RUN_TEST(scalloc_unit_test);
    RUN_TEST(test_scalloc_zeros);
    RUN_TEST(sfree_unit_test);
    RUN_TEST(srealloc_unit_test);
    RUN_TEST(test_scalloc_data_move);
    RUN_TEST(test_smalloc_data_move);

    RUN_TEST(test_smalloc_basic);
    RUN_TEST(test_smalloc_zero_size);
    RUN_TEST(test_smalloc_large_size);
    RUN_TEST(test_scalloc_basic);
    RUN_TEST(test_sfree_basic);
    RUN_TEST(test_srealloc_grow);
    RUN_TEST(test_srealloc_shrink);
    RUN_TEST(test_multiple_allocations);
    RUN_TEST(test_allocate_freed_block);
    RUN_TEST(test_meta_data_size);
    RUN_TEST(test_fragmentation);
    RUN_TEST(test_realloc_complex);
    RUN_TEST(test_stress);
    RUN_TEST(test_scalloc_large);
    RUN_TEST(test_srealloc_null);
    RUN_TEST(test_integration_all_functions);

    cout << "All tests completed." << endl;
    return 0;
}

