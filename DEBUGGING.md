# 链表调试演示

此分支是课堂 debugging-demo，不是 syscall 作业的起始分支，不能用
遗留的 grade-lab-syscall 作为验收。Actions 运行原有 linked_list_test
四项用例及 usertests -q。

修复了三个问题：空链表插入不能遍历尚未初始化的 next；遍历应沿
next 指针而非 end++；pop 应断开旧节点的 next，不能破坏新表头或
在弹出最后一个节点时解引用空指针。另处理重复创建、销毁和分配失败。

实验背景参考 MIT 2023 课程的调试指导：
https://pdos.csail.mit.edu/6.1810/2023/labs/guidance.html
