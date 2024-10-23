<center> 

# 实验二实验报告

</center>

实验名称：物理内存和页表   

</center>

## 一、实验目的

* 掌握并理解页表的建立和使用方法

* 明白操作系统物理内存的管理

* 明白页的分配方法

## 二、实验过程

### 1.练习一：理解first-fit 连续物理内存分配算法（思考题）

**First-Fit** 内存分配算法的基本思想是：内存管理器维护一个空闲块列表，当接收到内存分配请求时，它会扫描列表并找到第一个足够大的空闲块。如果找到的块比请求的大得多，通常会将其拆分，剩余部分作为新的空闲块继续留在列表中。

在本实验中该算法的实现位于`default_pmm.c`文件中，在该文件实现了对于内存页面的分配、释放等操作。下面我们对该文件中的四个主要函数:`default_init`、`default_init_memmap`、`default_alloc_pages`、`default_free_pages`分别进行分析。

#### default_init()

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

#### default_init_memmap()

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

#### default_alloc_pages()

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

#### default_free_pages（）

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


