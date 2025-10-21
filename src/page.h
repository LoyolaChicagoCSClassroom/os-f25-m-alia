#ifndef PAGE_H
#define PAGE_H


struct ppage{
	struct ppage *next;
	struct ppage *prev;
	void *physical_addr;
};

extern struct ppage *free_page_list;

void init_pfa_list(void);
struct ppage *allocate_physical_pages(unsigned int npages);
void free_physical_pages(struct ppage *ppage_list);

#endif


