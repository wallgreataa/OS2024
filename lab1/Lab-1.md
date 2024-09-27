# **练习一** 
阅读 `kern/init/entry.S`内容代码，结合操作系统内核启动流程，说明指令 `la` `sp`, `bootstacktop` 完成了什么操作，目的是什么？ `tail kern_init` 完成了什么操作，目的是什么？
## **`la sp, bootstacktop` 指令** 
- **操作**

 `la` 指令将`bootstacktop`标签所代表的内存地址加载到栈指针寄存器（`sp`）中。

- **目的**

 此指令的目的是设置内核启动时的初始栈指针。在操作系统内核启动的早期阶段，需要设置一个安全的栈区域，用于存放函数调用、局部变量等。`bootstack`是一个在内存布局中预先定义的区域，用作启动时的栈（`Boot Stack`），而`bootstacktop`是这个栈区域的顶部地址。通过将栈指针设置为`bootstacktop`，内核就准备好了使用这个栈来执行初始化代码，包括后续的加载内核其他部分、初始化硬件设备等操作。

## **tail kern_init 指令**
- **操作**

 在实际的汇编或链接过程中，`kern_init`函数的地址和代码会被放置在`kern entry`:标签之后的某个位置。

- **目的**

 帮助跳转至内核入口点`kern_init`函数，并随后完成其他初始化工作。

# **练习二** 

请编程完善`trap.c`中的中断处理函数`trap`，在对时钟中断进行处理的部分填写`kern/trap/trap.c`函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用`print_ticks`子程序，向屏幕上打印一行文字“100 ticks”，在打印完10行后调用`sbi.h`中的`shut_down()`函数关机。

**部分代码**

```c
void interrupt_handler(struct trapframe `tf) {
    intptr_t cause = (tf->cause << 1) >> 1;
    switch (cause) {  
        case IRQ_S_TIMER:
            clock_set_next_event();
            //设置下次时钟中断- clock_set_next_event() 定义在/kern/driver/clock.c

            if(ticks++ % TICK_NUM == 0 && num < 10)
            {
                num++;
                //计数器（ticks）加一

                print_ticks();
                //当计数器加到100的时候，输出`100ticks`表示触发100次时钟中断，同时打印次数（num）加一
            }
            else if(num == 10){
                sbi_shutdown();
                //判断打印次数，当打印次数为10时，调用<sbi.h>中的关机函数关机

            }
            break;
        other cases...

        default:
            print_trapframe(tf);
            break;
    }
}

```
在`case: IRQ_S_TIMER`中，通过每次发生时钟中断是设置下一次的时钟中断周期，完成定时的终端任务实现。当计数器完成十次计数是，则调用`<sbi.h>`中的关机函数终止程序进行。

# **扩展练习 Challenge1** 
描述`ucore`中处理中断异常的流程（从异常的产生开始），其中`mov` `a0`，`sp`的目的是什么？`SAVE_ALL`中寄寄存器保存在栈中的位置是什么确定的？对于任何中断，`__alltraps` 中都需要保存所有寄存器吗？请说明理由。

### Challenge 1：描述与理解中断流程

在 `ucore` 中，处理中断异常的流程如下：

1. **异常类型**：当 CPU 捕获异常时，会产生不同类型的中断，包括 `Exception`、`Trap` 和 `Interrupt`。以 `Interrupt` 为例，CPU 会跳转到中断向量表的基地址 `stvec`。

2. **中断向量表**：中断向量表将不同种类的中断映射到对应的中断处理程序。根据中断的类型，CPU 找到对应的处理程序入口点。

3. **进入中断处理程序**：进入中断处理程序后，需要保存原先的寄存器状态（上下文），以便中断处理结束后能够恢复到原来的状态。这些寄存器统称为 CPU 的上下文（context）。

4. **上下文保存与恢复**：上下文的保存与恢复通过汇编宏 `SAVE_ALL` 和 `RESTORE_ALL` 实现。这些宏将上下文包装为结构体，切换前的上下文作为参数传递给 `trap()` 函数。

5. **处理中断**：在 `kern/trap/trap.c` 中，按照中断类型进行分发，进入相应的处理函数，例如 `interrupt_handler()`，并执行对应的处理逻辑。

6. **恢复上下文**：在完成中断处理后，返回到 `kern/trap/trapentry.S` 并恢复之前保存的上下文，以确保被打断的程序能够无缝继续执行。

### 其他问题

- **mov a0, sp 的目的是什么？**  
  这条指令将当前栈指针 `sp` 的值保存到寄存器 `a0` 中，为后续处理中提供当前栈的地址，以便正确管理栈中的数据。

- **SAVE_ALL 中寄存器保存在栈中的位置是什么确定的？**  
  保存的位置由当前栈指针 `sp` 决定，通过栈的操作将寄存器的值压入栈中，确保上下文能够正确恢复。

- **对于任何中断，__alltraps 中都需要保存所有寄存器吗？请说明理由。**  
  是的，保存所有寄存器是必要的，因为中断可能在任意时刻发生，所有寄存器的状态需要被保存，以确保能够完整恢复执行环境。不同的中断处理可能会使用不同的寄存器，保存所有寄存器可以避免数据丢失或覆盖。

---


# **扩展练习 Challenge2** 
在`trapentry.S`中汇编代码 `csrw sscratch`, `sp`；`csrrw s0`, `sscratch`, `x0`实现了什么操作，目的是什么？`save all`里面保存了`stval scause`这些`csr`，而在`restore all`里面却不还原它们？那这样`store`的意义何在呢？
### Challenge 2：理解上下文切换机制

1. **汇编代码**：
    `csrw sscratch, sp`：这条指令将当前栈指针 `sp` 的值写入 CSR 寄存器 `sscratch`，用于临时存储。目的是为了在后续处理中能够访问到当前栈指针的值。
    `csrrw s0, sscratch, x0`：这条指令从 `sscratch` 中读取值并写入到寄存器 `s0`，同时将 `x0`（即零寄存器）写入 `sscratch`。目的是将当前栈指针的值转存到 `s0` 中，便于后续使用。

2. **保存与恢复 CSR**：
    在 `SAVE_ALL` 中保存 `stval` 和 `scause` 这些 CSR 寄存器的值，是为了在处理中断时保留与中断相关的状态信息。
    在 `restore_all` 中不恢复它们的原因是因为这些寄存器的值在处理中断时并不需要在恢复上下文时重置。`stval` 和 `scause` 用于指示异常的原因，有时甚至可以将异常相关信息记录在`log`日志中。

---


# **扩展练习Challenge3：完善异常中断** 
编程完善在触发一条非法指令异常 `mret`和，在 `kern/trap/trap.c`的异常处理函数中捕获，并对其进行处理，简单输出异常类型和异常指令触发地址，即`Illegal instruction caught at 0x(地址)`，`ebreak caught at 0x（地址）`与`Exception type:Illegal instruction`，`Exception type: breakpoint`。


### Challenge 3：完善异常中断

在 `kern/trap/trap.c` 中的异常处理函数中，可以添加如下代码来处理非法指令异常：

```c
void exception_handler(struct trapframe *tf) {
    switch (tf->cause) {
       
        case CAUSE_ILLEGAL_INSTRUCTION:

            cprintf("Exception Type:Illegal Instruction\n");

            // tf->epc寄存器保存了触发中断的指令的地址
            // 因此输出该寄存器的值就是中断指令的地址
            // %08x的含义：08表示输出8个字符，x是输出16进制
            cprintf("Illegal Instruction caught at 0x%08x\n", tf->epc);
            
            // 记录下一条指令,内部地址偏移+4

            tf->epc += 4;

            break;
        case CAUSE_BREAKPOINT:

            
            cprintf("Exception Type:Breakpoint\n");

            // tf->epc寄存器保存了触发中断的指令的地址
            // 因此输出该寄存器的值就是中断指令的地址
            // %08x的含义：08表示输出8个字符，x是输出16进制
            cprintf("Breakpoint caught at 0x%08x\n", tf->epc);
            
            // 记录下一条指令,内部地址偏移+2
            
            tf->epc += 2;
            
            break;
       case other...
    }
}

```

该 `exception_handler` 函数用于处理异常，包括非法指令和断点异常。首先，它根据 `tf->cause` 判断异常类型。如果是非法指令异常，则输出异常类型和触发地址，并将 `tf->epc` 加 4，以跳过当前的 4 字节指令。如果是断点异常，则输出相应的信息，并将 `tf->epc` 加 2，以跳过当前的 2 字节指令。该函数还可以扩展以处理其他类型的异常。
