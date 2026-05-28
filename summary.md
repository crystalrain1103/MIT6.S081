# MIT6.S081 Operating System Summary(2021)

author: github@crystalrain

date: 2026/05/28

[TOC]

## Prologue

本课程围绕教学操作系统xv6展开，内容上实现、修改、调试一个完整操作系统。xv6是一个基于Unix V6思想、运行在RISC-V架构上的简化版Unix-like OS。整个课程围绕下面核心概念展开：

- 抽象硬件（Abstraction）
- 分时复用（Multiplex）
- 隔离与共享（Isolation and Sharing）
- 高性能（Performance）

课程分为5条主线：

1. **操作系统的本质**：OS的本质是资源管理器，是对底层硬件的抽象。
2. **用户与内核的交互**：通过`syscall`, `trap`, `interrupt`来实现用户态到内核的合法切换，保证非法访问的正确处理。
3. **虚拟内存**：通过`page table`建立从虚拟内存`va`到物理内存`pa`的映射，是实现`lazy allocation`, `copy-on-write`, `mmap`等功能的核心。
4. **并发与多核**：包括多进程，多线程及其调度`shedule`, `context switch`，共享带来的`race condition`及其解决方法`lock`。
5. **文件系统**：包括`inode`, `log`等概念，文件系统并非必须，它更类似于一种协议，用于保证读写的正确性，非易失性和效率。



## Lectures

### L1 Intro

关键词：

- Unix 哲学
- shell
- fork/exec/wait
- pipe

核心思想：**Everything is a process**

### L3 System calls

关键词：

- user mode / kernel mode
- syscall
- trap
- context switch

核心思想：**系统调用本质是特权切换**。用户程序不能直接访问硬件，只能trap 进入内核->保存上下文->执行 kernel code->恢复现场

### L4 Page Tables

关键词：

- VA → PA translation
- multi-level page table
- PTE
- TLB

核心思想：**虚拟内存是地址空间抽象**

### L6 Isolation

关键词：

- trampoline
- trapframe
- privilege level

核心思想：**真正实现隔离的是硬件**

### L8 Page faults

关键词：

- page fault
- lazy allocation
- demand paging
- copy-on-write
- mmap
- protection fault

核心思想：**利用Page Fault节省物理内存**

### L9 Interrupts

关键词：

- trap
- interrupt handler
- concurrency
- interrupt controller
- polling vs interrupt

核心思想：**中断让 CPU 能够异步响应外部事件，当外部事件发生频率较低时中断比轮询更能节省资源**

### L10 Multiprocessors & Locking

关键词：

- spinlock
- atomic instruction
- race condition

核心思想：**锁保护的是不变量（保证并发正确性）**

### L11 Thread switching

关键词：

- scheduler
- context switch
- yield

核心思想：**并发很多时候只是快速切换**，OS使用`timer interrupt`和`scheduler`制造“同时运行”的幻觉。

### L13 Sleep & Wakeup

关键词：

- condition synchronization
- lost wakeup

核心思想：**并发bug本质是时序问题（保证并发正确性）**

### L14-16 File System

关键词：

- inode
- directory
- logging
- buffer cache

核心思想：**文件系统本质是树 + block**，**logging 是 crash consistency 的核心**

### L17 Virtual memory for applications

关键词：

- mmap
- shared virtual memory
- garbage collection（GC）

核心思想：**虚拟内存不仅可以用于内核的隔离，也能提供应用层的抽象**

### L18 OS organization

关键词：

- Monolithic Kernel（宏内核）
- Microkernel（微内核）
- IPC
- kernel performance tradeoff

核心思想：**不同内核组织之间的trade-off**

| 结构        | Kernel 大小 | 模块位置      | 通信方式      | 性能 | 隔离性 |
| ----------- | ----------- | ------------- | ------------- | ---- | ------ |
| Monolithic  | 大          | kernel 内     | function call | 很高 | 差     |
| Layered     | 中等        | 分层          | layer call    | 中   | 中     |
| Microkernel | 极小        | user space    | IPC           | 较低 | 很强   |
| Modules     | 大          | kernel module | function call | 高   | 较差   |
| Hybrid      | 中等偏大    | 混合          | IPC + call    | 较高 | 较强   |

### L19 Virtual Machines

关键词：

- Virtual Machine（虚拟机）
- Hypervisor（虚拟机监控器）
- Trap-and-Emulate
- Full Virtualization
- VM isolation

核心思想：**虚拟机本质上是对整个硬件系统的虚拟化**

### L20 Kernels and HLL

关键词：

- High-Level Language（HLL）
- memory safety
- kernel bugs
- garbage collection（GC）

核心思想：**C语言与高级语言在编写操作系统上的trade-off**

### L21 Networking

关键词：

- packet
- Ethernet
- IP
- TCP
- UDP
- socket

核心思想：**网络本质上是分层的数据传输系统，提供抽象与封装**

### L22 Meltdown

关键词：

- Meltdown
- speculative execution
- cache side channel
- privilege isolation
- KPTI（Kernel Page Table Isolation）

核心思想：**现代 CPU 的性能优化可能破坏安全隔离，硬件也可能违反 OS 的安全假设**

Meltdown攻击主要是利用的CPU的**预测执行**和**缓存读取时间与主存差别**，判断某个本来没有权限访问的数据是否被预测加载入cache中，进而窃取数据。该攻击仅适用于内核内存被映射到用户态空间的操作系统，而像xv6这样用户态通过trampoline跳转到内核的操作系统，用户态中仅保留了trap frame page的映射，因而不会被窃取到内核的数据（因为根本没有办法在用户态读取到内核的数据）。

### L23 RCU

关键词：

- RCU（Read-Copy Update）
- lock-free read
- scalability
- concurrent data structure

核心思想：**在读远多于写的场景下，读操作不应该被锁阻塞**



## Labs

### Lab util

- 实现功能：`sleep`, `pingpong`, `primes`, `find`, `xargs`

- 理解概念：`fork`, `pipe`, `exec`, `fd`

### Lab syscall

- 实现功能：`trace`, `sysinfo`
- 理解概念：
  - 从用户态到内核的过程
  - 系统调用分发
  - 系统调用的参数传递
  - trap处理

### Lab pgtbl

- 实现功能：`speed up system calls`, `page table print`, `detecting pages accessed`
- 理解概念：
  - 页表结构与地址转换
  - PTE、VA、PA 

### Lab traps

- 实现功能：`back trace`, `alarm`
- 理解概念：
  - trapframe
  - 时钟中断
  - 用户态恢复

### Lab lazy

- 实现功能：`lazy allocation`
- 理解概念：
  - 按需分配内存
  - 实现虚拟内存的优势

### Lab COW

- 实现功能：`copy-on-write`
- 理解概念：
  - 优化fork函数，共享页面

### Lab thread

- 实现功能：`context switching`, `using thread`, `barrier`
- 理解概念：
  - 线程与进程的区别
  - 线程切换与调度

### Lab lock

- 实现功能：`mem allocator`, `buffer cache`
- 理解概念：
  - 加锁与性能之间的trade off

### Lab fs

- 实现功能：`large files`, `symbolic links`
- 理解概念：
  - metadata结构
  - inode


### Lab mmap

- 实现功能：`mmap`, `munmap`
- 理解概念：
  - 将文件IO变为直接内存访问，减小系统调用开销


### Lab net

- 实现功能：`e1000_transmit`, `e1000_receive`
- 理解概念：
  - 什么是设备驱动
  - Ethernet、ARP、UDP等传输协议