<center> 

# 实验二实验报告

## **实验名称：物理内存和页表**   



---

## 一、实验目的

* 掌握并理解页表的建立和使用方法

* 明白操作系统物理内存的管理

* 明白页的分配方法

---

## 二、实验过程

### 1.练习一：理解 first-fit 连续物理内存分配算法（思考题）

**First-Fit** 内存分配算法的基本思想是：内存管理器维护一个空闲块列表，当接收到内存分配请求时，它会扫描列表并找到第一个足够大的空闲块。如果找到的块比请求的大得多，通常会将其拆分，剩余部分作为新的空闲块继续留在列表中。

在本实验中该算法的实现位于`default_pmm.c`文件中，在该文件实现了对于内存页面的分配、释放等操作。下面我们对该文件中的四个主要函数:`default_init`、`default_init_memmap`、`default_alloc_pages`、`default_free_pages`分别进行分析。

#### 页面初始化1

```c
free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

static void
default_init(void) {
    list_init(&free_list);
    nr_free = 0;
}
```

* 该函数主要初始化空闲列表 `free_list`，并将空闲页面计数 `nr_free` 设为 0，表示当前系统没有可用的空闲内存块。其中`free_list`和`nr_free`都是结构体`free_area`的数据域，即该结构体保存了空闲物理页的所有信息。

* 其中函数`list_init()`是一个链表初始化函数，传递给它的是 `free_list`，即一个代表空闲内存块的双向链表的头结点。`list_init()`的作用是将 `free_list` 初始化为一个空链表。一个空链表的特征是它的前驱和后继节点都指向它自己，即 `free_list` 的 `prev` 和 `next` 都指向 `free_list`，表示链表中当前没有任何元素。

* 初始化 `nr_free` 为 0，表示当前系统还没有任何空闲页面。随着系统的运行，后续通过函数 `default_init_memmap` 向链表中添加空闲页面时，`nr_free` 会被增加，以反映当前系统中实际的空闲内存块数量。

#### 页面初始化2

`default_init_memmap()` 函数的主要作用是初始化一块指定的物理内存区域，将这块区域标记为空闲，并将其加入到管理空闲内存的链表 `free_list` 中，同时更新空闲页面的总数 `nr_free`。这个函数在内存管理系统中负责管理和初始化新的一块可用内存块。其具体代码如下：

```c
static void
default_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }
}
```

* 首先参数`struct Page *base`是一个指向物理内存页面数组的指针，表示内存的起始位置，每个 `Page` 结构体代表一个物理页面；参数`size_t n`：表示这块区域的大小，即页面的数量，`n` 指定了有多少个页面从 `base` 开始需要被初始化为空闲状态。

* 然后`assert(n>0)`的作用是确保`n`的值大于0，表示要初始化的页面数量必须有效。这是一个安全检查，防止无效调用。

* 进入循环体`for (; p != base + n; p++)`之后，该部分代码遍历从 `base` 开始的每个页面，直到 `base + n` 结束，逐个初始化这些页面的相关属性：
  
  * `assert(PageReserved(p))`：确保这些页面当前已经被标记为保留状态，表明这些页面之前没有被初始化为自由页面。
  
  * `p->flags = p->property = 0`：将页面的 `flags` 标志位和 `property` 属性清零。`flags` 表示页面的状态，`property` 用来记录空闲块的大小，在这里它被重置。
  
  * `set_page_ref(p, 0)`：将页面的引用计数 `ref` 设为 0，表示该页面当前没有任何用户使用。

* 标记第一个页面并更新页面总数。`base->property = n`：将 `base` 指向的第一个页面的 `property` 属性设置为 `n`，表示这是一个大小为 `n` 的空闲块；`SetPageProperty(base)`：将 `base` 标记为拥有 `PG_property` 属性，表示这个页面是一个空闲块的起始页面（可用来分配）；`nr_free += n`的作用是将 `n` 加到全局变量 `nr_free`，以更新系统中的空闲页面总数，表示当前系统中新增了 `n` 个可用页面。

* 将新空闲块插入空闲列表。如果链表为空，表示这是系统中第一个空闲块，直接将 `base` 所指向的块插入链表中；如果链表非空，通过函数`list_next(le)`遍历空闲链表，寻找合适的位置插入新的空闲块。然后`if (base < page)`判断 `base` 是否位于当前链表节点之前。因为链表通常是按地址顺序排列的，新的空闲块需要插入到适当的位置以保持链表的有序性;如果遍历到链表的末尾都没有找到合适的位置（即 `base` 的地址大于当前链表中的所有块），则将 `base` 插入到链表的末尾。

* 总结：`default_init_memmap()`一共做了4件事：1）初始化每个页面，将它们标记为空闲；2）设置第一个页面的 `property` 属性，表示该空闲块的大小；3）更新系统的空闲页面总数 `nr_free`；4）将新初始化的空闲块插入到空闲链表中，确保链表按地址顺序排列。

#### 页面分配

`default_alloc_pages(size_t n)` 是一个用于分配页面的函数，依据 **First-Fit** 内存分配算法来实现。在该函数中，内存管理系统从空闲内存块链表中找到第一个大小足够的块，分配所请求的页面数 `n`，如果找到的块比需要的页面数多，还会将剩余的部分重新作为空闲块插入到链表中。这个函数实现了基本的内存页面分配功能。  其代码如下所示：

```c
static struct Page *
default_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    if (page != NULL) {
        list_entry_t* prev = list_prev(&(page->page_link));
        list_del(&(page->page_link));
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            SetPageProperty(p);
            list_add(prev, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;
}
```

* 首先函数参数`size_t n`表示需要分配的页面数，即用户请求分配的连续物理页面数量。

* 检查完安全性后，检查空闲页面的总数，代码如下。检查系统中剩余的空闲页面总数 `nr_free` 是否足够满足请求。如果请求的页面数 `n` 大于系统中当前可用的页面数 `nr_free`，则返回 `NULL`，表示无法分配页面。
  
  ```c
  if (n > nr_free) {
      return NULL;
  }
  ```

* 然后遍历空闲链表寻找合适的空闲块。进入`while`循环体之后该部分代码开始遍历空闲页面链表 `free_list`，寻找第一个大小足够的块来满足分配请求。通过判断`p->property >= n`检查当前块的 `property` 属性是否大于等于 `n`。`property` 表示该块的空闲页面数。如果找到符合条件的块，则将该块的起始页面指针存储到 `page`，并跳出循环。

* 之后处理找到的空闲块：
  
  * 从链表中删除找到的空闲块。`list_prev(&(page->page_link))`：获取 `page` 前一个链表节点，用于后续插入拆分后的空闲块。然后`list_del(&(page->page_link))`：将找到的块 `page` 从空闲链表中移除，因为该块将被部分或全部用于分配
  
  * 拆分剩余的空闲块。如果找到的块 `page` 比请求的页面数 `n` 大，则需要将多余的页面重新作为新的空闲块插回链表。`struct Page *p = page + n;`：计算新的空闲块的起始位置，即从 `page` 向后偏移 `n` 个页面的地址处；`p->property = page->property - n;`：更新新空闲块的大小，即原始块的大小减去已经分配的页面数 `n`。`SetPageProperty(p);`：标记新空闲块的 `PG_property`，表示它是一个空闲块。`list_add(prev, &(p->page_link));`：将新空闲块插入到链表中原始块的前一个位置，保持链表的顺序。
  
  * 更新空闲页面总数并标记分配的块。`nr_free -= n;`：更新全局变量 `nr_free`，减少 `n` 个空闲页面数；`ClearPageProperty(page);`：清除 `page` 块的 `PG_property` 标志位，表示该块已被分配，不再是空闲块。

* 最后返回`page`，以供调用者使用，如果没有则返回`NULL`。

#### 页面释放

`default_free_pages(struct Page *base, size_t n)` 函数的作用是释放从 `base` 开始的 `n` 个连续页面，并将它们重新插入到空闲页面链表 `free_list` 中。函数还会尝试将相邻的空闲页面合并，避免内存碎片的产生。其代码如下所示：

```c
static void default_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }

    list_entry_t* le = list_prev(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
        }
    }

    le = list_next(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }
}
```

* 首先解释参数：1）`struct Page *base`：指向要释放的第一个页面，表示从这个地址开始的内存块；2）`size_t n`：要释放的连续页面数，即从 `base` 开始的 `n` 个物理页面。

* 通过循环遍历的方式重置每一个页面的状态。通过循环体`for (; p != base + n; p++)`将页面的标志位 `flags` 重置为 0，表示该页面现在为空闲状态，并将页面的引用计数 `ref` 设为 0，表示没有任何用户正在使用该页面。

* 然后再设置空闲块属性。首先将第一个页面的 `property` 属性设置为 `n`，表示从 `base` 开始有 `n` 个连续的空闲页面；然后将 `PG_property` 属性设置到第一个页面，标记它是一个新的空闲块的起始页面。最后将释放的页面数 `n` 加到全局变量 `nr_free` 中，更新系统中空闲页面的总数。
  
  ```c
  base->property = n;
  SetPageProperty(base);
  nr_free += n;
  ```

* 再将空闲块加入空闲链表中。同样进行判断，如果空闲链表为空则直接插入；如果空闲链表非空，则像前面的方法通过找到合适的位置进行插入。

* 最后进行合并相邻的块：
  
  * 合并前面的相邻块：通过找到`base`前面空闲块，再检查前一个块的结束位置是否与当前块的起始位置相邻。如果相邻，则可以合并。将前一个块 `p` 的 `property` 属性增加到当前块的大小，表示这两个块合并为一个更大的块。再清除 `base` 的 `PG_property` 标志位，因为它已经被合并到前面的块。最后删除当前块，更新合并的块即可。
  
  * 合并后面的相邻块：与合并前面的相邻块的方法相似，不同是合并前面的相邻块清除的是`base`的标记位，合并后面的块清除的是后一个块`p`的标记位。

通过对以上四个函数的分析，我们对`first-fit` 连续物理内存分配算法有了基本的了解。系统启动后，我们把`pmm_manager`的值赋值为`default_pmm_manager`，从而使得操作系统使用`default_pmm_manager`中的这些物理页的分配和释放的方法来管理物理内存。

---

### 2.练习二：实现 Best-Fit 连续物理内存分配算法（需要编程）

#### 设计与实现过程

**Best-Fit** 内存分配算法的基本思想是：当接收到内存分配请求时，内存管理器会遍历空闲块列表，找到**大小最接近但足够大的空闲块**进行分配。如果找到的块比请求的大得多，通常会将其拆分，剩余部分作为新的空闲块继续留在列表中。Best-Fit 相比于 First-Fit 算法更擅长减少碎片化，但查找过程较慢，因为它需要遍历整个列表。

在本实验中，Best-Fit 算法的实现主要通过两个函数实现内存的分配和释放，分别是：`best_fit_alloc_pages` 和 `best_fit_free_pages`。其余函数均采取原方案，下面对这两个函数进行分析。


##### 页面分配
`best_fit_alloc_pages` 的作用是从空闲页面链表中找到一个大小最接近且足够大的块来满足分配请求。该块如果比所需页面数多，会进行拆分，将剩余部分保留在空闲链表中。

```c
static struct Page *
best_fit_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }

    struct Page *best_page = NULL;
    list_entry_t *le = &free_list;
    size_t best_fit_size = nr_free +1;

    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n && p->property < best_fit_size) {
            best_page = p;
            best_fit_size = p->property;
        }
    }

    if (best_page != NULL) {
        list_entry_t *prev = list_prev(&(best_page->page_link));
        list_del(&(best_page->page_link));

        if (best_page->property > n) {
            struct Page *p = best_page + n;
            p->property = best_page->property - n;
            SetPageProperty(p);
            list_add(prev, &(p->page_link));
        }

        nr_free -= n;
        ClearPageProperty(best_page);
    }

    return best_page;
}
```
- 检查请求页面数是否合理：
`best_fit_alloc_pages` 函数是 `Best-Fit` 算法的核心实现。函数首先使用 `assert(n > 0)` 确保请求的页面数` n` 大于 0，表示这是一个有效的请求。如果系统中空闲页面总数不足，则返回 `NULL`。

- 遍历空闲链表：
通过 `while` 循环遍历整个空闲链表 `free_list`，检查每个空闲块的 `property` 值，判断它是否能够满足请求页面数 `n`。具体来说，条件 `p->property >= n` 确保当前空闲块的大小至少能满足请求，`p->property < best_fit_size` 确保当前找到的块比之前找到的最小块更合适。遍历过程中，`best_page` 会更新为最合适的空闲块，`best_fit_size` 记录该块的大小。

- 找到合适的块后进行分配：
如果找到最合适的块 `best_page`，则从链表中移除该块。如果该块比请求的页面数多（即 `p->property > n`），则将多余的页面拆分成一个新的块，并将其重新插入到空闲链表中。拆分后的块的大小为原块大小减去请求页面数 `n`。

- 更新全局变量并返回：
最后，减少全局变量 `nr_free` 中的空闲页面数，并清除分配块的 `PG_property` 标志，标志它已经被分配。然后返回找到并分配的 `Page` 块。

##### 页面释放
`best_fit_free_pages` 函数的作用是释放从 `base` 开始的 `n` 个连续页面，并将它们重新插入到空闲链表`free_list` 中。函数还会尝试将相邻的空闲块进行合并，减少内存碎片。

```c
static void
best_fit_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t *le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page *page = le2page(le, page_link);
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }

    list_entry_t *le = list_prev(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
        }
    }

    le = list_next(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }
}
```
- 初始化和安全检查：
函数首先检查释放的页面数是否大于 0，确保这是一个有效的操作。然后遍历每个页面，检查它们是否可以安全释放。通过 `assert(!PageReserved(p) && !PageProperty(p))`，确保页面没有被保留并且不是空闲块。释放的页面的标志位被清除，引用计数重置。

- 设置空闲块的属性：
将 `base->property` 设置为 `n`，表示从 `base` 开始的连续 `n` 个页面是一个新的空闲块，并通过 `SetPageProperty` 标记这个块为可用。然后将 `nr_free` 加上 `n`，更新系统中的空闲页面总数。

- 插入到空闲链表：
接着检查空闲链表 `free_list` 是否为空。如果空链表为空，直接将新的空闲块插入链表；否则，遍历链表找到合适的位置插入新块，保证链表保持按地址有序的状态。

- 合并相邻的空闲块：

与前面块合并：检查当前块的前一个块 `p` 是否与 `base` 相邻。如果前一个块的结束地址与当前块的起始地址相同，则将它们合并为一个更大的块。
与后面块合并：同样检查当前块的后一个块 `p`，如果当前块的结束地址与后一个块的起始地址相邻，也可以合并。

- 更新链表：
如果发生合并，合并后的块将被更新为新的空闲块，原来的块则从链表中移除。

#### 2. Best-Fit 算法是否有进一步的改进空间？

1. **查找效率的提升**：
   - 目前的 `Best-Fit` 实现通过遍历整个空闲链表寻找最适合的块，这种方式的时间复杂度为 O(n)，对于大规模系统来说，查找开销较大。
   - 可以考虑使用更高效的数据结构，例如**平衡二叉树**或**堆**，来加速查找过程，使得查找复杂度降低为 O(log n)。通过这样的改进，分配效率可以得到显著提升。

2. **碎片管理的优化**：
   - `Best-Fit` 虽然能减少碎片化，但随着内存分配和释放的反复进行，系统中仍可能出现较小的、难以利用的碎片。
   - 进一步的改进可以引入**内存整理**机制，定期扫描和整理内存，将零散的碎片合并为连续的内存块，以提升大块内存分配的效率。

---
### 实验总结

*通过实验，掌握了页表建立及管理物理内存的方法，理解了 First-Fit 和 Best-Fit 算法的具体实现。*

---
### 扩展练习Challenge：硬件的可用物理内存范围的获取方法（思考题）
  在RISC-V架构下使用**sv39分页机制**时，操作系统必须获取物理内存的布局，以正确映射虚拟内存到物理内存，如果操作系统无法提前知道当前硬件的可用物理内存范围，当**引导加载程序是OpenSBI**时，有下列几种方法和思路：

####  1.**通过OpenSBI API获取内存信息**
  OpenSBI作为RISC-V系统的引导加载程序，提供了对硬件的抽象层，操作系统可以调用OpenSBI提供的标准API来获取物理内存的布局信息，OpenSBI通过它的接口提供内存映射信息，允许操作系统查询可用的物理内存范围。
##### 具体方法：
 - 调用OpenSBI的API（例如** `sbi_get_memory_map` **），操作系统可以查询内存布局，包括可用内存和保留区域。
 - 这些API返回的内存映射结构中会包含物理内存段的起始地址、大小、以及这些区域是否可供操作系统使用。

####  2.**解析设备树（Device Tree, DTB）**
  OpenSBI在操作系统启动时会将设备树传递给操作系统，设备树包含硬件资源的详细描述，包括内存布局。操作系统可以解析设备树中的** `/memory` **节点，来获取物理内存的可用范围。
##### 具体步骤：
 - 操作系统读取启动时传递的设备树（由OpenSBI传递）。
 - 解析设备树中的`/memory`节点（该节点描述了物理内存的起始地址和大小）。
 - 内核根据设备树中的信息，确定哪些物理内存是可用的，并据此建立页表映射。
##### 示例设备树节点：
```dts
memory {
    device_type = "memory";
    reg = <0x00000000 0x80000000>;  // 内存起始地址和大小
};
```

####  3.**逐页探测法（Memory Probing）**
  如果操作系统无法通过OpenSBI API或设备树获取到物理内存的完整布局，可以使用**逐页探测法**，这种方法通过探测物理内存中的页来确定哪些内存可以被正常访问。
##### 方法步骤：
 - 从一个假定的最低物理地址开始（如常见的内核加载地址：`0x80000000`），逐页尝试访问物理内存。
 - 操作系统向这些页写入并读取数据，通过检测是否发生访问错误来判断这些物理页是否有效。
 - 当访问到某些保留区域或超出物理内存范围时，会出现访问错误，操作系统据此判断该页不可用。
 虽然这种方法效率较低，但它可以在没有其他硬件信息的情况下确保操作系统能获取到物理内存的实际范围。

####  4.**读取CSR（Control and Status Registers）**
  某些RISC-V处理器可能会提供一些专用的CSR（控制与状态寄存器）来表示物理内存的布局，操作系统可以在机器模式（Machine Mode）下读取这些寄存器以获取内存范围。
##### 示例：
 - 操作系统可以在机器模式下读取特定的寄存器，从而确定可用的物理内存范围。

####  5.**引导加载器传递手动配置的内存范围**
  在某些情况下，OpenSBI或其他引导加载器可能没有足够的硬件能力提供详细的内存布局信息，这时系统管理员可以手动在引导加载器配置中指定内存范围。操作系统可以从这些配置中读取内存布局。
##### 示例：
 - 操作系统启动时，可以通过内核命令行参数（如`mem=4G`）指定物理内存的大小。
---
### 总结：
 * 操作系统可以选择性的结合上述几种方法，通过设备树获取内存的初步信息，之后通过逐页探测或读取CSR进一步确认内存的可用性和范围,确保即使设备树或OpenSBI提供的信息不完整，操作系统也能正确识别物理内存。*


