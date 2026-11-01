"""Capture the lab's GDB observations on the Actions runner only."""
import pathlib
import subprocess


def observe(label, commands):
    with open(f"debug-{label}-qemu.log", "w") as output:
        qemu = subprocess.Popen(
            ["make", "qemu-gdb", "CPUS=1", "GDBPORT=26000"],
            stdin=subprocess.PIPE, stdout=output, stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        try:
            args = ["gdb-multiarch", "-q", "-nx", "-batch", "kernel/kernel"]
            for command in [
                "set pagination off", "set confirm off",
                "set tcp auto-retry on", "set tcp connect-timeout 30",
                "target remote localhost:26000",
            ] + commands:
                args.extend(["-ex", command])
            result = subprocess.run(args, text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=90)
            pathlib.Path(f"debug-{label}.log").write_text(result.stdout)
            print(result.stdout)
            if result.returncode:
                raise RuntimeError(f"GDB {label} failed")
        finally:
            # Kill the whole make/QEMU group, including when GDB times out.
            import os
            import signal
            os.killpg(qemu.pid, signal.SIGTERM)
            qemu.wait(timeout=10)


observe("normal", [
    "break syscall", "continue", "backtrace",
    "p /x cpus[0].proc->trapframe->a7", "p /x $sstatus",
    "p cpus[0].proc->name", "p cpus[0].proc->pid", "detach",
])

source = pathlib.Path("kernel/syscall.c")
original = source.read_text()
try:
    source.write_text(original.replace("num = p->trapframe->a7;",
                                       "num = *(int *)0;", 1))
    subprocess.run(["make", "kernel/kernel"], check=True)
    observe("fault", [
        "break kerneltrap", "continue", "backtrace",
        "p /x $scause", "p /x $stval", "p /x $sepc", "x/i $sepc",
        "p cpus[0].proc->name", "p cpus[0].proc->pid", "detach",
    ])
finally:
    source.write_text(original)
