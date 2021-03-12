#include "mm.h"      // prototypes of functions implemented in this file

#include "memlib.h"  // mem_sbrk -- to extend the heap
#include <string.h>  // memcpy -- to copy regions of memory

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))

/**
 * A block header uses 4 bytes for:
 * - a block size, multiple of 8 (so, the last 3 bits are always 0's)
 * - an allocated bit (stored as LSB, since the last 3 bits are needed)
 *
 * A block footer has the same format.
 * Check Figure 9.48(a) in the textbook.
 */
typedef int BlockHeader;

/**
 * Read the size field from a block header (or footer).
 *
 * @param bp address of the block header (or footer)
 * @return size in bytes
 */
static int get_size(BlockHeader *bp) {
    return (*bp) & ~7;  // discard last 3 bits
}

/**
 * Read the allocated bit from a block header (or footer).
 *
 * @param bp address of the block header (or footer)
 * @return allocated bit (either 0 or 1)
 */
static int get_allocated(BlockHeader *bp) {
    return (*bp) & 1;   // get last bit
}

/**
 * Write the size and allocated bit of a given block inside its header.
 *
 * @param bp address of the block header
 * @param size size in bytes (must be a multiple of 8)
 * @param allocated either 0 or 1
 */
static void set_header(BlockHeader *bp, int size, int allocated) {
    *bp = size | allocated;
}

/**
 * Write the size and allocated bit of a given block inside its footer.
 *
 * @param bp address of the block header
 * @param size size in bytes (must be a multiple of 8)
 * @param allocated either 0 or 1
 */
static void set_footer(BlockHeader *bp, int size, int allocated) {
    char *footer_addr = (char *)bp + get_size(bp) - 4;
    // the footer has the same format as the header
    set_header((BlockHeader *)footer_addr, size, allocated);
}

/**
 * Find the payload starting address given the address of a block header.
 *
 * The block header is 4 bytes, so the payload starts after 4 bytes.
 *
 * @param bp address of the block header
 * @return address of the payload for this block
 */
static char *get_payload_addr(BlockHeader *bp) {
    return (char *)(bp + 1);
}

/**
 * Find the header address of the previous block on the heap.
 *
 * @param bp address of a block header
 * @return address of the header of the previous block
 */
static BlockHeader *get_prev(BlockHeader *bp) {
    // move back by 4 bytes to find the footer of the previous block
    BlockHeader *previous_footer = bp - 1; //move back 4 bytes
    int previous_size = get_size(previous_footer); //get prev size
    char *previous_addr = (char *)bp - previous_size;
    return (BlockHeader *)previous_addr;
}

/**
 * Find the header address of the next block on the heap.
 *
 * @param bp address of a block header
 * @return address of the header of the next block
 */
static BlockHeader *get_next(BlockHeader *bp) {
    int this_size = get_size(bp);
     // TODO: to implement, look at get_prev
    char *next_addr = (char *)bp+this_size; //set next address
    return (BlockHeader *)next_addr;
}

/**
 * In addition to the block header with size/allocated bit, a free block has
 * pointers to the headers of the previous and next blocks on the free list.
 *
 * Pointers use 4 bytes because this project is compiled with -m32.
 * Check Figure 9.48(b) in the textbook.
 */
typedef struct {
    BlockHeader header;
    BlockHeader *prev_free;
    BlockHeader *next_free;
} FreeBlockHeader;

/**
 * Find the header address of the previous **free** block on the **free list**.
 *
 * @param bp address of a block header (it must be a free block)
 * @return address of the header of the previous free block on the list
 */
static BlockHeader *get_prev_free(BlockHeader *bp) {
    FreeBlockHeader *fp = (FreeBlockHeader *)bp;
    return fp->prev_free;
}

/**
 * Find the header address of the next **free** block on the **free list**.
 *
 * @param bp address of a block header (it must be a free block)
 * @return address of the header of the next free block on the list
 */
static BlockHeader *get_next_free(BlockHeader *bp) {
    FreeBlockHeader *fp = (FreeBlockHeader *)bp;
    return fp->next_free;
}

/**
 * Set the pointer to the previous **free** block.
 *
 * @param bp address of a free block header
 * @param prev address of the header of the previous free block (to be set)
 */
static void set_prev_free(BlockHeader *bp, BlockHeader *prev) {
    FreeBlockHeader *fp = (FreeBlockHeader *)bp;
    fp->prev_free = prev;
}

/**
 * Set the pointer to the next **free** block.
 *
 * @param bp address of a free block header
 * @param next address of the header of the next free block (to be set)
 */
static void set_next_free(BlockHeader *bp, BlockHeader *next) {
    FreeBlockHeader *fp = (FreeBlockHeader *)bp;
    fp->next_free = next;
}

/* Pointer to the header of the first block on the heap */
static BlockHeader *heap_blocks;

/* Pointers to the headers of the first and last blocks on the free list */
static BlockHeader *free_headp;
static BlockHeader *free_tailp;

/**
 * Add a block at the beginning of the free list.
 *
 * @param bp address of the header of the block to add
 */
static void free_list_prepend(BlockHeader *bp) {
    // TODO: implement
    if((free_headp!=NULL) || (free_tailp!=NULL)){ //if there is element in the list
        set_prev_free(bp,NULL); //set the previous for the newly added to null
        set_next_free(bp,free_headp); //set the next for the newly added to head, which was the first
        set_prev_free(free_headp,bp); //set the previous of the head to newly added block
        free_headp = bp; //now the head is the newly added
    }
    else if((free_headp==NULL) && (free_tailp==NULL)) { //if there is no element in the list
        set_prev_free(bp, NULL); //previous of bp is null
        set_next_free(bp,NULL); //next of bp is nul
        free_headp = bp; //head is bp
        free_tailp = bp; //tail is bp
    }
}

/**
 * Add a block at the end of the free list.
 *
 * @param bp address of the header of the block to add
 */
static void free_list_append(BlockHeader *bp) {
    // TODO: implement
    if((free_headp!=NULL) || (free_tailp!=NULL)){ //if there is element in the list    
        set_next_free(bp,NULL); //set next of the bp to null
        set_next_free(free_tailp,bp); //set the next of the tail to bp
        set_prev_free(bp,free_tailp); //set prev of the bp to tail
        free_tailp = bp; //new tail is now bp
    }
    else if((free_headp==NULL) && (free_tailp==NULL)) { //if there is no element in the list
        set_prev_free(bp, NULL);
        set_next_free(bp,NULL);
        free_headp = bp;
        free_tailp = bp;
    }
}

/**
 * Remove a block from the free list.
 *
 * @param bp address of the header of the block to remove
 */
static void free_list_remove(BlockHeader *bp) {
    // TODO: implement
    if((get_prev_free(bp)==NULL) && (get_next_free(bp)!=NULL)){ //if bp is the first block
        set_prev_free(get_next_free(bp),NULL); //set previous of head to null
        free_headp = get_next_free(bp); //head is the next of bp 
        set_next_free(bp,NULL); //set next of bp to null 
    }
    else if((get_next_free(bp)==NULL) && (get_prev_free(bp)!=NULL)){ //if bp is tail
        set_next_free(get_prev_free(bp), NULL); //set next of the previous block of bp to null because that block is now the tail
        free_tailp = get_prev_free(bp); //set previous of the bp to tail
        set_prev_free(bp,NULL);//set previous of bp to null
    }
    else if((get_prev_free(bp)==NULL) && (get_next_free(bp)==NULL)){ // if there is one block remaining
        free_headp = NULL; //set head to null
        free_tailp = NULL; //set tail to null
        set_prev_free(bp,NULL); //set prev of bp to null
        set_next_free(bp,NULL); //set next of bp to null
    }
    else if((get_prev_free(bp)!=NULL) && (get_next_free(bp)!=NULL)){ //if bp is not the first block
        set_next_free(get_prev_free(bp),get_next_free(bp)); //connect previous block to next block of bp
        set_prev_free(get_next_free(bp),get_prev_free(bp)); //connect next block to preivous block of bp
    }
}

/**
 * Mark a block as free, coalesce with contiguous free blocks on the heap, add
 * the coalesced block to the free list.
 *
 * @param bp address of the block to mark as free
 * @return the address of the coalesced block
 */
static BlockHeader *free_coalesce(BlockHeader *bp) {
    // mark block as free
    int size = get_size(bp);
    set_header(bp, size, 0);
    set_footer(bp, size, 0);
    
    int nextSize=0;
    int prevSize=0;
    int totalSize=0;

    // check whether contiguous blocks are allocated
    int prev_alloc = get_allocated(get_prev(bp));
    int next_alloc = get_allocated(get_next(bp));

    if (prev_alloc && next_alloc) { //surrounded by allocated
        // TODO: add bp to free list
        if(size<1000){ //added condition to separate when to prepend and when to append
            free_list_prepend(bp); //prepend condition
        }
        else{
            free_list_append(bp); //append condition
        }
        return bp;

    } else if (prev_alloc && !next_alloc) { //when next is not allocated, coalesce with next
        // TODO: remove next block from free list
        free_list_remove(get_next(bp));
        
        // TODO: add bp to free list
        if(size<1000){ //added condition 
            free_list_prepend(bp); //prepend condition
        }
        else{
            free_list_append(bp); //append condition
        }
        // TODO: coalesce with next block
        nextSize = get_size(get_next(bp)); //get the size of the next 
        totalSize = size+nextSize; //new block total size
        set_header(bp, totalSize, 0); //set the header with new size, 0 for unallocated
        set_footer(bp, totalSize, 0); //set footer with new size, 0 foir unallocated
        return bp;
    } 
    else if (!prev_alloc && next_alloc) { //when prev is not allocated, coalesce with prev
        // TODO: coalesce with previous block
        prevSize = get_size(get_prev(bp)); //get size of the previous
        totalSize = size+prevSize;  //get total size
        //Don't have to add to the list just update header and footer 
        set_header(get_prev(bp),totalSize,0); //set the header of the previous to new total size
        set_footer(get_prev(bp),totalSize,0); //set the footer of the previous to new total size
        return get_prev(bp);
    }
    else { //both are not allocated - then remove the next and update the header and footer of the previous
        // TODO: remove next block from free list
        free_list_remove(get_next(bp)); 
        // TODO: coalesce with previous and next block
        nextSize = get_size(get_next(bp)); //get size of next
        prevSize = get_size(get_prev(bp)); //get size of prev
        totalSize = prevSize+size+nextSize; //total size of the new block (prev+mysize+next)
        set_header(get_prev(bp),totalSize,0); //set header of the previous to new total size
        set_footer(get_prev(bp),totalSize,0); //set footer of the previous to new total size
        return get_prev(bp);
    }
}

/**
 * Extend the heap with a free block of `size` bytes (multiple of 8).
 *
 * @param size number of bytes to allocate (a multiple of 8)
 * @return pointer to the header of the new free block
 */
static BlockHeader *extend_heap(int size) {
    // bp points to the beginning of the new block
    char *bp = mem_sbrk(size);
    if ((long)bp == -1)
        return NULL;
    // write header over old epilogue, then the footer
    BlockHeader *old_epilogue = (BlockHeader *)bp - 1;
    set_header(old_epilogue, size, 0);
    set_footer(old_epilogue, size, 0);
    // write new epilogue
    set_header(get_next(old_epilogue), 0, 1);
    // merge new block with previous one if possible
    return free_coalesce(old_epilogue);
}

int mm_init(void) {
    // init list of free blocks
    free_headp = NULL;
    free_tailp = NULL;
    
    // create empty heap of 4 x 4-byte words
    char *new_region = mem_sbrk(16);
    if ((long)new_region == -1)
        return -1;

    heap_blocks = (BlockHeader *)new_region;
    set_header(heap_blocks, 0, 0);      // skip 4 bytes for alignment
    set_header(heap_blocks + 1, 8, 1);  // allocate a block of 8 bytes as prologue
    set_footer(heap_blocks + 1, 8, 1);
    set_header(heap_blocks + 3, 0, 1);  // epilogue
    heap_blocks += 1;                   // point to the prologue header

    // TODO: extend heap with an initial heap size
    extend_heap(200);
    return 0;
}

void mm_free(void *bp) {
   // TODO: move back 4 bytes to find the block header, then free block
    BlockHeader* blockH; //new pointer
    blockH = (BlockHeader*)bp-1; //move back 4bytes
    free_coalesce(blockH);
}

/**
 * Find a free block with size greater or equal to `size`.
 *
 * @param size minimum size of the free block
 * @return pointer to the header of a free block or `NULL` if free blocks are
 *         all smaller than `size`.
 */
//BlockHeader* finder = free_headp;
static BlockHeader *find_fit(int size) {
    // TODO: implement
    //First Fit// 
    //Doesn't work
    /* while(finder){
        if(get_size(finder)>=size){
            return finder;
        }
        finder = get_next_free(finder);
    }*/
    //faster
    /*BlockHeader* ptr = free_tailp;
    while(ptr){
        int bSize = get_size(ptr);
        if(bSize<size){
            ptr = get_prev_free(ptr);
        }
        else if(get_size(ptr)>=size){
            return ptr;
        }
    }*/
    
    //better fit
    if(size>270){ //condition to split 
        BlockHeader* hptr = free_tailp; //if the size is big start from the tail
        while(hptr){ //loop to find 
            int bSize = get_size(hptr); //get the size for comparison
            //int min =1000000;
            if(bSize>=size){ //check is big enough
                if(get_prev_free(hptr)!=NULL){ //since starting from tail, check if there is block before
                    //BlockHeader* bptr = (BlockHeader*)get_prev_free(hptr); 
                    int nSize = get_size(get_prev_free(hptr)); //get the size of before block
                    if(nSize<bSize && nSize>=size ){ //if the before block is large enough but smaller 
                        return get_prev_free(hptr); //it is a better fit
                    }
                    else{
                        return hptr;
                    }
                }
                else{
                    return hptr;
                }
                //min = MIN(bSize,min);
            }
            else if(bSize<size){ //traverse
                hptr = get_prev_free(hptr);
            }
        }
    }
    else if(size<=270){ //start from the head
        BlockHeader* hptr = free_headp; 
        while(hptr){
            int bSize = get_size(hptr); //get the size
            //int min =1000000;
            if(bSize>=size){ //if the size is large enough
                if(get_next_free(hptr)!=NULL){ //since starting from head, check if it is not the last
                    //BlockHeader* bptr = (BlockHeader*)get_prev_free(hptr); 
                    int nSize = get_size(get_next_free(hptr)); //get the size of one after
                    if(nSize<bSize && nSize>=size ){ //check if size is big and smaller  
                        return get_next_free(hptr); //better fit
                    }
                    else{
                        return hptr;
                    }
                }
                else{
                    return hptr;
                }
                //min = MIN(bSize,min);
            }
            else if(bSize<size){
                hptr = get_next_free(hptr); //traverse
            }
        }
    }      
    return NULL; //return null when can not find.
}

/**
 * Allocate a block of `size` bytes inside the given free block `bp`.
 *
 * @param bp pointer to the header of a free block of at least `size` bytes
 * @param size bytes to assign as an allocated block (multiple of 8)
 * @return pointer to the header of the allocated block
 */
static BlockHeader *place(BlockHeader *bp, int size) {
    // TODO: if current size is greater, use part and add rest to free list
    int newSize = get_size(bp)-size; //new remaining size to put in free list
    free_list_remove(bp); //remove bp from freelist
    if(newSize<=8){
        //if size is too small for freelist
        set_header(bp,get_size(bp),1); //use all of the size to allocate
        set_footer(bp,get_size(bp),1);
    }
    else{
        if(size>25){ //check if size is greater than 25 than put it at the back 
            //back
            set_header(bp,newSize,0); //set new size and 0
            set_footer(bp,newSize,0); //set new size and 0
            set_header(get_next(bp),size,1); //getnext of bp and set allocated to size
            set_footer(get_next(bp),size,1); //set to allocated with size
            free_coalesce(bp); //check to coalsce
            return get_next(bp);
        }
        else if(size<=25){ //check if size is less than 25 than put it at the front
            //front
            set_header(bp,size,1); //set allocated with size
            set_footer(bp,size,1); //set allocated with size
            set_header(get_next(bp),newSize,0); //getnext of bp is the remaining block so set header
            set_footer(get_next(bp),newSize,0); //set footer of getnext of bp
            free_coalesce(get_next(bp)); //check to coalsce
            return bp;
        }
    }
    return bp;
}

/**
 * Compute the required block size (including space for header/footer) from the
 * requested payload size.
 *
 * @param payload_size requested payload size
 * @return a block size including header/footer that is a multiple of 8
 */
static int required_block_size(int payload_size) {
    payload_size += 8;                    // add 8 for for header/footer
    return ((payload_size + 7) / 8) * 8;  // round up to multiple of 8
}

void *mm_malloc(size_t size) {
    // ignore spurious requests
    if (size == 0)
        return NULL;

    int required_size = required_block_size(size);

    // TODO: find a free block or extend heap
    if(find_fit(required_size)){ //if findfit return other than null
        // TODO: allocate and return pointer to payload
        return get_payload_addr(place(find_fit(required_size),required_size)); //place it and return address
    }
    else if(!find_fit(required_size)){//if I need to extend heap, when findfit returned null
        while(1){ //loop until find fit finds a adequate size
            extend_heap(required_size);
            if(find_fit(required_size)){ //if findfit returned other than null
                // TODO: allocate and return pointer to payload
                return get_payload_addr(place(find_fit(required_size),required_size));
            }
        }
    }
    return NULL;
}

void *mm_realloc(void *ptr, size_t size) { 
    BlockHeader* hptr = (BlockHeader*)ptr-1; //set header
    size_t rSize = required_block_size(size); // get the required block size
    size_t size_block=get_size(hptr); //get size using the header 
    size_t sSize = size_block - rSize; // size of the subtracted
    size_t aSize = get_size(hptr) + get_size(get_next(hptr)); //add the neighboring size
    size_t newSize = aSize - rSize;
    if (hptr == NULL) {
        // equivalent to malloc
        return mm_malloc(size);
    }else if (size == 0) {
        // equivalent to free
        mm_free(hptr);
        return NULL;
    } else {
         if (size_block<rSize){ //when the size is smaller than the required size
            if(!get_allocated(get_next(hptr))){     
                if(aSize>=rSize){ //if the added size is greater than the required size
                    free_list_remove(get_next(hptr)); //get it out from the list
                    if(newSize<=250){ //set condition for size
                        set_header(hptr,aSize,1); //set allocated
                        set_footer(hptr,aSize,1); //set allocated 
                    }else{ 
                        set_header(hptr,rSize,1); //set allocated
                        set_footer(hptr,rSize,1); //set allocated
                        set_header(get_next(hptr),newSize,0); //getnext of hptr is the remaining block so set header
                        set_footer(get_next(hptr),newSize,0); //set footer of getnext of hptr
                        free_coalesce(get_next(hptr)); //check to coalsce
                    }
                    return get_payload_addr(hptr);
                }
            }
            void*new_ptr = mm_malloc(size);
            memcpy(new_ptr,ptr,MIN(size,(unsigned)get_size(ptr-4)-8));
            mm_free(ptr);
            return new_ptr;
        }
        else if(size_block>=rSize){
            if (sSize>250){
                set_header(hptr,rSize,1); //set allocated
                set_footer(hptr,rSize,1); //set allocated
                set_header(get_next(hptr),sSize,0); //getnext of ptr is the remaining block so set header
                set_footer(get_next(hptr),sSize,0); //set footer of getnext of hptr
                free_coalesce(get_next(hptr)); //check to coalsce
            }
            return get_payload_addr(hptr);
        }
       /*case find largest free block <=size ->resize
         
        case size is larger check adjacent -> coalesce

        case allocate new block */
    }
    return NULL;
}
    

