# 实验四实验报告

## **实验名称：进程管理**

---

## 一、实验目的

- 了解内核线程创建/执行的管理过程
- 了解内核线程的切换和基本调度过程

---

## 二、实验练习

### 练习1：分配并初始化一个进程控制块（需要编码）

alloc_proc函数（位于kern/process/proc.c中）负责分配并返回一个新的struct proc_struct结构，用于存储新建立的内核线程的管理信息。ucore需要对这个结构进行最基本的初始化，你需要完成这个初始化过程。

> 【提示】在alloc_proc函数的实现中，需要初始化的proc_struct结构中的成员变量至少包括：state/pid/runs/kstack/need_resched/parent/mm/context/tf/cr3/flags/name。

请在实验报告中简要说明你的设计实现过程。请回答如下问题：

> 请说明proc_struct中`struct context context`和`struct trapframe *tf`成员变量含义和在本实验中的作用是啥？（提示通过看代码和编程调试可以判断出来）

**回答：**

- **初始化过程：**

```c
proc->state = PROC_UNINIT;
proc->pid = -1;
proc->runs = 0;
proc->kstack = 0;
proc->need_resched = 0;
proc->parent = NULL;
proc->mm = NULL;
memset(&(proc->context), 0, sizeof(struct context));
proc->tf = NULL;
proc->cr3 = boot_cr3;
proc->flags = 0;
memset(proc->name, 0, PROC_NAME_LEN + 1);
```

1. `proc->state = PROC_UNINIT;`：该字段表示进程的状态，初始化为 `PROC_UNINIT`（未初始化状态），表明该进程刚刚被创建，还没有完全初始化。进程的状态通常包括如运行中、就绪、等待等状态，具体状态会根据进程的生命周期变化；
  
2. `proc->pid = -1;`：`pid` 是进程的唯一标识符，即进程 ID。它在操作系统中用于区分不同的进程。初始化为 `-1` 表示该进程还没有被分配有效的 `pid`，可能是一个尚未正式启动的进程；
  
3. `proc->runs = 0;`：`runs` 记录该进程已经运行的次数。初始化为 0 表示进程还没有开始执行过任何代码；
  
4. `proc->kstack = 0;`：`kstack` 用于指向该进程在内核态时使用的栈空间。内核栈用于存储内核模式下的函数调用和局部变量。初始化为 0 表示进程还没有分配内核栈；
  
5. `proc->need_resched = 0;`：该字段表示该进程是否需要重新调度。初始化为 0 表示该进程当前不需要调度。这个标志通常用于调度器中，用于判断当前进程是否应该被抢占；
  
6. `proc->parent = NULL;`：`parent` 指向该进程的父进程。初始化为 `NULL` 表示该进程还没有父进程，通常父进程会在进程创建时进行初始化；
  
7. `proc->mm = NULL;`：`mm` 是进程的内存管理信息结构，指向该进程的内存描述符（`mm_struct`）。该字段为空表示该进程没有关联的内存空间，通常在进程创建时会为其分配内存管理结构；
  
8. `memset(&(proc->context), 0, sizeof(struct context));`：将 `context` 结构体的所有字段初始化为 0。`context` 结构用于存储进程的寄存器状态、程序计数器等信息，在进程切换时使用。初始化为 0 是为了确保在进程切换之前，这些字段不会包含不确定的值；
  
9. `proc->tf = NULL;`：`tf` 是指向当前进程的 trapframe（陷阱帧）的指针，用于保存进程在发生中断或异常时的寄存器状态。初始化为 `NULL` 表示新进程没有发生过中断或系统调用；
  
10. `proc->cr3 = boot_cr3;`：`cr3` 是一个特定寄存器的值，指向进程的页目录（Page Directory），用于内存管理和虚拟地址转换。`boot_cr3` 是系统启动时的页目录基址，初始化时进程的 `cr3` 被设置为该值；
  
11. `proc->flags = 0;`：`flags` 用于保存进程的标志位，例如是否处于某些特定状态或是否需要执行特定操作。初始化为 0 表示没有设置任何标志；
  
12. `memset(proc->name, 0, PROC_NAME_LEN + 1);`：该语句将 `name` 数组的所有字符初始化为 0。`name` 用于存储进程的名称，`PROC_NAME_LEN` 是名称的最大长度，+1 是为了存储字符串结束符 `\0`。
  

- 问题：请说明proc_struct中`struct context context`和`struct trapframe *tf`成员变量含义和在本实验中的作用是啥？

在 `alloc_proc` 函数中，`struct proc_struct` 结构的初始化涉及多个成员变量，这些成员的含义和作用在内核线程管理中至关重要。 `struct context context` 和 `struct trapframe *tf`在实验中的含义和作用如下：

**1. `struct context context`**

- **含义**： `struct context` 主要用于保存进程的上下文信息，它包含了进程执行过程中所需的寄存器状态。上下文切换是操作系统调度机制的核心，当操作系统需要切换执行的进程时，会保存当前进程的上下文（即寄存器状态），并加载下一个进程的上下文，恢复到该进程之前的执行状态。上下文包括进程在用户态和内核态执行时的状态，比如程序计数器（PC）、栈指针（SP）等。
  
- **在本实验中的作用**：
  在本实验中，`context` 的初始化是非常重要的，因为它确保了每个新进程都有一个独立的上下文，并能够在切换时恢复它的执行状态。在 `alloc_proc` 函数中，`memset(&(proc->context), 0, sizeof(struct context))` 会将 `context` 结构的内容清零，确保其在初始化时不包含任何不必要的值，这样在上下文切换时能够可靠地保存和恢复进程的执行状态。
  

**2. `struct trapframe *tf`**

- **含义**： `struct trapframe` 用于保存进程在发生中断或异常时的寄存器状态，尤其是进入内核态时（如系统调用、硬件中断等）。在操作系统中，当一个进程从用户态进入内核态时，CPU会触发一个中断或异常，操作系统需要保存进程的状态（包括寄存器等）以便稍后恢复。`trapframe` 就是用于存储这些寄存器状态的结构。
  
- **在本实验中的作用**：
  在本实验中，`tf` 变量指向一个 `trapframe` 结构，用于保存进程在触发中断或系统调用时的寄存器状态。`proc->tf = NULL` 表示新创建的进程还没有绑定具体的中断处理框架，它的 `trapframe` 是空的。稍后在进程调度和中断处理过程中，这个 `trapframe` 会被填充为有效的值，用于记录进程的执行状态，确保进程在中断后能够恢复正确的执行路径。
  

在 `alloc_proc` 函数中，`context` 被清零以确保其状态初始化为一个已知的值，而 `tf` 被初始化为 `NULL`，这表明新进程尚未经历任何中断或系统调用。

### 练习2：练习2：为新创建的内核线程分配资源（需要编码）

创建一个内核线程需要分配和设置好很多资源。kernel_thread函数通过调用**do_fork**函数完成具体内核线程的创建工作。do_kernel函数会调用alloc_proc函数来分配并初始化一个进程控制块，但alloc_proc只是找到了一小块内存用以记录进程的必要信息，并没有实际分配这些资源。ucore一般通过do_fork实际创建新的内核线程。do_fork的作用是，创建当前内核线程的一个副本，它们的执行上下文、代码、数据都一样，但是存储位置不同。因此，我们**实际需要"fork"的东西就是stack和trapframe**。在这个过程中，需要给新内核线程分配资源，并且复制原进程的状态。你需要完成在kern/process/proc.c中的do_fork函数中的处理过程。它的大致执行步骤包括：

- 调用alloc_proc，首先获得一块用户信息块。
- 为进程分配一个内核栈。
- 复制原进程的内存管理信息到新进程（但内核线程不必做此事）
- 复制原进程上下文到新进程
- 将新进程添加到进程列表
- 唤醒新进程
- 返回新进程号

请在实验报告中简要说明你的设计实现过程。

```cpp
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    
    // 检查是否达到最大进程数限制
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }

    ret = -E_NO_MEM;
    
    // 1. 调用 alloc_proc 为新进程分配 proc_struct
    proc = alloc_proc();
    if (proc == NULL) {
        goto bad_fork_cleanup_kstack;
    }

    // 2. 为新进程分配一个唯一的进程 ID
    proc->pid = get_pid();

    // 3. 为新进程分配内核栈
    setup_kstack(proc);

    // 4. 根据 clone_flags 复制或共享父进程的内存管理信息
    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_proc;
    }

    // 5. 复制父进程的上下文信息（trapframe）到新进程
    copy_thread(proc, stack, tf);

    // 6. 将新进程加入进程哈希表和进程队列
    hash_proc(proc);
    list_add(&proc_list, &(proc->list_link));

    // 7. 唤醒新进程，使其变为可调度状态
    wakeup_proc(proc);

    // 设置返回值为新进程的 PID
    ret = proc->pid;

fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);  // 释放内核栈
bad_fork_cleanup_proc:
    kfree(proc);  // 释放进程控制块
    goto fork_out;
}

```
`do_fork`函数完善如上所示，下面我们介绍我们的设计实现过程：

- 检查是否超出最大进程数限制
  
```c
if (nr_process >= MAX_PROCESS) {
    goto fork_out;
}
```
首先，我们检查系统当前运行的进程数量 `nr_process` 是否超过了最大进程数 `MAX_PROCESS`。如果超出，我们直接跳转到 `fork_out` 并返回错误代码 `-E_NO_FREE_PROC`，表示没有足够的空间来创建新进程。

- 分配进程控制块
  
调用 `alloc_proc()` 函数分配一个新的 `proc_struct`，即新进程的控制块。如果分配失败，`proc` 将是 NULL，此时我们需要清理资源并返回错误。
```c
proc = alloc_proc();
if (proc == NULL) {
    goto bad_fork_cleanup_kstack;
```
`alloc_proc()` 负责为新进程分配一块内存，并初始化进程的基本信息，例如进程状态、优先级、指向内核栈的指针等。如果分配失败，我们会跳转到错误处理部分 `bad_fork_cleanup_kstack` 来释放资源。

- 分配唯一的进程 ID
  
为新进程分配一个唯一的进程 `ID（PID）`。这里使用 `get_pid()` 函数为新进程生成一个唯一的 `PID`。生成的 `PID` 被存储在 `proc->pid` 中。
```c
proc->pid = get_pid();
```

- 为新进程分配内核栈

调用 `setup_kstack(proc)` 为新进程分配一个内核栈。每个进程都需要一个内核栈来保存其内核态执行的上下文。
```c
setup_kstack(proc);
```

- 复制父进程的内存管理信息

根据 `clone_flags` 参数，判断是共享父进程的内存管理信息（CLONE_VM）还是复制一份新的内存空间。这里调用 `copy_mm(clone_flags, proc)` 来完成内存管理信息的复制或共享。如果复制失败，我们需要清理资源并返回错误。
```c
if (copy_mm(clone_flags, proc) != 0) {
    goto bad_fork_cleanup_proc;
}
```

`copy_mm()` 函数根据 `clone_flags` 决定是否进行内存管理信息的共享或复制。如果是线程创建，可能只需要共享内存（CLONE_VM），否则就需要为新进程分配独立的内存。

- 复制父进程的上下文

调用 `copy_thread(proc, stack, tf)` 将父进程的上下文（包括 `trapframe` 和寄存器值等）复制到新进程中。在 `copy_thread()` 函数中，父进程的 `trapframe` 会被复制到新进程的内核栈中，并且设置新进程的内核入口点。
```c
copy_thread(proc, stack, tf);
```


`trapframe` 存储了当前进程的寄存器状态，包含程序计数器、堆栈指针等。`copy_thread()` 会将这些信息复制到新进程的内核栈中，确保新进程从正确的位置继续执行。

- 将新进程加入进程列表和哈希表

调用 `hash_proc(proc)` 将新进程加入进程的哈希表，并调用 `list_add(&proc_list, &(proc->list_link))` 将新进程加入到进程队列 `proc_list` 中。
```c
hash_proc(proc);
list_add(&proc_list, &(proc->list_link));
```

进程列表和哈希表帮助调度器快速找到进程，并能够根据哈希值快速定位进程。

- 唤醒新进程

调用 `wakeup_proc(proc)` 将新进程的状态设置为 `PROC_RUNNABLE`，表示新进程已经准备好并且可以被调度器调度执行。
```c
wakeup_proc(proc);
```

这使得新进程进入可运行状态，调度器可以在后续的调度周期中将其选中并运行。

错误处理
在进程创建的过程中，可能会发生内存分配失败、内存管理复制失败等错误。因此，错误处理非常重要。

如果 `alloc_proc()` 失败，我们释放已经分配的内核栈并返回。
如果 `copy_mm()` 失败，我们释放进程控制块并返回。
这些错误处理确保了系统资源不会泄漏，并且如果创建进程失败，系统可以稳定运行。
```c
bad_fork_cleanup_kstack:
    put_kstack(proc);  // 释放内核栈
bad_fork_cleanup_proc:
    kfree(proc);  // 释放进程控制块
    goto fork_out;
```

- 返回新进程的 PID

最后，`do_fork` 函数返回新进程的 `PID`。这个 `PID` 是通过 `get_pid()` 获取的，确保每个进程在系统中的标识是唯一的。
```c
ret = proc->pid;
```

请回答如下问题：

- 请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由。

**回答：**
uCore 通过get_pid()函数给每一个新fork线程分配id，而在 get_pid() 函数中，其通过递增 PID、遍历进程列表并检查是否已被使用的方式，确保了每个新 fork 的进程都分配了一个唯一的 PID。因此，uCore 实现了为每个新 fork 的线程分配唯一的 ID。

### 练习3：编写proc_run 函数（需要编码）

proc_run用于将指定的进程切换到CPU上运行。它的大致执行步骤包括：

- 检查要切换的进程是否与当前正在运行的进程相同，如果相同则不需要切换。
- 禁用中断。你可以使用`/kern/sync/sync.h`中定义好的宏`local_intr_save(x)`和`local_intr_restore(x)`来实现关、开中断。
- 切换当前进程为要运行的进程。
- 切换页表，以便使用新进程的地址空间。`/libs/riscv.h`中提供了`lcr3(unsigned int cr3)`函数，可实现修改CR3寄存器值的功能。
- 实现上下文切换。`/kern/process`中已经预先编写好了`switch.S`，其中定义了`switch_to()`函数。可实现两个进程的context切换。
- 允许中断。

请回答如下问题：

- 在本实验的执行过程中，创建且运行了几个内核线程？

完成代码编写后，编译并运行代码：make qemu

如果可以得到如 附录A所示的显示内容（仅供参考，不是标准答案输出），则基本正确。

**回答：**

### 扩展练习Challenge：

- 说明语句`local_intr_save(intr_flag);....local_intr_restore(intr_flag);`是如何实现开关中断的？

**回答：**
首先在`kern\sync\sync.h`中找到代码：

```c
static inline bool __intr_save(void) {
    if (read_csr(sstatus) & SSTATUS_SIE) {
        intr_disable();
        return 1;
    }
    return 0;
}

static inline void __intr_restore(bool flag) {
    if (flag) {
        intr_enable();
    }
}

#define local_intr_save(x) \
    do {                   \
        x = __intr_save(); \
    } while (0)
#define local_intr_restore(x) __intr_restore(x);

#endif /* !__KERN_SYNC_SYNC_H__ */
```

函数 `local_intr_save(intr_flag);` 和 `local_intr_restore(intr_flag);` 实现了对中断的开关控制，具体功能如下：

**1. `local_intr_save(intr_flag);`**

这条语句通过调用 `__intr_save()` 函数来禁用中断并保存当前中断的状态。它的实现过程如下：

- **`__intr_save()` 函数**：
  
  ```c
  static inline bool __intr_save(void) {
      if (read_csr(sstatus) & SSTATUS_SIE) {
          intr_disable();   // 如果当前允许中断 (SSTATUS_SIE标志位为1)，则禁用中断
          return 1;          // 返回 1 表示中断之前是启用的状态
      }
      return 0;              // 如果中断本来就已经禁用，返回 0
  }
  ```
  
  - **中断状态检查**：首先通过 `read_csr(sstatus)` 读取当前中断状态寄存器 `sstatus`。如果 `SSTATUS_SIE` 标志位被设置为 1，表示当前中断是启用的。
  - **禁用中断**：如果中断启用 (`SSTATUS_SIE` 为 1)，调用 `intr_disable()` 函数禁用中断。`intr_disable()` 函数通常是通过修改 CSR 寄存器来清除 `SSTATUS_SIE` 位，确保中断被禁用。
  - **保存中断状态**：返回值为 1，表示中断状态之前是启用的。如果中断本来已经禁用，返回值为 0。
  
  最终，`local_intr_save(intr_flag)` 会将当前中断的启用状态（是否启用中断）保存到 `intr_flag` 变量中，并禁用中断（如果中断是启用的状态）。
  

**2. `local_intr_restore(intr_flag);`**

这条语句则恢复之前保存的中断状态。它的实现过程如下：

- **`__intr_restore(flag)` 函数**：
  
  ```c
  static inline void __intr_restore(bool flag) {
      if (flag) {
          intr_enable();    // 如果 flag 为真，表示之前中断是启用的，需要恢复中断
      }
  }
  ```
  
  - **恢复中断状态**：`__intr_restore()` 函数检查 `flag` 的值。如果 `flag` 为 1，表示之前中断是启用的，所以调用 `intr_enable()` 来重新启用中断。`intr_enable()` 函数通常是通过修改 CSR 寄存器来设置 `SSTATUS_SIE` 位，恢复中断。
  - 如果 `flag` 为 0，表示中断之前已经是禁用状态，所以不做任何操作。
  
  最终，`local_intr_restore(intr_flag)` 会根据之前保存的状态，恢复中断的启用/禁用状态。
  
---

## 三、关联知识
