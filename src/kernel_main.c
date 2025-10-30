
#include <stdint.h>
#include <string.h>
#include "page.h"
#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6
#define PAGE_SIZE 4096U
#define PAGE_MASK (~(PAGE_SIZE - 1U))
#define PD_INDEX(v) (((uintptr_t)(v) >> 22) & 0x3FF)
#define PT_INDEX(v) (((uintptr_t)(v) >> 12) & 0x3FF)
#define FRAME_FROM_ADDR(a) (((uintptr_t)(a)) >> 12)
#define ADDR_FROM_FRAME(f) (((uintptr_t)(f)) << 12)
#define MAX_PAGE_TABLES 64

struct page_directory_entry
{
   uint32_t present       : 1;   // Page present in memory
   uint32_t rw            : 1;   // Read/write
   uint32_t user          : 1;   // User-mode if set
   uint32_t writethru     : 1;
   uint32_t cachedisabled : 1;
   uint32_t accessed      : 1;
   uint32_t pagesize      : 1;
   uint32_t ignored       : 2;
   uint32_t os_specific   : 3;
   uint32_t frame         : 20;  // Frame address (>>12)
} __attribute__((packed));

struct page
{
   uint32_t present    : 1;
   uint32_t rw         : 1;
   uint32_t user       : 1;
   uint32_t accessed   : 1;
   uint32_t dirty      : 1;
   uint32_t unused     : 7;
   uint32_t frame      : 20;    // Frame address (>>12)
} __attribute__((packed));

static inline uintptr_t page_align_down(uintptr_t a) {
    return a & (uintptr_t)PAGE_MASK;
}

struct page_directory_entry pd_global[1024] __attribute__((aligned(4096)));
static struct page pt_pool[MAX_PAGE_TABLES][1024] __attribute__((aligned(4096)));
static uint8_t pt_used[MAX_PAGE_TABLES] = {0};

/* allocate a page table from pool, return pointer or NULL if exhausted */
static struct page *allocate_page_table(void) {
    for (int i = 0; i < MAX_PAGE_TABLES; ++i) {
        if (!pt_used[i]) {
            pt_used[i] = 1;
            /* zero the new page table manually */
            for (int j = 0; j < 1024; ++j) {   // assuming 1024 entries per page table
                pt_pool[i][j].present    = 0;
                pt_pool[i][j].rw         = 0;
                pt_pool[i][j].user       = 0;
                pt_pool[i][j].accessed   = 0;
                pt_pool[i][j].dirty      = 0;
                pt_pool[i][j].unused     = 0;
                pt_pool[i][j].frame      = 0;
            }
            return pt_pool[i];
        }
    }   
    return NULL; 
}       
        
static void free_page_table(struct page *pt) {
    /* find index */
    uintptr_t base = (uintptr_t)pt;
    for (int i = 0; i < MAX_PAGE_TABLES; ++i) {
        if ((uintptr_t)pt_pool[i] == base) {
            pt_used[i] = 0;
            /* zero the page table manually */
            for (int j = 0; j < 1024; ++j) {
                pt_pool[i][j].present    = 0;
                pt_pool[i][j].rw         = 0;
                pt_pool[i][j].user       = 0;
                pt_pool[i][j].accessed   = 0;
                pt_pool[i][j].dirty      = 0;
                pt_pool[i][j].unused     = 0;
                pt_pool[i][j].frame      = 0;
            }
            return;
        } 
    }
}

/*---------------------------------------------------*/
/* Public Domain version of printf                   */
/*                                                   */
/* Rud Merriam, Compsult, Inc. Houston, Tx.          */
/*                                                   */
/* For Embedded Systems Programming, 1991            */
/*                                                   */
/*---------------------------------------------------*/

#include "rprintf.h"
/*---------------------------------------------------*/
/* The purpose of this routine is to output data the */
/* same as the standard printf function without the  */
/* overhead most run-time libraries involve. Usually */
/* the printf brings in many kiolbytes of code and   */
/* that is unacceptable in most embedded systems.    */
/*---------------------------------------------------*/

static func_ptr out_char;
static int do_padding;
static int left_flag;
static int len;
static int num1;
static int num2;
static char pad_character;

size_t strlen(const char *str) {
    unsigned int len = 0;
    while(str[len] != '\0') {
        len++;
    }
    return len;
}

int tolower(int c) {
    if(c < 'a') { // Check if c is uppercase
        c -= 'a' - 'A';
    }
    return c;
}

int isdig(int c) {
    if((c >= '0') && (c <= '9')){
        return 1;
    } else {
        return 0;
    }
}




/*---------------------------------------------------*/
/*                                                   */
/* This routine puts pad characters into the output  */
/* buffer.                                           */
/*                                                   */
static void padding( const int l_flag)
{
   int i;

   if (do_padding && l_flag && (len < num1))
      for (i=len; i<num1; i++)
          out_char( pad_character);
   }

/*---------------------------------------------------*/
/*                                                   */
/* This routine moves a string to the output buffer  */
/* as directed by the padding and positioning flags. */
/*                                                   */
static void outs( charptr lp)
{
   if(lp == NULL)
      lp = "(null)";
   /* pad on left if needed                          */
   len = strlen( lp);
   padding( !left_flag);

   /* Move string to the buffer                      */
   while (*lp && num2--)
      out_char( *lp++);

   /* Pad on right if needed                         */
   len = strlen( lp);
   padding( left_flag);
   }

/*---------------------------------------------------*/
/*                                                   */
/* This routine moves a number to the output buffer  */
/* as directed by the padding and positioning flags. */
/*                                                   */
static void outnum(unsigned int num, const int base)
{
   charptr cp;
   int negative;
   char outbuf[32];
   const char digits[] = "0123456789ABCDEF";

   /* Check if number is negative                    */
   /* NAK 2009-07-29 Negate the number only if it is not a hex value. */
   if (num < 0L && base != 16L) {
      negative = 1;
      num = -num;
      }
   else
      negative = 0;

   /* Build number (backwards) in outbuf             */
   cp = outbuf;
   do {
      *cp++ = digits[num % base];
      } while ((num /= base) > 0);
   if (negative)
      *cp++ = '-';
   *cp-- = 0;

   /* Move the converted number to the buffer and    */
   /* add in the padding where needed.               */
   len = strlen(outbuf);
   padding( !left_flag);
   while (cp >= outbuf)
      out_char( *cp--);
   padding( left_flag);
}

/*---------------------------------------------------*/
/*                                                   */
/* This routine gets a number from the format        */
/* string.                                           */
/*                                                   */
static int getnum( charptr* linep)
{
   int n;
   charptr cp;

   n = 0;
   cp = *linep;
   while (isdig((int)*cp))
      n = n*10 + ((*cp++) - '0');
   *linep = cp;
   return(n);
}

/*---------------------------------------------------*/
/*                                                   */
/* This routine operates just like a printf/sprintf  */
/* routine. It outputs a set of data under the       */
/* control of a formatting string. Not all of the    */
/* standard C format control are supported. The ones */
/* provided are primarily those needed for embedded  */
/* systems work. Primarily the floaing point         */
/* routines are omitted. Other formats could be      */
/* added easily by following the examples shown for  */
/* the supported formats.                            */
/*                                                   */

void esp_printf( const func_ptr f_ptr, charptr ctrl, ...)
{
  va_list args;
  va_start(args, *ctrl);
  esp_vprintf(f_ptr, ctrl, args);
  va_end( args );
  
}

void esp_vprintf( const func_ptr f_ptr, charptr ctrl, va_list argp)
{

   int long_flag;
   int dot_flag;

   char ch;
   //va_list argp;

   //va_start( argp, ctrl);
   out_char = f_ptr;

   for ( ; *ctrl; ctrl++) {

      /* move format string chars to buffer until a  */
      /* format control is found.                    */
      if (*ctrl != '%') {
         out_char(*ctrl);
         continue;
         }

      /* initialize all the flags for this format.   */
      dot_flag   =
      long_flag  =
      left_flag  =
      do_padding = 0;
      pad_character = ' ';
      num2=32767;

try_next:
      ch = *(++ctrl);

      if (isdig((int)ch)) {
         if (dot_flag)
            num2 = getnum(&ctrl);
         else {
            if (ch == '0')
               pad_character = '0';

            num1 = getnum(&ctrl);
            do_padding = 1;
         }
         ctrl--;
         goto try_next;
      }

      switch (tolower((int)ch)) {
         case '%':
              out_char( '%');
              continue;

         case '-':
              left_flag = 1;
              break;

         case '.':
              dot_flag = 1;
              break;

         case 'l':
              long_flag = 1;
              break;
	
         case 'i':
         case 'd':
              if (long_flag || ch == 'D') {
                 outnum( va_arg(argp, long), 10L);
                 continue;
                 }
              else {
                 outnum( va_arg(argp, int), 10L);
                 continue;
                 }
         case 'x':
              outnum( (long)va_arg(argp, int), 16L);
              continue;

         case 's':
              outs( va_arg( argp, charptr));
              continue;

         case 'c':
              out_char( va_arg( argp, int));
              continue;

         case '\\':
              switch (*ctrl) {
                 case 'a':
                      out_char( 0x07);
                      break;
                 case 'h':
                      out_char( 0x08);
                      break;
                 case 'r':
                      out_char( 0x0D);
                      break;
                 case 'n':
                      out_char( 0x0D);
                      out_char( 0x0A);
                      break;
                 default:
                      out_char( *ctrl);
                      break;
                 }
              ctrl++;
              break;

         default:
              continue;
         }
      goto try_next;
      }
   va_end( argp);
   }

/*---------------------------------------------------*/



unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
 '9', '0', '-', '=', '\b',     /* Backspace */
 '\t',                 /* Tab */
 'q', 'w', 'e', 'r',   /* 19 */
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
   0,                  /* 29   - Control */
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 39 */
'\'', '`',   0,                /* Left shift */
'\\', 'z', 'x', 'c', 'v', 'b', 'n',                    /* 49 */
 'm', ',', '.', '/',   0,                              /* Right shift */
 '*',
   0,  /* Alt */
 ' ',  /* Space bar */
   0,  /* Caps lock */
   0,  /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,  
   0,  /* < ... F10 */
   0,  /* 69 - Num lock*/
   0,  /* Scroll Lock */
   0,  /* Home key */
   0,  /* Up Arrow */
   0,  /* Page Up */
 '-',
   0,  /* Left Arrow */
   0,  
   0,  /* Right Arrow */
 '+',
   0,  /* 79 - End key*/
   0,  /* Down Arrow */
   0,  /* Page Down */
   0,  /* Insert Key */
   0,  /* Delete Key */
   0,   0,   0,  
   0,  /* F11 Key */
   0,  /* F12 Key */
   0,  /* All other keys are undefined */
};


const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

struct termbuf {
	char ascii;
	char color;
};

int x = 0;
int y = 0;
int maxRows = 25; // technically only 24 rows can display at a time, but you need to save the new row somewhere before scrolling
int rowChars = 80;

void scroll(void) {
    char *vram = (char *)0xB8000;
    int rowBytes = rowChars * 2;

    for (int row = 0; row < maxRows - 1; row++) {
        for (int col = 0; col < rowBytes; col++) {
            vram[row * rowBytes + col] = vram[(row + 1) * rowBytes + col];
        }
    }

    for (int col = 0; col < rowChars; col++) {
        vram[(maxRows - 1) * rowBytes + col * 2]     = ' ';
        vram[(maxRows - 1) * rowBytes + col * 2 + 1] = 7;
    }
}

int putc(int data) {
    char *vram = (char *)0xB8000;
    int rowBytes = rowChars * 2;
    // intialize starting address at top left of screen

    if (data == '\r') {
        x = 0; // reset x position
        return 0; // automatically return and stop if escape character
    } else if (data == '\n') {
        x = 0;
        y++; // reset x position back to 0 and iterate y to be next row if newline
    } else { // otherwise write to screen at position, and update memory
        vram[y * rowBytes + x * 2]     = (char)data;
        vram[y * rowBytes + x * 2 + 1] = 7; 
        x++; // iterate x after writing to screen
        if (x >= rowChars) { // if at max charas for the row, move to the next row
            x = 0;
            y++;
        }
    }

    // automatically call scroll function when row count gets to 25
    if (y >= maxRows) {
        scroll();
        y = maxRows - 1;  // resets row count back to 24 after scrolling
    }
}

int get_cpl() {
	unsigned short cs;
	__asm__ ("mov %%cs, %0" : "=r"(cs));
    	return cs & 3; 
}

 /* Returns the (input) vaddr mapped on success, or NULL on failure (e.g., out of pt pool).
 */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd) {
    if (!vaddr || !pglist || !pd) return NULL;

    uintptr_t va = (uintptr_t)vaddr;
    struct ppage *cur = pglist;

    while (cur) {
        uint32_t pd_idx = PD_INDEX(va);
        uint32_t pt_idx = PT_INDEX(va);

        /* Allocate a page table if not present */
        if (!pd[pd_idx].present) {
            struct page *new_pt = allocate_page_table();
            if (!new_pt)
                return NULL;  // out of tables

            pd[pd_idx].present = 1;
            pd[pd_idx].rw = 1;
            pd[pd_idx].user = 0;
            pd[pd_idx].frame = FRAME_FROM_ADDR((uintptr_t)new_pt);
        }

        /* Retrieve the page table from PDE */
        struct page *pt = (struct page *)ADDR_FROM_FRAME(pd[pd_idx].frame);

        /* Create mapping */
        pt[pt_idx].present = 1;
        pt[pt_idx].rw = 1;
        pt[pt_idx].user = 0;
        pt[pt_idx].frame = FRAME_FROM_ADDR(cur->physical_addr);

        va += PAGE_SIZE;
        cur = cur->next;
    }

    return vaddr;
}

/* --- Helper to load page directory into CR3 --- */
void loadPageDirectory(struct page_directory_entry *pd) {
    /* pd must be physical address of page directory; we assume identity mapping here. */
    asm volatile("mov %0,%%cr3"
                 :
                 : "r"(pd)
                 : );
}

/* --- Enable paging by setting CR0.PG (bit 31) and CR0.PE (bit 0). --- */
void enable_paging(void) {
    asm volatile(
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        :
        :
        : "eax");
}

void main() {
    unsigned short *vram = (unsigned short*)0xb8000; // Base address of video mem
    const unsigned char color = 7; // gray text on black background


    // testing putc and scroll by printing 24 lines
    for (int line = 0; line < 24; line = line + 1) {
        putc('L');
        putc('i');
        putc('n');
        putc('e');
        putc(' ');
        putc('0' + (line / 10));  // tens digit
        putc('0' + (line % 10));  // ones digit
        putc('\r');
        putc('\n');
    }
    	// getting current execution level, also considered 25th line, should scroll the screen up one line
    	int cpl = get_cpl();
	esp_printf(putc, "Execution level = %d\r\n", cpl);

/* Identity-map 0x100000 -> page-by-page */
    uintptr_t start = 0x100000;
    uintptr_t end   = 0x110000; 

    /* Map kernel physical pages one-by-one using tmp ppage */
    for (uintptr_t a = start; a < end; a += PAGE_SIZE) {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.physical_addr = (void *)a;   /* physical == virtual for identity mapping */
        /* map_pages expects list; we map one page at a time */
        if (!map_pages((void *)a, &tmp, pd_global)) {
        // mapping failed, hang
        while(1);
    }
}


    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    /* Map downwards from current esp rounded down to page boundary */
    uintptr_t stack_top = page_align_down(esp);
    const int STACK_PAGES = 8; /* adjust as needed */
    for (int i = 0; i < STACK_PAGES; ++i) {
        uintptr_t a = stack_top - i * PAGE_SIZE;
        struct ppage tmp;
        tmp.next = NULL;
        tmp.physical_addr = (void *)a; /* identity map */
        if (!map_pages((void *)a, &tmp, pd_global)) {
            for (;;) ; /* error handling */
        }
    }

    /* Map video buffer at 0xB8000 (one page) */
    {
        struct ppage tmp;
        tmp.next = NULL;
        tmp.physical_addr = (void *)0xB8000;
        if (!map_pages((void *)0xB8000, &tmp, pd_global)) {
            for (;;) ;
        }
    }

    /* Load page directory into CR3 and enable paging. */
    loadPageDirectory(pd_global);
    enable_paging();
	
    while(1) {
        uint8_t status = inb(0x64);
        if((status & 1) == 1) {
            uint8_t scancode = inb(0x60);
            if (scancode < 128) {
            esp_printf(putc, "0x%02x %c\n",  scancode, keyboard_map[scancode]);
            }
        }
    }

}

