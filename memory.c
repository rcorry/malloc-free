#include <stdio.h>
#include <sys/mman.h>

typedef struct __node_t {
    size_t size;
    struct __node_t *next;
} node_t;

typedef struct __header_t{
    size_t size;
    int magic;
} header_t;

//Initialze persistant variables
const int MAGIC = 123456789;
node_t *head = NULL;

void* my_malloc(size_t size){
    node_t *largest = head;
    node_t *cur = head;

    //round up to next factor of 8 size = (size + 7) & (-8);
    size = (size + 7) & (-8);

    //finds largest chunk of memory
    while(cur){
        if(cur->size > largest->size){
            largest = cur;
        }
        cur = cur->next;
    }

    //checks if there is enough space to allocate
    if(largest->size + sizeof(node_t) < size){
        printf("NO CHUNCK LARGE ENOUGH\n");
        return NULL;
    }

    //frees up a chunk and returns a ptr to the chunk
    largest->size -= size + sizeof(header_t);
    header_t *chunk = (void*)largest + sizeof(node_t) + largest->size;
    chunk->size = size;
    chunk->magic = MAGIC;
    return chunk;
}

void my_free(header_t *allocated){
    node_t *cur = head;
    node_t *prev = NULL;

    //check if magic numbers matches
    if(MAGIC != allocated->magic){
        printf("MAGIC NUMBERS DON'T MATCH");
        return;
    }

    //find block to reallocate
    while(cur->next){
        if((void*)cur < (void*)allocated && (void*)cur->next > (void*)allocated){
            break;
        }
        cur = cur->next;
    }

    //checks if allocated block is on right of free block
    if(((void*)cur + cur->size + sizeof(node_t)) == (void*)allocated){
        size_t size = allocated->size;
        allocated = NULL;
        cur->size += size + sizeof(header_t);
        if(((void*) cur->next == ((void*)cur + sizeof(node_t) + cur->size))){
            node_t *tmp_next = cur->next;
            cur->size += cur->next->size + sizeof(node_t);
            cur->next = tmp_next->next;
        }
        return;
    }
    //checks if allocated block is on left of free block
    else if(((void*)allocated + allocated->size + sizeof(header_t)) == (void*)cur->next){
        size_t size = allocated->size;
        node_t *tmp_next = (void*)allocated;
        allocated = NULL;
        tmp_next->size = size + 2*sizeof(header_t) + cur->next->size;
        tmp_next->next = cur->next->next;
        cur->next = tmp_next;
        return;

    }
    //allocated block is between two free blocks
    else{
        node_t *new_node = (void*)allocated;
        size_t size = allocated->size;
        allocated = NULL;
        new_node->size = size;
        node_t *tmp_next = cur->next;
        cur->next = new_node;
        new_node->next = tmp_next;
        return;
    }
}


void print_free_list(){
    printf("HEAD: %ld ", (void*)head-(void*)head);
    printf("END: %zu SIZE: %zu\n",head->size + sizeof(node_t), head->size);
    node_t *cur = head->next;
    node_t *last= head;
    while(cur){
        printf("-----------------------------\n\n");
        printf("ALLOCATED BLOCK OF SIZE: %zu\n\n", (void*)cur-((void*)last+sizeof(node_t)+last->size));
        printf("-----------------------------\n");
        printf("START: %ld END: %zu SIZE: %zu\n", (void*)cur-(void*)head,(void*)cur-(void*)head + cur->size + sizeof(node_t),  cur->size);
        last = cur;
        cur = cur->next;
    }
    printf("\n");
}

void init(){
    head = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    head->size = 4096-sizeof(node_t);
    head->next = NULL;
}

void test_1(){
    printf("TEST 1: Free blocks are reused\n\n");

    printf("Free List Before Allocation\n");
    print_free_list();

    header_t *a = my_malloc(8);
    printf("Free list after allocating a block of size 8\n"); 
    print_free_list();

    my_free(a);
    printf("Free list after freeing up the previous block\n");
    print_free_list();

    a = my_malloc(8);
    printf("Free list after reallocating a block of size 8\n");
    print_free_list();
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");
}

void test_2(){
    printf("TEST 2: Blocks are split and freed correctly\n\n");

    printf("\nFree List Before Allocation\n");
    print_free_list();

    header_t *a = my_malloc(8);
    header_t *b = my_malloc(8);
    header_t *c = my_malloc(8);
    header_t *d = my_malloc(8);
    printf("\nFree list after allocating 4 blocks of size 8\n"); 
    print_free_list();


    my_free(a);
    my_free(c);
    printf("\nFree list after freeing up 2 blocks of size 8 showing 3 nodes on the free list\n"); 
    print_free_list();

    my_free(b);
    printf("\nFree list after freeing up the allocated block towards the end of size 8, leaving 2 entries on the free list\n"); 
    print_free_list();

    my_free(d);
    printf("\nFree list after freeing up the last allocated block of size 8, leaving 1 entry on the free list\n"); 
    print_free_list();
}

void test_3(){
    printf("TEST 3: Worst Fit\n\n");

    header_t *a = my_malloc(500);
    header_t *b = my_malloc(8);
    header_t *c = my_malloc(2000);
    header_t *d = my_malloc(8);
    my_free(a);
    my_free(c);
    printf("\nFree list after segmenting into 3 blocks of various sizes\n"); 
    print_free_list();

    header_t *e = my_malloc(64);
    printf("\nFree list after allocating a block of size 64\n"); 
    print_free_list();





}

int main(){
    getchar();
    
    init();
    test_1();
    getchar();

    init();
    test_2();
    getchar();

    init();
    test_3();
    return 0;
}

//TESTS
/*
Free blocks are reused
Blocks are split and freed correctly
Free list is in sorted order includes all free blocks
The heap is always alternating between free and allocated
Show Worst fit
*/