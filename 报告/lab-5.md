# 实验四实验报告

## **实验名称：进程管理**

---

## 一、实验目的：

- 了解第一个用户进程创建过程
- 了解系统调用框架的实现机制
- 了解ucore如何实现系统调用sys_fork/sys_exec/sys_exit/sys_wait来进行进程管理

---

## 二、实验练习

### 练习1：加载应用程序并执行（需要编码）

**do_execv**函数调用`load_icode`（位于kern/process/proc.c中）来加载并解析一个处于内存中的ELF执行文件格式的应用程序。你需要补充`load_icode`的第6步，建立相应的用户内存空间来放置应用程序的代码段、数据段等，且要设置好`proc_struct`结构中的成员变量trapframe中的内容，确保在执行此进程后，能够从应用程序设定的起始执行地址开始执行。需设置正确的trapframe内容。

请在实验报告中简要说明你的设计实现过程。

- 请简要描述这个用户态进程被ucore选择占用CPU执行（RUNNING态）到具体执行应用程序第一条指令的整个经过。

**回答：**

--- 

### 练习2: 父进程复制自己的内存空间给子进程（需要编码）

*创建子进程的函数`do_fork`在执行中将拷贝当前进程（即父进程）的用户内存地址空间中的合法内容到新进程中（子进程），完成内存资源的复制。具体是通过`copy_range`函数（位于kern/mm/pmm.c中）实现的，请补充`copy_range`的实现，确保能够正确执行。*

*请在实验报告中简要说明你的设计实现过程。*

- *如何设计实现`Copy on Write`机制？给出概要设计，鼓励给出详细设计。*

> *Copy-on-write（简称COW）的基本概念是指如果有多个使用者对一个资源A（比如内存块）进行读操作，则每个使用者只需获得一个指向同一个资源A的指针，就可以该资源了。若某使用者需要对这个资源A进行写操作，系统会对该资源进行拷贝操作，从而使得该“写操作”使用者获得一个该资源A的“私有”拷贝—资源B，可对资源B进行写操作。该“写操作”使用者对资源B的改变对于其他的使用者而言是不可见的，因为其他使用者看到的还是资源A。*

**回答：**

#### COW 机制的触发

在 COW 机制下，目标页在初始阶段会共享源页的内容，即源进程和目标进程共同使用同一个物理页。然而，一旦目标进程对该页发起写操作，由于该页标记为“只读”或“共享”，系统会触发一个页面错误。此时，操作系统的错误处理程序会捕获该错误，创建一个新的物理页，复制源页的内容，并更新目标进程的页表，确保它可以继续写入新的物理页。其他共享该页的进程则仍然能够访问原始页，确保它们不会受到影响。

#### Copy-on-Write (COW) 机制设计

**Copy-on-Write**（COW）机制是一种内存管理优化技术，它使得多个进程可以共享同一内存页，直到其中一个进程对该页进行写操作时，才会触发实际的内存拷贝。这种策略大大节省了内存资源，并且提高了多进程系统的效率。

##### 设计思路

基本的设计思想是，当多个进程共享同一资源（例如内存页）时，只有当某个进程试图修改该资源时，系统才会创建该资源的副本，以供该进程单独使用。在实际运行时，大部分内存页是可以被多个进程共享的，而只有当需要写操作时，才进行页的复制。

具体实现过程中，我们可以在页表项（PTE）中设置一个标志，表示该内存页是否可以共享。若多个进程共享同一内存页并且某个进程进行写操作时，这会触发一个页面错误，由错误处理程序来处理，具体操作是为该进程分配一个新的内存页，并将写入内容复制到新的页中。这一过程对其他共享该页的进程是透明的，它们仍然可以访问原始的内存页。

**copy_range函数的调用过程：do_fork()---->copy_mm()---->dup_mmap()---->copy_range()**

```cpp
int copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end,
               bool share) {
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));
    // copy content by page unit.
    do {
        // call get_pte to find process A's pte according to the addr start
        pte_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PTSIZE, PTSIZE); 
            continue;
        }
        // call get_pte to find process B's pte according to the addr start. If
        // pte is NULL, just alloc a PT
        if (*ptep & PTE_V) {
            if ((nptep = get_pte(to, start, 1)) == NULL) {
                return -E_NO_MEM;
            }
            uint32_t perm = (*ptep & PTE_USER);
            // get page from ptep
            struct Page *page = pte2page(*ptep);
            // alloc a page for process B
            struct Page *npage = alloc_page();
            assert(page != NULL);
            assert(npage != NULL);
            int ret = 0;
            /* LAB5:EXERCISE2 2212789
             * replicate content of page to npage, build the map of phy addr of
             * nage with the linear addr start
             *
             * Some Useful MACROs and DEFINEs, you can use them in below
             * implementation.
             * MACROs or Functions:
             *    page2kva(struct Page *page): return the kernel vritual addr of
             * memory which page managed (SEE pmm.h)
             *    page_insert: build the map of phy addr of an Page with the
             * linear addr la
             *    memcpy: typical memory copy function
             *
             * (1) find src_kvaddr: the kernel virtual address of page
             * (2) find dst_kvaddr: the kernel virtual address of npage
             * (3) memory copy from src_kvaddr to dst_kvaddr, size is PGSIZE
             * (4) build the map of phy addr of  nage with the linear addr start
             */
            void *src_kvaddr = page2kva(page);
            void *dst_kvaddr = page2kva(npage);
            memcpy(dst_kvaddr, src_kvaddr, PGSIZE);
            ret = page_insert(to, npage, start, perm);

            assert(ret == 0);
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}
```

##### 实现步骤

##### 1. **确保地址对齐和合法性**

```cpp
assert(start % PGSIZE == 0 && end % PGSIZE == 0);
assert(USER_ACCESS(start, end));
```

- **目的**：确保开始地址 (`start`) 和结束地址 (`end`) 都是页对齐的，即地址必须是页大小（`PGSIZE`）的整数倍。这样做是为了避免在不合法的内存位置进行操作，确保内存访问不会越界。
- **操作**：
  - `start % PGSIZE == 0` 和 `end % PGSIZE == 0`：确保开始和结束地址都能被页大小整除。
  - `USER_ACCESS(start, end)`：这是一个检查宏，用于验证给定的内存范围是否位于用户空间内，防止访问内核空间。

##### 2. **按页单位复制内存**

```cpp
do {
    pte_t *ptep = get_pte(from, start, 0), *nptep;
    if (ptep == NULL) {
        start = ROUNDDOWN(start + PTSIZE, PTSIZE); 
        continue;
    }
```

- **目的**：按页单位逐页复制内存。对于每个内存区间中的页，检查源进程是否有有效的页表项（PTE）。如果源页表项不存在，则跳过该页并处理下一个页。
- **操作**：
  - `get_pte(from, start, 0)`：通过 `get_pte` 函数获取源进程 (`from`) 中给定虚拟地址 (`start`) 的页表项（PTE）。如果该地址没有有效的 PTE（即页表项不存在），则跳过此页，继续检查下一个页。`ROUNDDOWN(start + PTSIZE, PTSIZE)` 用于调整 `start` 地址到下一个页边界，确保处理的地址是页对齐的。

##### 3. **处理有效的源页**

```cpp
if (*ptep & PTE_V) {
    if ((nptep = get_pte(to, start, 1)) == NULL) {
        return -E_NO_MEM;
    }
    uint32_t perm = (*ptep & PTE_USER);
    struct Page *page = pte2page(*ptep);
    struct Page *npage = alloc_page();
    assert(page != NULL);
    assert(npage != NULL);
```

- **目的**：检查源页是否有效（即源页是否在内存中），并为目标进程分配一个新的页表项和新的物理页。如果目标页表项不存在，就分配一个新的页表项。
- **操作**：
  - `*ptep & PTE_V`：检查源进程的页表项是否有效（`PTE_V` 标志表示该页存在且有效）。
  - `get_pte(to, start, 1)`：尝试为目标进程（`to`）获取对应虚拟地址的页表项，如果没有对应的页表项，则分配一个新的页表项（`nptep`）。如果分配失败，返回错误码 `-E_NO_MEM`。
  - `(*ptep & PTE_USER)`：提取源页表项中的权限位，判断该页是否属于用户空间。
  - `pte2page(*ptep)`：从源页表项中获取对应的页结构体（`page`）。
  - `alloc_page()`：为目标进程分配一个新的物理页面（`npage`）。

##### 4. **为目标进程分配新的内存页**

```cpp
void *src_kvaddr = page2kva(page);
void *dst_kvaddr = page2kva(npage);
memcpy(dst_kvaddr, src_kvaddr, PGSIZE);
ret = page_insert(to, npage, start, perm);
```

- **目的**：将源页内容复制到新分配的目标页，并将该目标页插入到目标进程的页表中。
- **操作**：
  - `page2kva(page)`：将源页转换为内核虚拟地址（`src_kvaddr`），通过该虚拟地址访问源页的内存内容。
  - `page2kva(npage)`：将目标页转换为内核虚拟地址（`dst_kvaddr`），用于存放从源页复制过来的数据。
  - `memcpy(dst_kvaddr, src_kvaddr, PGSIZE)`：将源页的内容复制到目标页。复制的大小是一个页的大小（`PGSIZE`）。
  - `page_insert(to, npage, start, perm)`：将新分配的目标页 (`npage`) 插入到目标进程的页表中，并为该页设置相应的权限（`perm`）。

##### 5. **更新目标进程的页表**

```cpp
assert(ret == 0);
```

- **目的**：确保目标页成功插入到目标进程的页表中，并且页表更新没有出现错误。
- **操作**：
  - `assert(ret == 0)`：检查 `page_insert` 返回值，确保页成功插入目标进程的页表。如果插入失败，`page_insert` 应返回一个非零值，程序会触发断言并终止执行。

##### 循环和结束

```cpp
start += PGSIZE;
} while (start != 0 && start < end);
return 0;
```

- **目的**：继续遍历每个地址，直到处理完所有页。
- **操作**：
  - `start += PGSIZE`：移动到下一个页的开始地址。
  - `while (start != 0 && start < end)`：循环条件保证了不会进入无限循环，同时确保处理的地址范围在有效的内存区间内。

最终，函数返回 0，表示复制操作成功完成。

COW 机制通过懒复制的策略，实现了内存的高效利用。当多个进程共享同一内存页时，只有在发生写操作时，才会真正进行内存的复制操作，从而显著节省了内存资源，并提高了系统的运行效率。这一机制在现代操作系统中得到了广泛应用，尤其是在进程创建时（如 `fork()` 系统调用）和内存管理中，极大地减少了不必要的内存拷贝操作。

--- 

### 练习3: 阅读分析源代码，理解进程执行 fork/exec/wait/exit 的实现，以及系统调用的实现（不需要编码）

请在实验报告中简要说明你对 fork/exec/wait/exit函数的分析。并回答如下问题：

- 请分析fork/exec/wait/exit的执行流程。重点关注哪些操作是在用户态完成，哪些是在内核态完成？内核态与用户态程序是如何交错执行的？内核态执行结果是如何返回给用户程序的？
- 请给出ucore中一个用户态进程的执行状态生命周期图（包执行状态，执行状态之间的变换关系，以及产生变换的事件或函数调用）。（字符方式画即可）

执行：make grade。如果所显示的应用程序检测都输出ok，则基本正确。（使用的是qemu-1.0.1）

**回答：**

## 三、实验中的知识点

#### 3.1 用户进程与内核进程

**用户进程**和**内核进程**是操作系统中的两个重要概念，它们分别代表了系统中的两种主要的进程类型，具有不同的权限和职责。理解这两者的区别对于掌握操作系统的基本原理至关重要。

- **用户进程**：
  用户进程是由用户启动并运行的程序。它们在操作系统的用户空间中运行，通常不能直接访问硬件资源或操作系统内核。用户进程通过系统调用（如 `read()`, `write()` 等）来请求内核提供的服务和资源。例如，常见的应用程序，如文本编辑器、浏览器、数据库等，都是用户进程。用户进程的运行受限于操作系统的安全机制，操作系统会通过进程调度、内存管理等方式来保护用户进程的运行不受其他进程的干扰。
  
  用户进程和内核进程的最大区别在于**权限**。用户进程无法直接访问硬件，也无法直接操作内核空间。为了访问这些资源，用户进程必须通过系统调用将请求交给内核处理。操作系统内核负责管理和调度所有的进程，并提供操作系统级别的服务，如文件系统、网络通信等。

- **内核进程**：
  内核进程是操作系统内核的一部分，直接运行在内核空间中。它们具有比用户进程更高的权限，可以直接访问硬件设备、内存和所有系统资源。内核进程负责执行与系统管理、硬件控制、设备驱动等相关的操作。内核进程通常是由操作系统内核启动和管理的，用于维护系统的基本功能。
  
  内核进程通常在特权模式（也叫内核模式）下执行，这意味着它们拥有访问所有内存、I/O 设备和硬件资源的权限。而用户进程则在用户模式下运行，权限受到严格的限制。用户进程与内核进程之间的切换通常通过系统调用进行，用户进程通过系统调用进入内核模式，内核进程执行完毕后，再切换回用户模式。

#### 3.2 Copy On Write（COW）

**Copy On Write（COW）** 是一种优化内存管理和资源共享的技术，广泛用于现代操作系统中，尤其是在多任务处理和虚拟内存管理中。COW 的基本思想是在多个进程共享相同资源时，只有在某个进程修改该资源时，才会为该进程创建一个副本。这样，在没有修改的情况下，多个进程可以共享同一份内存，从而节省系统的内存资源。

##### 3.2.1 COW 的工作原理

COW 的工作原理可以通过以下步骤进行描述：

1. **共享内存**：
   当一个进程（例如 A 进程）创建一个副本（例如，使用 `fork()` 创建子进程 B）时，父进程和子进程会共享相同的内存页面。此时，父子进程使用相同的物理内存页，而这些页是只读的。操作系统会设置页表项中的“只读”标志，防止这两个进程同时修改这些页面。通过这种方式，多个进程可以共享内存，减少内存的重复使用，从而提高内存使用效率。

2. **写时复制**：
   当进程 A 或进程 B 中的任意一个进程尝试修改共享内存时，操作系统会发生页面错误（Page Fault）。当检测到写操作时，操作系统会触发一个中断，将该进程的内存页面复制一份，并将修改操作应用到新复制的页面上。此时，修改后的页面只属于发起写操作的进程，原本的页面仍然供其他进程共享使用。

3. **避免不必要的复制**：
   在没有进行写操作的情况下，父子进程继续共享相同的内存页面，避免了不必要的内存复制。当进程没有修改内存时，COW 可以有效减少内存占用，提高性能。

##### 3.2.2 COW 的优势

- **内存效率**：
  COW 可以显著提高内存利用率。多个进程在没有修改内存内容时，可以共享相同的物理内存页，避免了每个进程都需要有一份独立的内存副本。内存复制只有在发生写操作时才会进行，从而减少了不必要的内存复制。

- **性能优化**：
  COW 能减少系统中不必要的内存分配和复制操作，尤其是在进程创建时（如 `fork()` 系统调用），父子进程之间不需要复制整个地址空间。只有在需要写入时，才会触发内存的复制操作，延迟了内存复制的时机，从而提高了系统的性能。

- **避免不必要的副本**：
  对于大量只读的子进程，COW 可以有效避免不必要的内存副本。例如，多个子进程都继承了相同的只读内存内容时，可以共用一块内存，而不需要为每个进程复制一份内存。

##### 3.2.3 COW 的应用场景

- **进程创建**：在 `fork()` 系统调用中，当子进程创建时，操作系统并不立即复制父进程的整个地址空间。相反，它们共享相同的内存页面，直到其中一个进程对这些页面进行写操作。此时，系统会为该进程创建新的内存副本，这种策略大大减少了 `fork()` 调用时的内存开销。

- **内存映射文件**：当进程通过内存映射（`mmap()`）映射文件时，如果多个进程映射同一个文件，操作系统也可以利用 COW 技术共享内存，直到其中一个进程修改文件内容。

- **虚拟内存管理**：COW 在虚拟内存管理中发挥了重要作用，尤其在实现分页机制时，允许多个进程共享内存，而在需要修改时再进行实际的内存复制。这种懒惰复制的方式可以显著减少内存占用和提高系统性能。

##### 3.2.4 COW 的挑战和解决方案

尽管 COW 在内存管理中具有显著的优势，但它也面临一些挑战：

- **性能问题**：当频繁进行写操作时，COW 可能会导致频繁的页面复制，从而影响系统性能。为了优化这一问题，操作系统可以采用更高效的内存管理策略，减少不必要的复制操作。

- **进程之间的同步**：在多进程环境下，COW 可能导致进程之间的同步问题，特别是在多个进程同时操作共享资源时。操作系统需要通过合适的同步机制来保证数据一致性。

##### 3.2.5 总结

Copy On Write 是一种有效的内存管理技术，它通过延迟复制内存直到发生写操作，从而节省了内存空间，并提高了系统性能。在多进程系统中，COW 技术的广泛应用可以显著减少内存开销，优化进程创建和内存管理的效率。然而，COW 也有一定的性能和同步问题，操作系统需要在设计时充分考虑这些因素，以实现最佳的资源管理。
