#include "malloc.h"
#include "string.h"
#include "errno.h"

typedef struct hdr { struct hdr* next; size_t size; int free; } hdr_t;
static hdr_t* free_list = 0;
static void* heap_start = 0;
static size_t heap_size = 0;

void malloc_init(void* base, size_t size){
    heap_start = base; heap_size = size;
    free_list = (hdr_t*)base;
    free_list->next = NULL;
    free_list->size = size - sizeof(hdr_t);
    free_list->free = 1;
}

void* malloc(size_t s){
    hdr_t* cur = free_list;
    while(cur){
        if(cur->free && cur->size >= s){
            if(cur->size > s + sizeof(hdr_t) + 8){
                // split
                hdr_t* new = (hdr_t*)((char*)cur + sizeof(hdr_t) + s);
                new->size = cur->size - s - sizeof(hdr_t);
                new->free = 1;
                new->next = cur->next;
                cur->next = new;
                cur->size = s;
            }
            cur->free = 0;
            return (char*)cur + sizeof(hdr_t);
        }
        cur = cur->next;
    }
    errno = ENOMEM; return NULL;
}

void free(void* p){
    if(!p) return;
    hdr_t* h = (hdr_t*)((char*)p - sizeof(hdr_t));
    h->free = 1;
    // coalesce
    hdr_t* cur = free_list;
    while(cur){
        if(cur->free && cur->next && cur->next->free){
            cur->size += sizeof(hdr_t) + cur->next->size;
            cur->next = cur->next->next;
        } else cur = cur->next;
    }
}
