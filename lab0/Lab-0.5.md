# Lab 0.5
<<<<<<< HEAD
***
## 搭建实验环境
***
### 使用Vmware安装Ubuntu
***
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

## 练习
***
### 实验报告要求
***
* 基于markdown格式来完成，以文本方式为主
* 填写各个基本练习中要求完成的报告内容
* 列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）
* 列出你认为OS原理中很重要，但在实验中没有对应上的知识点
### 练习1: 使用GDB验证启动流程
为了熟悉使用qemu和gdb进行调试工作,使用gdb调试QEMU模拟的RISC-V计算机加电开始运行到执行应用程序的第一条指令（即跳转到0x80200000）这个阶段的执行过程，说明RISC-V硬件加电后的几条指令在哪里？完成了哪些功能？要求在报告中简要写出练习过程和回答。
=======

## 实验报告要求：
基于markdown格式来完成，以文本方式为主
填写各个基本练习中要求完成的报告内容
列出你认为本实验中重要的知识点，以及与对应的OS原理中的知识点，并简要说明你对二者的含义，关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）
列出你认为OS原理中很重要，但在实验中没有对应上的知识点

### 验证GDB启动流程


>>>>>>> 4a901fcd321a1e4e6f03593475fd0cb0bd0c7d74
