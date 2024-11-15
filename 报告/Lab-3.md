# 实验三实验报告

## **实验名称：缺页异常和页面置换**

---

### 一、实验目的

- 明白操作系统虚拟内存的Page Fault异常处理实现
- 了解不同的页替换算法在操作系统中的实现
- 学会如何使用多级页表，处理缺页异常（Page Fault），实现页面置换算法。

--- 

### 二、实验练习

#### 练习1：理解基于FIFO的页面替换算法（思考题）

> 描述FIFO页面置换算法下，一个页面从被换入到被换出的过程中，会经过代码里哪些函数/宏的处理（或者说，需要调用哪些函数/宏），并用简单的一两句话描述每个函数在过程中做了什么？（为了方便同学们完成练习，所以实际上我们的项目代码和实验指导的还是略有不同，例如我们将FIFO页面置换算法头文件的大部分代码放在了`kern/mm/swap_fifo.c`文件中，这点请同学们注意）
> 
> - 至少正确指出10个不同的函数分别做了什么？如果少于10个将酌情给分。我们认为只要函数原型不同，就算两个不同的函数。要求指出对执行过程有实际影响,删去后会导致输出结果不同的函数（例如assert）而不是cprintf这样的函数。如果你选择的函数不能完整地体现”从换入到换出“的过程，比如10个函数都是页面换入的时候调用的，或者解释功能的时候只解释了这10个函数在页面换入时的功能，那么也会扣除一定的分数

出现缺页异常的时候，首先是在`kern/trap/trap.c`的`exception_handler()`函数里进行的。按照`scause`寄存器对异常的分类里，有`CAUSE_LOAD_PAGE_FAULT` 和`CAUSE_STORE_PAGE_FAULT`两个case。这里的异常处理程序会把**Page Fault** 分发给`kern/mm/vmm.c`的`do_pgfault()`函数并尝试进行页面置换。

##### 1. **`do_pgfault`**

- **触发条件**：页错误发生时，进程访问了一个不存在于物理内存中的虚拟地址。
- **作用**：检查该虚拟地址是否有效，并决定是否需要从磁盘加载页面。

在`do_pgfault` 函数中，首先会先查找虚拟内存区域（VMA）。即利用函数`find_vma(mm, addr)`：该函数通过 `mm`（进程的内存管理结构体）和虚拟地址 `addr` 查找该地址是否属于进程的虚拟内存区域（VMA）。如果找不到或地址不在有效的 VMA 范围内，函数会输出错误信息并返回失败。

##### 2. **`find_vma`**

- **触发条件**：在 `do_pgfault` 中，首先需要查找包含给定虚拟地址的 VMA（虚拟内存区域）。
- **作用**：返回包含该地址的虚拟内存区域（VMA）。

然后再查找相应的页表项。通过页表项，操作系统能够确定虚拟地址到物理地址的映射关系，并有效管理内存。调用函数`get_pet`查找相应的页表项。

##### 3. **`get_pte`**

- **触发条件**：在 `do_pgfault` 中，查找虚拟地址对应的页表项。
- **作用**：查找或分配虚拟地址 `addr` 对应的页表项（PTE）。如果该页表项不存在，且需要的话，会为它分配一个新的页表项。

如果页表项为空，调用 `pgdir_alloc_page(mm->pgdir, addr, perm)` 来分配一个新的物理页面并将其映射到虚拟地址 `addr` 上。这个操作会调用 `alloc_page` 来分配物理页面，并用 `page_insert` 进行映射。

##### 4. **`pgdir_alloc_page`**

- **触发条件**：如果页表项为空，且需要分配一个新页面，`pgdir_alloc_page` 会为该虚拟地址分配一个新的物理页面并更新页表。
- **作用**：为页表分配物理页面并建立虚拟地址与物理地址的映射。

如果页表项存在但它指向交换空间中的页面，那么首先需要从交换空间加载页面到内存。`swap_in(mm, addr, &page)`：根据虚拟内存地址 `addr` 从交换空间加载页面，并将加载的页面存储在 `page` 中。如果 `swap_in` 失败，则输出错误并返回。

##### 5. **`swap_in`**

- **触发条件**：当页面缺失且需要从磁盘加载时，`swap_in` 会被调用。
- **作用**：将页面从磁盘的交换空间加载到物理内存中。

##### 6. **`swap_out`**

- **触发条件**：如果内存不足并且需要交换出一个页面时，`swap_out` 会被调用。
- **作用**：选择一个页面（FIFO 算法会选择最久未使用的页面）并将该页面保存到交换空间中。

##### 7. **`get_page`**

- **触发条件**：在 `do_pgfault` 中，如果页面的物理内存尚未分配，则调用 `get_page`。
- **作用**：返回虚拟地址对应的物理页面，如果页表项为空，则触发页面置换。

##### 8. **`page_insert`**

- **触发条件**：当一个页面被加载到物理内存后，调用 `page_insert` 将该页面插入到页表中，建立虚拟地址到物理地址的映射。
- **作用**：将物理页面与虚拟地址建立映射。

##### 9. **`swap_map_swappable`**

- **触发条件**：页面加载到内存后，标记该页面为可交换的状态。
- **作用**：标记页面为可交换，从而允许将来将该页面再次交换到磁盘。

##### 10. **`tlb_invalidate`**

- **触发条件**：页面置换或映射更改时，需要刷新 TLB。
- **作用**：刷新 TLB，以确保新的虚拟地址到物理地址的映射生效。

##### 11. **`page_remove`**

- **触发条件**：当一个页面需要被换出时，解除虚拟地址与物理地址的映射。
- **作用**：移除虚拟地址和物理页面的映射，并标记该物理页面为不再使用。

##### 12. **`free_page`**

- **触发条件**：当页面被换出时，`free_page` 会释放不再需要的物理页面。
- **作用**：释放一个物理页面，将其标记为可用。

在swap_out函数中，

#### 练习2：深入理解不同分页模式的工作原理（思考题）

> get_pte()函数（位于`kern/mm/pmm.c`）用于在页表中查找或创建页表项，从而实现对指定线性地址对应的物理页的访问和映射操作。这在操作系统中的分页机制下，是实现虚拟内存与物理内存之间映射关系非常重要的内容。
> 
> - get_pte()函数中有两段形式类似的代码， 结合sv32，sv39，sv48的异同，解释这两段代码为什么如此相像。
> - 目前get_pte()函数将页表项的查找和页表项的分配合并在一个函数里，你认为这种写法好吗？有没有必要把两个功能拆开？

##### 1. **分析 `get_pte()` 中两段相似代码的结构：**

首先，`get_pte()` 函数的主要作用是根据给定的线性地址（`la`）查找或分配页表项（PTE）。在这段代码中，存在两个非常相似的代码块，分别处理了**页目录项**（PDE）和**页表项**（PTE）的查找和分配。这两个代码块都涉及到以下步骤：

- 查找页表项（如果存在）。
- 如果页表项不存在，且需要创建（由 `create` 参数控制），则为其分配一个新的页面。
- 初始化新的页面并设置页表项。

**相似代码的两段：**

1. **查找并分配页目录项（PDE）**：
   
   ```c
   pde_t *pdep1 = &pgdir[PDX1(la)];
   if (!(*pdep1 & PTE_V)) {
       struct Page *page;
       if (!create || (page = alloc_page()) == NULL) {
           return NULL;
       }
       set_page_ref(page, 1);
       uintptr_t pa = page2pa(page);
       memset(KADDR(pa), 0, PGSIZE);
       *pdep1 = pte_create(page2ppn(page), PTE_U | PTE_V);
   }
   ```
   
   - **作用**：查找 `pde_t *pdep1`（即页目录项），并根据 `PTE_V`（Present位）判断该目录项是否有效。如果无效且 `create` 为真，分配一个新的物理页面并将其初始化为页表。

2. **查找并分配页表项（PTE）**：
   
   ```c
   pde_t *pdep0 = &((pde_t *)KADDR(PDE_ADDR(*pdep1)))[PDX0(la)];
   if (!(*pdep0 & PTE_V)) {
       struct Page *page;
       if (!create || (page = alloc_page()) == NULL) {
           return NULL;
       }
       set_page_ref(page, 1);
       uintptr_t pa = page2pa(page);
       memset(KADDR(pa), 0, PGSIZE);
       *pdep0 = pte_create(page2ppn(page), PTE_U | PTE_V);
   }
   ```
   
   - **作用**：查找页表项 `pdep0`（即页表项），并根据 `PTE_V` 判断该页表项是否有效。如果无效且 `create` 为真，分配一个新的物理页面并将其初始化为页面条目。

##### 2. **为什么这两段代码如此相像？（结合 SV32, SV39, SV48 的不同）**

这两段代码的结构非常相似，因为它们执行的是**相同的操作**：**查找并分配页表项**，只不过它们操作的是不同层次的页表（页目录和页表）。这种相似性源自于虚拟内存结构在不同级别的页表（如 SV32、SV39 和 SV48）中的**一致性**，尤其是在分配新页表项时，虽然在细节上有所不同，但总体的结构和操作是相同的。

- **SV32、SV39 和 SV48 的共同点：**
  
  - 所有这些地址映射结构都使用了**多级页表**，它们将虚拟地址分为多个段（如页目录、页表和页内偏移）。无论是 SV32、SV39 还是 SV48，都会使用类似的方式来查找和分配每一层页表的条目（PDE 和 PTE）。
  - **查找**：不同级别的页表条目（PDE 和 PTE）通过索引虚拟地址的不同部分来确定。`PDX1(la)` 和 `PDX0(la)` 在不同的页表层级（SV32、SV39、SV48）中使用不同数量的位来索引。
  - **分配**：如果页表项不存在且 `create` 为真，都会分配新的物理页面，初始化为页面表项。此操作在多级页表的每一层都相似。

- **SV32、SV39 和 SV48 的区别：**
  
  - **SV32**：使用两级页表（页目录 + 页表），每个页表条目对应 4KB 页。
  - **SV39**：使用三级页表（L0 + L1 + L2），每个页表条目对应 8KB 页（具体取决于页表项大小和操作系统实现）。
  - **SV48**：使用四级页表（L0 + L1 + L2 + L3），每个页表条目对应更大的内存页（如 4KB、2MB 或 1GB）。

尽管这些实现有差异，但在**查找和创建页表项**的操作中，代码模式非常相似，主要区别在于页表项索引的位数和页面大小。

##### 3. **是否有必要将“查找页表项”和“分配页表项”拆开？**

目前 `get_pte()` 函数将**查找页表项**和**分配页表项**的逻辑合并在一个函数中，这种实现方式在许多内存管理系统中是常见的，特别是在进行虚拟内存管理时。然而，**是否拆开这两个功能**可以根据以下几个考虑来判断：

###### 拆开两个功能的优点：

1. **功能分离，增强可读性和可维护性**：
   
   - **单一职责原则**：将“查找页表项”和“分配页表项”拆开，使得每个函数的职责更加明确。例如，可以有一个专门用于查找页表项的函数 `find_pte()`，和一个用于分配页表项的函数 `alloc_pte()`，这有助于提高代码的可读性和可维护性。

2. **重用性**：
   
   - 如果查找页表项和分配页表项的功能分开，其他地方可能只需要查找而不需要分配，或者只需要分配而不需要查找。分离这两个功能可以提高它们的重用性。

3. **优化和扩展**：
   
   - 例如，如果系统支持多种不同的内存管理策略（如不同的页面置换算法），可能需要对查找和分配页表项的过程做出不同的优化。将这两部分拆开可以更容易地在不影响其他部分的情况下进行优化和扩展。

###### 保留在同一个函数中的优点：

1. **简化代码结构**：
   
   - 在实际应用中，查找和分配页表项通常是紧密关联的。大多数情况下，查找页表项和分配页表项是一起进行的，拆开这两个步骤可能导致代码过于分散，增加不必要的复杂性。

2. **性能考虑**：
   
   - 这两个操作是经常一起调用的，因此将它们合并在一个函数中可能有助于减少不必要的函数调用和性能开销。

3. **减少函数调用开销**：
   
   - 将查找和分配操作放在一个函数中，避免了每次查找页表项时都进行单独的分配操作，可以减少调用栈的深度，稍微提高性能。

在当前的实现中，将查找和分配操作合并在同一个函数中是合理的，特别是对于常见的页面管理操作。然而，如果系统需要更高的灵活性、可维护性和优化空间，拆开这两个功能是一个可取的选择，尤其是当系统的页面管理策略复杂时。

#### 练习3：给未被映射的地址映射上物理页（需要编程）

> 补充完成do_pgfault（mm/vmm.c）函数，给未被映射的地址映射上物理页。设置访问权限 的时候需要参考页面所在 VMA 的权限，同时需要注意映射物理页时需要操作内存控制 结构所指定的页表，而不是内核的页表。
> 
> 请在实验报告中简要说明你的设计实现过程。请回答如下问题：
> 
> - 请描述页目录项（Page Directory Entry）和页表项（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。
> - 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？
> - 数据结构Page的全局变量（其实是一个数组）的每一项与页表中的页目录项和页表项有无对应关系？如果有，其对应关系是啥？

在 `do_pgfault` 函数中，处理缺页错误（Page Fault）时，关键的操作是通过页表项（PTE）查找虚拟地址对应的物理页面，若没有对应的物理页面，则可能会进行页面换入操作（swap）。你在 `do_pgfault` 函数中需要补充代码的部分涉及到 **交换页面的加载** 和 **页面映射**。具体来说：

1. **查找页表项**：
   
   - `get_pte(mm->pgdir, addr, 1)` 用来获取虚拟地址 `addr` 对应的页表项（PTE）。如果该虚拟地址没有对应的页表项，且 `create` 参数为 `1`，则会创建一个新的页表项。

2. **判断页表项是否存在**：
   
   - 如果 `*ptep == 0`，表示该虚拟地址的页面没有被映射到物理内存上（页表项为空），此时需要分配新的物理页面。
   - `pgdir_alloc_page(mm->pgdir, addr, perm)` 用于分配物理页面，并更新页表项，将其映射到虚拟地址。

3. **交换页面**：
   
   - 如果交换初始化已经完成（`swap_init_ok`），那么当前访问的页面可能在交换空间中，操作系统会从磁盘中加载该页面。`swap_in(mm, addr, &page)` 用于将页面从交换空间加载到内存。
   - 加载成功后，通过 `page_insert` 将物理页面与虚拟地址建立映射，并调用 `swap_map_swappable` 将页面标记为可交换。

4. **更新页面结构**：
   
   - 设置 `page->pra_vaddr = addr;` 用于记录物理页面的虚拟地址。

5. **错误处理**：
   
   - 如果任何步骤失败（如页面分配、交换加载等），函数会输出错误信息并跳转到 `failed` 标签。

###### 3.1 代码编程及解释

**代码实现** ：

```c
int
do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr) {
    int ret = -E_INVAL;
    //try to find a vma which include addr
    struct vma_struct *vma = find_vma(mm, addr);

    pgfault_num++;
    //If the addr is in the range of a mm's vma?
    if (vma == NULL || vma->vm_start > addr) {
        cprintf("not valid addr %x, and  can not find it in vma\n", addr);
        goto failed;
    }

    /* IF (write an existed addr ) OR
     *    (write an non_existed addr && addr is writable) OR
     *    (read  an non_existed addr && addr is readable)
     * THEN
     *    continue process
     */
    uint32_t perm = PTE_U;
    if (vma->vm_flags & VM_WRITE) {
        perm |= (PTE_R | PTE_W);
    }
    addr = ROUNDDOWN(addr, PGSIZE);

    ret = -E_NO_MEM;

    pte_t *ptep=NULL;
    /*
    * Maybe you want help comment, BELOW comments can help you finish the code
    *
    * Some Useful MACROs and DEFINEs, you can use them in below implementation.
    * MACROs or Functions:
    *   get_pte : get an pte and return the kernel virtual address of this pte for la
    *             if the PT contians this pte didn't exist, alloc a page for PT (notice the 3th parameter '1')
    *   pgdir_alloc_page : call alloc_page & page_insert functions to allocate a page size memory & setup
    *             an addr map pa<--->la with linear address la and the PDT pgdir
    * DEFINES:
    *   VM_WRITE  : If vma->vm_flags & VM_WRITE == 1/0, then the vma is writable/non writable
    *   PTE_W           0x002                   // page table/directory entry flags bit : Writeable
    *   PTE_U           0x004                   // page table/directory entry flags bit : User can access
    * VARIABLES:
    *   mm->pgdir : the PDT of these vma
    *
    */


    ptep = get_pte(mm->pgdir, addr, 1);  //(1) try to find a pte, if pte's
                                         //PT(Page Table) isn't existed, then
                                         //create a PT.

    if(ptep==NULL){
        cprintf("get_pte in do_pgfault failed\n");
        goto failed;
    }

    if (*ptep == 0) {
        if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
            cprintf("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    } else {
        /*LAB3 EXERCISE 3: 2213912
        * 请你根据以下信息提示，补充函数
        * 现在我们认为pte是一个交换条目，那我们应该从磁盘加载数据并放到带有phy addr的页面，
        * 并将phy addr与逻辑addr映射，触发交换管理器记录该页面的访问情况
        *
        *  一些有用的宏和定义，可能会对你接下来代码的编写产生帮助(显然是有帮助的)
        *  宏或函数:
        *    swap_in(mm, addr, &page) : 分配一个内存页，然后根据
        *    PTE中的swap条目的addr，找到磁盘页的地址，将磁盘页的内容读入这个内存页
        *    page_insert ： 建立一个Page的phy addr与线性addr la的映射
        *    swap_map_swappable ： 设置页面可交换
        */

        if (swap_init_ok) {
            struct Page *page = NULL;
            // 你要编写的内容在这里，请基于上文说明以及下文的英文注释完成代码编写
            //(1）According to the mm AND addr, try
            //to load the content of right disk page
            //into the memory which page managed.
            //(2) According to the mm,
            //addr AND page, setup the
            //map of phy addr <--->
            //logical addr
            //(3) make the page swappable.
            if ((ret = swap_in(mm, addr, &page)) != 0) {
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
            }    
            page_insert(mm->pgdir, page, addr, perm);
            swap_map_swappable(mm, addr, page, 1);
            page->pra_vaddr = addr;
        } else {
            cprintf("no swap_init_ok but ptep is %x, failed\n", *ptep);
            goto failed;
        }
   }

   ret = 0;
failed:
    return ret;
}
```

- **定义一个页面结构体指针：**
  
  ```c
  struct Page *page = NULL;
  ```
  
  - 定义一个指向 `struct Page` 的指针 `page`，用于保存从交换空间加载到内存中的页面。
  - `struct Page` 通常表示一个物理页面，它包含该页面的相关信息，如物理地址、引用计数等。

- **从交换空间加载页面：**
  
  ```c
  if ((ret = swap_in(mm, addr, &page)) != 0) {
      cprintf("swap_in in do_pgfault failed\n");
      goto failed;
  }
  ```
  
  - **`swap_in(mm, addr, &page)`**：该函数尝试从交换空间（磁盘）加载与虚拟地址 `addr` 对应的页面。交换空间用于存储从物理内存中换出的页面，`swap_in` 会根据 `addr` 获取对应的页面，将它加载到物理内存中，并将该页面的指针赋值给 `page`。
  - 如果 `swap_in` 返回非零值，表示加载页面失败，此时输出错误信息并跳转到 `failed` 标签，执行错误处理。

- **将页面映射到虚拟地址：**
  
  ```c
  page_insert(mm->pgdir, page, addr, perm);
  ```
  
  - **`page_insert`**：该函数将加载到内存中的物理页面 `page` 与虚拟地址 `addr` 建立映射。`perm` 是该页面的权限，控制是否可读、可写等。
  - 这个操作会更新页表，使得进程可以通过虚拟地址 `addr` 访问到物理页面 `page`。

- **标记页面为可交换：**
  
  ```c
  swap_map_swappable(mm, addr, page, 1);
  ```
  
  - **`swap_map_swappable`**：该函数将页面 `page` 标记为“可交换”的状态。意味着在未来，这个页面可以再次被交换到磁盘（如果内存压力增大，系统需要腾出空间）。
  - 这对于内存管理至关重要，因为它告诉操作系统这个页面目前在内存中，但将来可能会被交换出去。

- **记录页面的虚拟地址：**
  
  ```c
  page->pra_vaddr = addr;
  ```
  
  
  
  
  - **`page->pra_vaddr`**：将物理页面 `page` 的虚拟地址（`addr`）记录下来。这是为了追踪页面的虚拟地址与物理页面之间的映射关系。
  - 该字段用于内存管理的其他部分，确保操作系统能够有效地管理每个物理页面的虚拟地址映射。

###### 3.2 描述页目录项（Page Directory Entry）和页表项（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。

在 ucore 操作系统中，**页替换算法**（如 FIFO、LRU 等）决定了在内存满了之后，哪些页面应该被换出到磁盘，以便腾出空间来加载新的页面。页目录项（PDE）和页表项（PTE）提供了关键的信息，用来辅助实现页替换算法。

1. **有效性检查与页面置换**
- **PDE 的有效位（PTE_V）** 和 **PTE 的有效位（PTE_V）** 都决定了一个虚拟地址是否已经映射到物理内存。如果页目录项或者页表项无效，表示该虚拟地址没有有效的物理内存映射。在实现页替换算法时，可以通过检查这些标志位来确定哪些页面不再使用，或者哪些页面需要被替换。
  
  - 在页面置换时，操作系统可以通过检查这些标志位，快速判断哪些页面是有效的，哪些页面可以被换出。
  - 如果页面的 `PTE_V` 为 0，说明该页面不在内存中，可以通过置换将其加载进来。
2. **访问权限的帮助**
- 页目录项和页表项中的 **权限位（PTE_U 和 PTE_W）** 控制了页面的访问权限，决定了页面是否可读、可写以及是否可由用户访问。操作系统在进行页替换时，可能需要考虑页面的访问权限。
  
  - 在页替换算法中，操作系统可能会优先选择可写（`PTE_W`）的页面进行置换，或者根据 `PTE_U` 判断是否允许用户进程访问该页面。如果页面是只读的或具有高优先级的访问权限（如代码页面），则可能不容易被替换。
  - 在某些实现中，**脏页标志**（可能存在于 `PTE`）表示页面自上次交换以来是否被修改，操作系统在选择置换页面时会考虑这一点，以确保写入过的页面被正确地保存回磁盘。
3. **页替换和页面的访问历史**
- 页表项中的 **引用位**（可能存在于 `PTE`）和 **脏页标志**（如果有）对实现页替换算法非常重要。现代的操作系统会使用 **LRU（最近最少使用）** 或 **CLOCK** 算法来进行页面置换，这些算法通常会根据页面的访问历史来决定哪些页面应该被换出。
  
  - **引用位（Reference Bit）**：该位通常在访问页面时被设置为 1。在页替换算法中，系统可以检查引用位来判断页面是否近期被访问过。例如，**LRU** 算法会优先选择未被访问的页面进行置换。
  - **脏页标志（Dirty Bit）**：表示页面自上次被换出以来是否被修改。操作系统需要在换出页面时检查这个标志。如果页面是“脏的”，则需要将其内容保存回磁盘，否则可以跳过该步骤。
4. **辅助页面置换决策**
- 通过 **页目录项** 和 **页表项** 提供的信息，操作系统能够更有效地管理内存。例如，操作系统可以根据虚拟内存区域的标志（如 `VM_WRITE`）来决定是否允许对某些页面进行置换，或者是否需要优先保留某些页面。
  
  - **`PTE_U`（用户访问权限）**：如果页面是用户级别的内存，可能需要更频繁地进行交换或置换，特别是在用户进程的内存压力较大时。
  - **`PTE_W`（写权限）**：如果页面是可写的，操作系统可能需要在页替换时将页面内容保存到交换空间或磁盘中。

###### 3.3 ****如果 ucore 的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？****

如果出现了页访问异常，那么硬件将引发页访问异常的地址将被保存在 cr2 寄存器中，设置错误代码，然后触发 Page Fault 异常，进入**do_pgdefault**函数处理。

###### 3.4 **数据结构 Page 的全局变量（其实是一个数组）的每一项与页表中的页目录项和页表项有无对应关系？如果有，其对应关系是啥？**

在 ucore 操作系统中，`Page` 是用于管理物理内存页面的数据结构。每个 `Page` 对象代表物理内存中的一页，`Page` 数组则是一个全局变量，表示整个物理内存中的所有页面。而页目录项（PDE）和页表项（PTE）则是虚拟内存到物理内存的映射的关键结构，它们用于管理虚拟地址与物理内存之间的映射关系。

**`PTE` 和 `Page` 数组是直接对应关系**：每个有效的 `PTE` 存储物理页框号（PFN），PFN 对应 `Page` 数组中的一个条目，表示一个物理页面。**`PDE` 与 `Page` 数组是间接关系**：`PDE` 指向页表，而页表项（`PTE`）则指向具体的物理页面。`PDE` 通过页表间接指向物理页面，而 `PTE` 通过 PFN 直接指向 `Page` 数组中的物理页面。

**`PDE` → 页表 → `PTE` → 物理页面（`Page`）**

- `PDE` 将虚拟地址映射到页表。
- `PTE` 将虚拟地址映射到物理页框号（PFN）。
- PFN 指向物理页面，物理页面则由 `Page` 数组中的元素表示。


