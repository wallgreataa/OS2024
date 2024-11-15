# Lab-0.5：

***
## 搭建实验环境

### 使用Vmware安装Ubuntu

* 下载[VmWare](https://www.vmware.com/) 和 [Ubuntu](https://ubuntu.com/download/desktop) 并在VmWare中安装Ubuntu虚拟机

### 路径设置
***
* 为了避免后续配置过程中系统找不到正确的路径执行，因此在终端设置RISCV环境变量。首先打开.bashrc 文件：
```
vim ~/.bashrc
```

* 在.bashrc 文件中添加以下行：
```
export RISCV=/your/path/to/riscv
```
* 输入以下命令使修改生效：
```
source ~/.bashrc
```
### 安装Vim编译器
***
**1. 使用APT包管理器安装Vim**
* Ubuntu 的默认包管理器 APT 提供了Vim，打开终端并输入以下命令来安装Vim：
```
sudo apt update
sudo apt install vim
```
**2. 检查 Vim 是否安装成功**
* 通过以下命令检查 Vim 版本，确认它是否成功安装：
```
vim --version
```
* 如果显示版本信息，说明安装成功。
### 安装配置GCC
***
**1. 使用APT包管理器安装 GCC**
* Ubuntu 的默认包管理器 APT 提供了GCC，打开终端并输入以下命令来安装Vim：
```
sudo apt install gcc
```
**2. 检查安装版本**
* 通过以下命令检查 GCC 版本，确认它是否成功安装：
```
gcc --version
```
**3. 安装构建工具**
* 输入以下命令来安装 build-essential 包（这个包有常用的编译工具）：
```
sudo apt install build-essential
```
**4. 验证 GCC 是否配置成功**
* 通过以下命令检查 GCC 版本：
```
gcc --version
```
**5. 配置编译器路径**
* 设置环境变量的方式指定编译器路径，
首先打开.bashrc 文件：
```
vim ~/.bashrc
```
* 在.bashrc 文件中添加以下行：
```
export CC=/usr/bin/gcc-10
```
* 输入以下命令使修改生效：
```
source ~/.bashrc
```
### 安装配置QEMU
***
**1. 确保安装所需环境存在** 
* 为了避免后续安装报错，先执行以下命令：
```
sudo apt-get install pkg-config
sudo apt-get install libglib2.0-dev
sudo apt-get install libpixman-1-dev
sudo apt-get install libsdl2-2.0
sudo apt-get install libsdl2-dev
sudo apt-get install python2.7-dev
```
**2. 安装QEMU**
* 执行以下命令：
```
wget https://download.qemu.org/qemu-4.1.1.tar.xz
tar xvJf qemu-4.1.1.tar.xz
cd qemu-4.1.1
./configure --target-list=riscv32-softmmu,riscv64-softmmu
make
export PATH=$PWD/riscv32-softmmu:$PWD/riscv64-softmmu:$PATH
```
* 在上述命令执行过程中，由于网络波动导致部分文件或代码数据缺失，可能会导致报错，此时有两个解决方案：
（1）重复执行几次
（2）在物理机中打开[QEMU压缩包](https://download.qemu.org/qemu-4.1.1.tar.xz)并进行下载，之后拖到虚拟机中进行后续操作
**3. 路径添加**
* 在命令行中输入一下命令，设置绝对路径：
```
~/qemu-4.1.1/riscv32-softmmu:~/qemu-4.1.1/riscv64-softmmu:$PATH
```
* 执行以下命令，将文件添加到路径中：
```
vim ~/.bashrc
```
* 将打开的界面拖至最后一行，点击i键进入编辑模式，在最后一行插入下述代码：
```
export PATH=PATH_TO_INSTALL
```
* 插入后点击esc键退出编辑，输入:wq保存并退出，之后输入以下命令使修改生效：
```
source ~/.bashrc
```
* 执行以下命令查看是否配置成功：
```
qemu-system-riscv64 --version
```
### 安装配置GDB
***
**1. 确保安装所需环境存在** 
* 为了避免后续安装报错，先执行以下命令：
```
sudo apt install apt-file
sudo apt-file update
sudo apt-file find libncurses.so.5
sudo apt install libncurses5
```
**2. 安装GDB**
* 执行以下命令：
```
sudo apt-get install libncurses5-dev python2 python2-dev texinfo libreadline-dev
wget https://mirrors.tuna.tsinghua.edu.cn/gnu/gdb/gdb-13.1.tar.xz
tar -xvf gdb-13.1.tar.xz
cd gdb-13.1
mkdir build && cd build
../configure --prefix=/usr/local --target=riscv64-unknown-elf --enable-tui=yes
make
sudo make install
```
* 执行以下命令查看是否配置成功：
```
riscv64-unknown-elf-gdb -v
```
***
# 练习

## 实验报告要求

* 基于markdown格式来完成，以文本方式为主
* 填写各个基本练习中要求完成的报告内容
* 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）
* 列出你认为OS原理中很重要，但在实验中没有对应上的知识点

## 练习1：使用GDB验证启动流程

为了验证GDB的启动流程，我们使用示例代码 Makefile 中的 make debug和make gdb 指令来完成对代码的调试，首先我们先了解了基本的调试代码：

* x/10i 0x80000000 : 显示 0x80000000 处的10条汇编指令。

* x/10i $pc : 显示即将执行的10条汇编指令。

* x/10xw 0x80000000 : 显示 0x80000000 处的10条数据，格式为16进制32bit。

* info register: 显示当前所有寄存器信息。

* info r t0: 显示 t0 寄存器的值。

* break funcname: 在目标函数第一条指令处设置断点。

* break *0x80200000: 在 0x80200000 处设置断点。

* continue: 执行直到碰到断点。

* si: 单步执行一条汇编指令。

### 验证内核能够成功启动

![内核成功启动示意图](./images/1.png)

如上图所示，内核成功运行

### 调试内核启动过程以及调试过程分析

![内核成功开始调试](./images/2.png)

#### 加电后的指令状态

![加电后的指令状态](./images/3.png)

我们可以发现加电后的指令状态仅有`$t0=0x0000000000001000`,即riscv的默认复位地址，也就是从这里开始运行，然后跳转到OpenSBI进行运行。

#### 复位地址开始的10条指令及其功能

通过调试命令：`x/10i 0x0000000000001000`，我们可以访问从`0x0000000000001000`开始的10条指令：

![开始的10条指令](./images/4.png)

| 指令 | 作用 |
| ----- | ----- |
| `0x1000:auipc t0,0x0`| 将当前 PC(0x1000)存入t0 |
| `0x1004:addi a1,t0,32` | 将 t0 加 32，得到 0x1020 并存入 a1 |
| `0x1008:csrr a0,mhartid` | 读取当前硬件线程 ID 并存入 a0 |
| `0x100c:ld t0,24(t0)` | 从 0x1018 内存地址读取 64 位数据，并存入 t0 |
| `0x1010:jr t0` | 跳转到 t0 中存储的地址 |

以上指令就完成了进入OpenSBI的过程。

#### 查看跳转后10条指令的内容及作用

![跳转后10条指令](./images/5.png)

通过调试命令：`x/10i 0x80000000`，我们可以访问从`0x80000000`开始的10条指令：

| 指令 | 作用 |
| ----- | ----- |
| `0x80000000:csrr a6,mhartid`|从控制状态寄存器（CSR）中读取 mhartid并将其值存入寄存器 a6。mhartid 是一个寄存器，存储当前硬件线程（hart）的 ID。 |
| `0x80000004:bgtz a6,0x80000108` | 检查寄存器 a6 的值是否大于零。如果是，则跳转到地址 0x80000108 处。 |
| `0x80000008:auipc t0,0x0` | 将当前的 PC 加上立即数（在这里是 0x0）并存储在寄存器 t0 中。 |
| `0x8000000c:addi t0,t0,1032` | 将 t0 的值加上立即数 1032，并将结果存储在 t0 中。 |
| `0x80000010:auipc t1,0x0` | 将当前 PC 的值加上立即数 0x0，并存储到 t1 中。假设当前 PC 为 0x80000010。 |
| `0x80000014:addi t1,t1,-16` | 将 t1 的值减去 16，然后将结果存储到 t1 中。 |
| `0x80000018:sd t1,0(t0)` | 将寄存器 t1 中的 64 位数据存储到地址 t0 指向的位置。在此例中，它将 t1 的内容存储到 t0 + 0 处。 |
| `0x8000001c:auipc t0,0x0` | 将当前 PC 加上立即数 0x0，并将结果存储到 t0 中。假设当前 PC 为 0x8000001c。 |
| `0x80000020:addi t0,t0,1020` | 将 t0 的值加上 1020，然后将结果存储到 t0 中。 |
| `0x80000024:ld t0,0(t0)` | 从 t0 指向的地址处读取 64 位数据，并将其存储到寄存器 t0 中。在此例中，它从 0x80000400 处读取一个双字。 |

# 本实验中重要的知识点及理解

## RISC-V硬件加电后的基本执行过程的理解

* 进入复位地址：在理论知识中RISC-V加电后一般会进入的复位地址是：`0x1000`,而在实验中PC指向的第一个地址就是`0x1000`，在实验中也是验证了理论的正确性；
* 执行初始化代码部分：cpu会开始执行位于复位地址处的初始化代码。这些代码通常包括处理器的基本设置，例如设置堆栈指针、设置寄存器、禁用或启用中断等等。
* 进入OpenSBI并开始执行：在`0x1000`地址往下执行几行后，进入了`0x80000000`,即将内核镜像预先加载到 Qemu 物理内存以地址 `0x80200000` 开头的区域上，在实验中也确实验证了这一点。
* 加载和执行应用程序： 一旦初始化完成，处理器会加载并执行应用程序代码。这通常涉及到从存储设备中加载应用程序二进制文件到内存中，并跳转到应用程序的入口点。





