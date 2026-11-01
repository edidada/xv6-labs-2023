# xv6 2023 实验分支导航

所有实验均从同名原始分支独立实现，解答分支统一为 `edidada/<原分支名>`。
编译与 QEMU 验证全部在 GitHub Actions 执行；本地只做代码编辑和静态检查。

| 分支 | 实现 | 官方要求 |
| --- | --- | --- |
| [edidada/util](https://github.com/edidada/xv6-labs-2023/tree/edidada/util) | sleep/pingpong/primes/find/xargs | [MIT util](https://pdos.csail.mit.edu/6.1810/2023/labs/util.html) |
| [edidada/syscall](https://github.com/edidada/xv6-labs-2023/tree/edidada/syscall) | trace/sysinfo 与 GDB 故障观察 | [MIT syscall](https://pdos.csail.mit.edu/6.1810/2023/labs/syscall.html) |
| [edidada/pgtbl](https://github.com/edidada/xv6-labs-2023/tree/edidada/pgtbl) | 共享 PID 页、vmprint、pgaccess | [MIT pgtbl](https://pdos.csail.mit.edu/6.1810/2023/labs/pgtbl.html) |
| [edidada/traps](https://github.com/edidada/xv6-labs-2023/tree/edidada/traps) | 内核回溯和用户 alarm | [MIT traps](https://pdos.csail.mit.edu/6.1810/2023/labs/traps.html) |
| [edidada/cow](https://github.com/edidada/xv6-labs-2023/tree/edidada/cow) | 引用计数和写时复制 fork | [MIT cow](https://pdos.csail.mit.edu/6.1810/2023/labs/cow.html) |
| [edidada/thread](https://github.com/edidada/xv6-labs-2023/tree/edidada/thread) | 用户线程、并行哈希表、barrier | [MIT thread](https://pdos.csail.mit.edu/6.1810/2023/labs/thread.html) |
| [edidada/lock](https://github.com/edidada/xv6-labs-2023/tree/edidada/lock) | 每 CPU 分配器及分桶缓冲缓存 | [MIT lock](https://pdos.csail.mit.edu/6.1810/2023/labs/lock.html) |
| [edidada/fs](https://github.com/edidada/xv6-labs-2023/tree/edidada/fs) | 双重间接块和符号链接 | [MIT fs](https://pdos.csail.mit.edu/6.1810/2023/labs/fs.html) |
| [edidada/mmap](https://github.com/edidada/xv6-labs-2023/tree/edidada/mmap) | 按需文件映射与 fork/exit/unmap | [MIT mmap](https://pdos.csail.mit.edu/6.1810/2023/labs/mmap.html) |
| [edidada/net](https://github.com/edidada/xv6-labs-2023/tree/edidada/net) | E1000 TX/RX 环 | [MIT net](https://pdos.csail.mit.edu/6.1810/2023/labs/net.html) |

课堂演示分支：

- `edidada/riscv`：嵌套汇编调用约定练习，远程 ABI 测试和 usertests。
- `edidada/debugging-demo`：修复链表演示的插入、遍历和弹出错误，运行原有四项链表测试和 usertests。

## 查看远程证据

```powershell
gh run list --branch edidada/util
gh run view <run-id> --log
gh run download <run-id> --name lab-results --dir <output-dir>
```

每个分支提供 `.github/workflows/lab.yml`，产物包含 `.ci-results/grade.log`
及 QEMU 输出。syscall 另保存 GDB 日志，traps 保存 call.asm，net 保存 pcap。
评分脚本沿用原始课程文件；两个无独立官方作业的演示分支使用单独回归脚本。
官方必做实验为上述十项；网页标为 optional 的挑战项目不纳入必做验收。

各实验的问答保存在对应分支的 answers-*.txt；具体汇编地址以 Actions 产物为准。
