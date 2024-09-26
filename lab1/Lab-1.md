# **练习一** #
阅读 *kern/init/entry.S*内容代码，结合操作系统内核启动流程，说明指令 *la* *sp*, *bootstacktop* 完成了什么操作，目的是什么？ *tail kern_init* 完成了什么操作，目的是什么？
## ***la sp, bootstacktop* 指令** ##
**操作**

*la* 指令将*bootstacktop*标签所代表的内存地址加载到栈指针寄存器（*sp*）中。

**目的**

此指令的目的是设置内核启动时的初始栈指针。在操作系统内核启动的早期阶段，需要设置一个安全的栈区域，用于存放函数调用、局部变量等。*bootstack*是一个在内存布局中预先定义的区域，用作启动时的栈（*Boot Stack*），而*bootstacktop*是这个栈区域的顶部地址。通过将栈指针设置为*bootstacktop*，内核就准备好了使用这个栈来执行初始化代码，包括后续的加载内核其他部分、初始化硬件设备等操作。

## **tail kern_init 指令** ##
**操作**

在实际的汇编或链接过程中，*kern_init*函数的地址和代码会被放置在*kern entry*:标签之后的某个位置。

**目的**

帮助跳转至内核入口点*kern_init*函数，并随后完成其他初始化工作。

```
void interrupt_handler(struct trapframe *tf) {
    intptr_t cause = (tf->cause << 1) >> 1;
    switch (cause) {
      
        case IRQ_S_TIMER:
            // "All bits besides SSIP and USIP in the sip register are
            // read-only." -- privileged spec1.9.1, 4.1.4, p59
            // In fact, Call sbi_set_timer will clear STIP, or you can clear it
            // directly.
            // cprintf("Supervisor timer interrupt\n");
             /* LAB1 EXERCISE2   YOUR CODE : 2111454 */
            /*(1)设置下次时钟中断- clock_set_next_event()
             *(2)计数器（ticks）加一
             *(3)当计数器加到100的时候，我们会输出一个`100ticks`表示我们触发了100次时钟中断，同时打印次数（num）加一
            * (4)判断打印次数，当打印次数为10时，调用<sbi.h>中的关机函数关机
            */

            clock_set_next_event();// 定义在/kern/driver/clock.c
            if(ticks++ % TICK_NUM == 0 && num < 10)
            {
                num++;
                print_ticks();
            }
            else if(num == 10){
                sbi_shutdown();
            }
            break;
        other cases...
        
        default:
            print_trapframe(tf);
            break;
    }
}

```
