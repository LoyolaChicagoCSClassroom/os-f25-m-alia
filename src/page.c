#include <stdio.h>
#include "page.h"
#include <stddef.h>
#define PAGENUMBER 128

struct ppage physical_page_array[128]; // 128 pages, each 2mb in length covers 256 megs of memory
struct ppage *free_page_list = NULL;

void init_pfa_list(void) {
	
	free_page_list = &physical_page_array[0];
	int i = 0;
	for (i = 0; i < PAGENUMBER; i++) {
		physical_page_array[i].physical_addr = (void *)(i * 0x200000); // 2MB apart

		if (i == 0) {
			physical_page_array[i].prev = NULL;
		} else {
			physical_page_array[i].prev = &physical_page_array[i - 1];
		}

		if (i == PAGENUMBER - 1) {
			physical_page_array[i].next = NULL;
		} else {
			physical_page_array[i].next = &physical_page_array[i + 1];
		}
	}
}

struct ppage *allocate_physical_pages(unsigned int npages) {
	struct ppage *allocated_head;
	struct ppage *curr;
	struct ppage *next_free;

	if (npages == 0) {
		return NULL;
	}

	if (free_physical_pages == NULL) {
		return NULL;
	}

	allocated_head = free_page_list;
	curr = allocated_head;
	
	for (int i = 0; i < npages - 1; i++) {
		if (curr->next == NULL) {
			break;
		}
		curr = curr->next;
	}

	if (next_free != NULL) {
		next_free->prev = NULL;
	}
	
	curr->next = NULL;
	free_page_list = next_free;

	return allocated_head;
}

void free_physical_pages(struct ppage *ppage_list) {
	struct ppage *last;
	struct ppage *curr;

	if (ppage_list == NULL) {
		return;
	}

	last = ppage_list;
	while (last->next != NULL) {
		last = last->next;
	}

	curr = free_page_list;
	last->next = curr;

	if (curr != NULL) {
		curr->prev = last;
	}

	ppage_list->prev = NULL;
	free_page_list = ppage_list;
}




