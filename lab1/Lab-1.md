# **练习一** 
阅读 *kern/init/entry.S*内容代码，结合操作系统内核启动流程，说明指令 *la* *sp*, *bootstacktop* 完成了什么操作，目的是什么？ *tail kern_init* 完成了什么操作，目的是什么？
## ***la sp, bootstacktop* 指令** 
**操作**

*la* 指令将*bootstacktop*标签所代表的内存地址加载到栈指针寄存器（*sp*）中。

**目的**

此指令的目的是设置内核启动时的初始栈指针。在操作系统内核启动的早期阶段，需要设置一个安全的栈区域，用于存放函数调用、局部变量等。*bootstack*是一个在内存布局中预先定义的区域，用作启动时的栈（*Boot Stack*），而*bootstacktop*是这个栈区域的顶部地址。通过将栈指针设置为*bootstacktop*，内核就准备好了使用这个栈来执行初始化代码，包括后续的加载内核其他部分、初始化硬件设备等操作。

## **tail kern_init 指令**
**操作**

在实际的汇编或链接过程中，*kern_init*函数的地址和代码会被放置在*kern entry*:标签之后的某个位置。

**目的**

帮助跳转至内核入口点*kern_init*函数，并随后完成其他初始化工作。

# **练习二** 

请编程完善**trap.c**中的中断处理函数**trap**，在对时钟中断进行处理的部分填写**kern/trap/trap.c**函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用**print_ticks**子程序，向屏幕上打印一行文字“100 ticks”，在打印完10行后调用**sbi.h**中的**shut_down()**函数关机。

**部分代码**
```
void interrupt_handler(struct trapframe *tf) {
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
在**case: IRQ_S_TIMER**中，通过每次发生时钟中断是设置下一次的时钟中断周期，完成定时的终端任务实现。当计数器完成十次计数是，则调用**<sbi.h>**中的关机函数终止程序进行。

# **扩展练习 Challenge1** 
描述**ucore**中处理中断异常的流程（从异常的产生开始），其中**mov** **a0**，**sp**的目的是什么？**SAVE_ALL**中寄寄存器保存在栈中的位置是什么确定的？对于任何中断，**__alltraps** 中都需要保存所有寄存器吗？请说明理由。
处理中断异常的流程：CPU在捕获异常后产生某种类型的中断（Exception、Trap、Interrupt），以练习二中产生Interrupt类型的中断为例，CPU跳转到到stvec（中断向量表基址）。中断向量表的把不同种类的中断映射到对应的中断处理程序，进而进入中断处理程序的入口点。进入中断处理程序后，我们还需把原先的寄存器保存下来，用来中断处理完毕后恢复到原状态，这些寄存器也被叫做CPU的context(上下文，情境)。保存与恢复上下文通过汇编宏SAVE_ALL、RESTORE_ALL指令将上下文包装为结构体，切换前的上下文作为一个结构体，传递给trap()作为函数参数。kern/trap/trap.c按照中断类型进行分发(此处为interrupt_handler()分支)并对应的处理语句。在完成处理后返回kern/trap/trapentry.S并恢复原先的上下文，中断处理结束。



# **扩增练习 Challenge2** 
在**trapentry.S**中汇编代码 **csrw sscratch**, **sp**；**csrrw s0**, **sscratch**, **x0**实现了什么操作，目的是什么？**save all**里面保存了**stval scause**这些**csr**，而在**restore all**里面却不还原它们？那这样**store**的意义何在呢？


# **扩展练习Challenge3：完善异常中断** 
编程完善在触发一条非法指令异常 **mret**和，在 **kern/trap/trap.c**的异常处理函数中捕获，并对其进行处理，简单输出异常类型和异常指令触发地址，即**Illegal instruction caught at 0x(地址)**，**ebreak caught at 0x（地址）**与**Exception type:Illegal instruction**，**Exception type: breakpoint**。

