# **练习一** #
阅读 kern/init/entry.S内容代码，结合操作系统内核启动流程，说明指令 la sp, bootstacktop 完成了什么操作，目的是什么？ tail kern_init 完成了什么操作，目的是什么？
## **la sp, bootstacktop 指令** ##
操作：la 指令将bootstacktop标签所代表的内存地址加载到栈指针寄存器（sp）中。

目的：此指令的目的是设置内核启动时的初始栈指针。在操作系统内核启动的早期阶段，需要设置一个安全的栈区域，用于存放函数调用、局部变量等。bootstack是一个在内存布局中预先定义的区域，用作启动时的栈（Boot Stack），而bootstacktop是这个栈区域的顶部地址。通过将栈指针设置为bootstacktop，内核就准备好了使用这个栈来执行初始化代码，包括后续的加载内核其他部分、初始化硬件设备等操作。

## **tail kern_init 指令** ##
操作：在实际的汇编或链接过程中，kern_init函数的地址和代码会被放置在kern entry:标签之后的某个位置。

目的：帮助跳转至内核入口点kern_init()函数，并随后完成其他初始化工作。
