## libfault - tiny crash reporting library

[![MIT](http://b.repl.ca/v1/license-BSD3-blue.png)](http://en.wikipedia.org/wiki/BSD_licenses)
[![C](http://b.repl.ca/v1/language-C-yellow.png)](http://en.wikipedia.org/wiki/C_(programming_language))

`libfault` is a tiny C library that helps you track down crashes by
providing robust, simple crash reporting with an easy-to-use API. It's
originally based on code from [Phusion Passenger][phusion], with some
extensions.

[phusion]: http://www.reddit.com/r/programming/comments/13vmik/redis_crashes_a_small_rant_about_software/c783lzx

It does things like:

  - Ensures all code is async signal-safe.

  - Catches SIGSEGV, SIGABRT, SIGILL, SIGBUS, SIGFPE.

  - Runs the signal handler in a separate, pre-allocated stack using
    `sigaltstack()`, just in case the crash occurs because you went
    over stack boundaries.

  - Reports time and PID of the crashing process.

  - Forks off a child process for gathering most crash report
    information. This is because not all operating systems allow
    signal handlers to do a lot of stuff, even if your code is async
    signal safe. For example if you try to `waitpid()` in a SIGSEGV
    handler on OS X, the kernel just terminates your process.

  - Calls `fork()` on Linux directly using `syscall()` because the
    glibc `fork()` wrapper tries to grab the `ptmalloc2` lock. This
    will deadlock if it was the memory allocator that crashed.

  - Prints a backtrace upon crash, using `backtrace_symbols_fd()`. We
    explicitly do not use `backtrace()` because the latter may
    `malloc()` memory, and that is not async signal safe (it could be
    memory allocator crashing for all you know!)

  - Pipes the output of `backtrace_symbols_fd()` to an external script
    that demangels C++ symbols into sane, readable symbols.

  - Works around OS X-specific signal-threading quirks.

  - Optionally invokes a beep. Useful in developer mode for grabbing
    the developer's attention.

  - Optionally dumps the entire crash report to a file *in addition*
    to writing to stderr.

  - Dumps the process memory map.

  - Dumps the registers and stack of the thread that triggered the
    fault.

  - Full IA32/AMD64/ARM support.

  - Hooks `assert()` handlers to report assert information in the logs.

  - Gathers program-specific debugging information, e.g. runtime
    state. You can supply a custom callback to do this.

  - Places a time limit on the crash report gathering code. Because
    the gathering code may allocate memory or doing other async signal
    unsafe stuff you never know whether it will crash or deadlock. We
    give it a few seconds at most to gather information.

  - Allows you to specify a URL/email address for reports to be sent
    to for users.

Here's what a crash report looks like, using the `t/test3.c` source example:

```
$ ./bin/libfault t/test3
f(): presumably LD_PRELOADed
g(): gonna crash soon
h(): crashing
Assertion failed! t/test3.c:11: h: 0
[ pid=21141, timestamp=1418366467 ] Process aborted! signo=SIGABRT(6), reason=SI_TKILL, signal sent by PID 21141 with UID 1000, si_addr=0x000003e800005295
[ pid=21141 ] Crash log dumped to /tmp/exe-crash.libfault.1418366467
--------------------------------------
Fri Dec 12 00:41:07 CST 2014
Linux 3.13.0-34-generic #60-Ubuntu SMP Wed Aug 13 15:45:27 UTC 2014 x86_64 x86_64
time(seconds)        unlimited
file(blocks)         unlimited
data(kbytes)         unlimited
stack(kbytes)        8192
coredump(blocks)     0
memory(kbytes)       unlimited
locked memory(kbytes) 64
process              127208
nofiles              1024
vmemory(kbytes)      unlimited
locks                unlimited
--------------------------------------
[ pid=21141 ] Last assertion failure: (0), function h, file t/test3.c, line 11.
--------------------------------------
[ pid=21141 ] Register dump
RAX:0x0000000000000000 RBX:0x00007f40bc897868
RCX:0xffffffffffffffff RDX:0x0000000000000006
RDI:0x0000000000005295 RSI:0x0000000000005295
RBP:0x0000000000000000 RSP:0x00007fff73cf5798
R8 :0x00007f40bcc96740 R9 :0x0000000000400a54
R10:0x0000000000000008 R11:0x0000000000000246
R12:0x000000000040080b R13:0x00007fff73cf5a00
R14:0x0000000000000000 R15:0x0000000000000000
RIP:0x00007f40bc50dbb9 EFL:0x0000000000000246
CGF:0x0000000000000033
--------------------------------------
[ pid=21141 ] Stack dump (16 words)
(0x00007fff73cf5810) -> (0x0000000000000000)
(0x00007fff73cf5808) -> (0x0000000000000000)
(0x00007fff73cf5800) -> (0x0000000000000000)
(0x00007fff73cf57f8) -> (0x0000000000000000)
(0x00007fff73cf57f0) -> (0x0000000000000000)
(0x00007fff73cf57e8) -> (0x0000000000000000)
(0x00007fff73cf57e0) -> (0x0000000000000000)
(0x00007fff73cf57d8) -> (0x0000000000000000)
(0x00007fff73cf57d0) -> (0x0000000000000000)
(0x00007fff73cf57c8) -> (0x0000000000000000)
(0x00007fff73cf57c0) -> (0x0000000000000000)
(0x00007fff73cf57b8) -> (0x0000000000000000)
(0x00007fff73cf57b0) -> (0x0000000000000000)
(0x00007fff73cf57a8) -> (0x0000000000000000)
(0x00007fff73cf57a0) -> (0x0000000000000020)
(0x00007fff73cf5798) -> (0x00007f40bc510fc8)
--------------------------------------
[ pid=21141 ] libc backtrace available!
[ pid=21141 ] Backtrace with 13 frames:
lib/libfaultpreload.so(+0x2988)[0x7f40bc89f988]
lib/libfaultpreload.so(+0x14c7)[0x7f40bc89e4c7]
lib/libfaultpreload.so(+0x3678)[0x7f40bc8a0678]
/lib/x86_64-linux-gnu/libc.so.6(+0x36c30)[0x7f40bc50dc30]
/lib/x86_64-linux-gnu/libc.so.6(gsignal+0x39)[0x7f40bc50dbb9]
/lib/x86_64-linux-gnu/libc.so.6(abort+0x148)[0x7f40bc510fc8]
lib/libfaultpreload.so(+0x3bd7)[0x7f40bc8a0bd7]
t/test3[0x40093a]
t/test3[0x40095c]
t/test3[0x40097c]
t/test3(_start+0x0)[0x40080b]
/lib/x86_64-linux-gnu/libc.so.6(__libc_start_main+0xf5)[0x7f40bc4f8ec5]
t/test3[0x400834]
--------------------------------------
[ pid=21141 ] Memory mappings:
00400000-00401000 r-xp 00000000 fc:01 269296                             /home/a/code/libfault/t/test3
00600000-00601000 r--p 00000000 fc:01 269296                             /home/a/code/libfault/t/test3
00601000-00602000 rw-p 00001000 fc:01 269296                             /home/a/code/libfault/t/test3
015a8000-015c9000 rw-p 00000000 00:00 0                                  [heap]
7f40bc4d7000-7f40bc692000 r-xp 00000000 fc:01 1573179                    /lib/x86_64-linux-gnu/libc-2.19.so
7f40bc692000-7f40bc892000 ---p 001bb000 fc:01 1573179                    /lib/x86_64-linux-gnu/libc-2.19.so
7f40bc892000-7f40bc896000 r--p 001bb000 fc:01 1573179                    /lib/x86_64-linux-gnu/libc-2.19.so
7f40bc896000-7f40bc898000 rw-p 001bf000 fc:01 1573179                    /lib/x86_64-linux-gnu/libc-2.19.so
7f40bc898000-7f40bc89d000 rw-p 00000000 00:00 0
7f40bc89d000-7f40bc8a3000 r-xp 00000000 fc:01 269282                     /home/a/code/libfault/lib/libfaultpreload.so
7f40bc8a3000-7f40bcaa2000 ---p 00006000 fc:01 269282                     /home/a/code/libfault/lib/libfaultpreload.so
7f40bcaa2000-7f40bcaa3000 r--p 00005000 fc:01 269282                     /home/a/code/libfault/lib/libfaultpreload.so
7f40bcaa3000-7f40bcaa4000 rw-p 00006000 fc:01 269282                     /home/a/code/libfault/lib/libfaultpreload.so
7f40bcaa4000-7f40bcac7000 r-xp 00000000 fc:01 1573173                    /lib/x86_64-linux-gnu/ld-2.19.so
7f40bcc96000-7f40bcc99000 rw-p 00000000 00:00 0
7f40bcca2000-7f40bccc6000 rw-p 00000000 00:00 0                          [stack:21141]
7f40bccc6000-7f40bccc7000 r--p 00022000 fc:01 1573173                    /lib/x86_64-linux-gnu/ld-2.19.so
7f40bccc7000-7f40bccc8000 rw-p 00023000 fc:01 1573173                    /lib/x86_64-linux-gnu/ld-2.19.so
7f40bccc8000-7f40bccc9000 rw-p 00000000 00:00 0
7fff73cd7000-7fff73cf8000 rw-p 00000000 00:00 0
7fff73d72000-7fff73d74000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]
--------------------------------------
[ pid=21141 ] Open files and file descriptors:
lsof: WARNING: can't stat() ext4 file system /var/lib/docker/aufs
      Output information may be incomplete.
COMMAND   PID USER   FD   TYPE DEVICE SIZE/OFF    NODE NAME
test3   21141    a  cwd    DIR  252,1     4096 6636002 /home/a/code/libfault
test3   21141    a  rtd    DIR  252,1     4096       2 /
test3   21141    a  txt    REG  252,1     8801  269296 /home/a/code/libfault/t/test3
test3   21141    a  mem    REG  252,1  1845024 1573179 /lib/x86_64-linux-gnu/libc-2.19.so
test3   21141    a  mem    REG  252,1    32977  269282 /home/a/code/libfault/lib/libfaultpreload.so
test3   21141    a  mem    REG  252,1   149120 1573173 /lib/x86_64-linux-gnu/ld-2.19.so
test3   21141    a    0u   CHR  136,6      0t0       9 /dev/pts/6
test3   21141    a    1w  FIFO    0,8      0t0 8899510 pipe
test3   21141    a    2w  FIFO    0,8      0t0 8899510 pipe
test3   21141    a    4w  FIFO    0,8      0t0 8899510 pipe
--------------------------------------

./bin/libfault: line 5: 21141 Aborted                 (core dumped) LD_PRELOAD="$BASEDIR/libfaultpreload.so $LD_PRELOAD" "$@"
```

## Usage

There are two ways to use `libfault`, and both of them are quite easy:

### Using the API

The recommended way to use `libfault.c` is to just copy the
`lib/libfault.c` and `lib/libfault.h` files into your project, and
link to them. The following example demonstrates the basic API:

```c
#include "libfault.h"

void setup_crash_reports() {
  libfault_init();
  libfault_set_app_name("Testing application");  /* Name put into log */
  libfault_set_app_version("0.0");               /* Version put into log */
  libfault_set_log_name("/tmp/libfault-test1."); /* Prefix for log files */
  libfault_set_bugreport_url("https://foo.com/bugs.cgi?report=new");
  libfault_install_handlers();
}

int main() {
  setup_crash_reports();
  return 0;
}
```

That's it. Now, if your program crashes, you'll get a nice crash
report and it'll be pushed out to a log file as well.

For an example of the reports that are generated, try running `make
test`, and then run `./t/test1` or `./t/test2`.

The full user API is documented in `lib/libfault.h`.

**Note**: The `Makefile` also by default installs a `libfault.so` and
`libfault.a`, as well as `libfault.h` when you run `make
install`. Thus, you could also install it globally into your system
and link normally with `gcc -lfault foo.c`. See below for more on the
`libfault` tool and installation.

### The `libfault` tool

By default, the `Makefile` will build a shared object named
`libfault.so`, and if you run `make install`, it will install it as
well as a `libfault` tool. This tool injects a basic crash reporting
hook into any application via `$LD_PRELOAD`, so you can wrap arbitrary
applications with it to test things or give more informative crash
reports.

Usage is simple:

```
$ libfault /path/to/application
```

If you simply run `make` and do not run `make install`, you can still
use the `libfault` tool by just running `./bin/libfault`, which will
locate the correct in-place shared library.

### Environment variable options

You can change the behavior of `libfault` through some environment
variables:

  - `LIBFAULT_ABORT_HANDLER`: enabled by default. If set to any other
    value than `y`, `yes`, `true`, or `on`, the signal handlers will
    not be installed and the application will crash normally.

  - `LIBFAULT_BEEP_ON_ABORT`: disabled by default. If set to `y`,
    `yes`, `true` or `on`, the signal handler will call the `beep`
    command in a child process to alert you. See notes below about the
    `beep` command.

  - `LIBFAULT_STOP_ON_ABORT`: disabled by default. If set to `y`,
    `yes`, `true` or `on`, the signal handler will call
    `raise(SIGSTOP)` early after being invoked. This is useful if you
    want to attach to the halted process with GDB or some other
    tool. The application can be continued by sending it the `SIGCONT`
    signal.

All of these options can be used with the `libfault` tool, or any
application linked to the `libfault` library.

**A note about the `beep` command**: On modern Ubuntu systems (and
probably some others), the `beep` command will not make a sound even
if it's installed. You'll need to load the `pcspkr` driver; give
`modprobe pcspkr` a shot and try again.

## Installation

Simply run `make install` to install the `libfault` tool.

Note that the `Makefile` also properly supports the `PREFIX=` and
`DESTDIR=` environment variables. For example:

    make                                 # creates libfault.so
    make install                         # binaries to /usr/local/{bin,lib} by default
    make install PREFIX=$HOME            # install to $HOME/{bin,lib}
    make install DESTDIR=tmp PREFIX=/usr # install to ./tmp/usr/{bin,lib}

## Join in

Be sure to read the [contributing guidelines][contribute]. File bugs
in the GitHub [issue tracker][].

Master [git repository][gh]:

  * `git clone https://github.com/thoughtpolice/libfault.git`

There's also a [BitBucket mirror][bb]:

  * `git clone https://bitbucket.org/thoughtpolice/libfault.git`

## Authors

See [AUTHORS.txt](https://raw.github.com/thoughtpolice/libfault/master/AUTHORS.txt).

## License

Dual BSD3. See
[LICENSE.txt](https://raw.github.com/thoughtpolice/libfault/master/LICENSE.txt)
for specific terms of copyright and redistribution.

[contribute]: https://github.com/thoughtpolice/libfault/blob/master/CONTRIBUTING.md
[issue tracker]: http://github.com/thoughtpolice/libfault/issues
[gh]: http://github.com/thoughtpolice/libfault
[bb]: http://bitbucket.org/thoughtpolice/libfault
