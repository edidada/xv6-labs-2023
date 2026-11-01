# RISC-V 调用约定练习

riscv 是基础源码分支，无官方 grade-lab-*。根据 MIT 2023
RISC-V 讲义实现汇编 abisum：计算两倍的 1..n 累加和，嵌套调用
sum_to 时在 16 字节对齐的栈中保存 ra。用户程序检查 n=0..99。
Actions 同时启动 xv6 执行 ABI 用例和原有 usertests -q。

参数和返回值使用 a0，t0 是调用者保存寄存器；嵌套调用会改写 ra，
因此非叶子函数必须保存返回地址。栈向低地址增长。

来源：https://pdos.csail.mit.edu/6.1810/2023/lec/l-riscv.txt
