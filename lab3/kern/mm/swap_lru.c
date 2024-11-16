#include <defs.h>
#include <riscv.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_lru.h>
#include <list.h>

extern list_entry_t pra_list_head;
int lru_use[100];
int time=0;

static int
_page_register(struct Page *page) {
    cprintf("%x\n",(page->pra_vaddr));
    lru_use[(page->pra_vaddr/PGSIZE)]=1;
    time++;
    return 0;
}
static int
_page_unregister(struct Page *page) {
    lru_use[(int)(page->pra_vaddr/PGSIZE)]=9999;
    return 0;
}
static int
_page_use(uintptr_t pra_vaddr) {
    cprintf("pra_vaddr %x comes into use\n",pra_vaddr);
    if(lru_use[(int)(pra_vaddr/PGSIZE)]==9999)
        lru_use[(int)(pra_vaddr/PGSIZE)]=1;
    else lru_use[(int)(pra_vaddr/PGSIZE)]++;
    return 0;
}

static int
_lru_init(void) {
    return 0;
}

static int
_lru_set_unswappable(struct mm_struct* mm, uintptr_t addr) {
    return 0;
}

static int
_lru_tick_event(struct mm_struct* mm) {
    return 0;
}

static int
_lru_init_mm(struct mm_struct* mm) {
    list_init(&pra_list_head);
    mm->sm_priv=&pra_list_head;
    for(int i=0;i<100;i++){
        lru_use[i]=9999;
    }
    return 0;
}

static int
_lru_map_swappable(struct mm_struct* mm, uintptr_t addr, struct Page* page, int swap_in) {
    list_entry_t* head=(list_entry_t*) mm->sm_priv;
    list_entry_t* entry=&(page->pra_page_link);
    assert(entry!=NULL && head!=NULL);
    list_add(head, entry);
    //_page_register(page);
    return 0;
}

static int
_lru_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick) {
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    assert(head!=NULL);
    assert(in_tick==0);
    /* Select the victim */
    //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
    //(2)  set the addr of addr of this page to ptr_page
    list_entry_t *le = head->next;
    int min=9999;
    int min_index=0;
    for(int i=0;i<100;i++){
        if(lru_use[i]<min){
            min=lru_use[i];
            min_index=i;
        }
    }
    cprintf("min_index %d min %d\n",min_index,min);

    list_entry_t *lep = head->next;
    while(1){
        if (lep == head) {
            break;
        }
        struct Page *pt = le2page(lep, pra_page_link);
        cprintf("p->pra_vaddr %x, index %d, use_times %d \n",pt->pra_vaddr, (int)(pt->pra_vaddr/PGSIZE), lru_use[(int)(pt->pra_vaddr/PGSIZE)]);
        lep = lep->next;
    }

    while (1) {
        struct Page *p = le2page(le, pra_page_link);
        //cprintf("p->pra_vaddr %x, index %d, use_times %d \n",p->pra_vaddr, (int)(p->pra_vaddr/PGSIZE), lru_use[(int)(p->pra_vaddr/PGSIZE)]);
        assert(p != NULL);
        if (p->pra_vaddr == (uintptr_t)(min_index)*PGSIZE) {
            *ptr_page = p;
            list_del(le);
            _page_unregister(p);
            return 0;
        }
        le = le->next;
    }
    return -1;
}

static int
_lru_check_swap(void){
    cprintf("check swap (lru)\n");
    *(unsigned char *)0x3000 = 0x0c;
    _page_use(0x3000);
    assert(pgfault_num==4);
    *(unsigned char *)0x1000 = 0x0a;
    _page_use(0x1000);
    assert(pgfault_num==4);
    *(unsigned char *)0x4000 = 0x0d;
    _page_use(0x4000);
    assert(pgfault_num==4);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==4);
    *(unsigned char *)0x5000 = 0x0e;
    _page_use(0x5000);
    assert(pgfault_num==5);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==5);
    *(unsigned char *)0x1000 = 0x0a;
    _page_use(0x1000);
    assert(pgfault_num==6);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==6);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==6);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==6);
    *(unsigned char *)0x2000 = 0x0b;
    _page_use(0x2000);
    assert(pgfault_num==6);
    *(unsigned char *)0x3000 = 0x0c;
    _page_use(0x3000);
    assert(pgfault_num==7);
    *(unsigned char *)0x4000 = 0x0d;
    _page_use(0x4000); 
    assert(pgfault_num==7);
    *(unsigned char *)0x5000 = 0x0e;
    _page_use(0x5000);
    assert(pgfault_num==7);
    *(unsigned char *)0x1000 = 0x0a;
    _page_use(0x1000);
    assert(pgfault_num==8);
    *(unsigned char *)0x1000 = 0x0a;
    _page_use(0x1000);
    assert(pgfault_num==8);
    *(unsigned char *)0x4000 = 0x0a;
    _page_use(0x6000);
    assert(pgfault_num==8);
    *(unsigned char *)0x1000 = 0x0a;
    _page_use(0x1000);
    assert(pgfault_num==8);

    return 0;
}

struct swap_manager swap_manager_lru =
{
    .name            = "lru swap manager",
    .init            = &_lru_init,
    .init_mm         = &_lru_init_mm,
    .tick_event      = &_lru_tick_event,
    .map_swappable   = &_lru_map_swappable,
    .set_unswappable = &_lru_set_unswappable,
    .swap_out_victim = &_lru_swap_out_victim,
    .check_swap      = &_lru_check_swap,
};