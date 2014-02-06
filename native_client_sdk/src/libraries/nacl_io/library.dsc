{
  'TOOLS': ['newlib', 'glibc', 'pnacl', 'linux'],
  'SEARCH': [
    '.',
    'pepper',
    '../third_party/newlib-extras',
    'include',
  ],
  'TARGETS': [
    {
      'NAME' : 'nacl_io',
      'TYPE' : 'lib',
      'SOURCES' : [
        'dbgprint.c',
        "devfs/dev_fs.cc",
        "devfs/jspipe_node.cc",
        "devfs/tty_event_emitter.cc",
        "devfs/tty_node.cc",
        "dir_node.cc",
        "event_emitter.cc",
        "event_listener.cc",
        "fifo_char.cc",
        "filesystem.cc",
        "fusefs/fuse_fs.cc",
        "fusefs/fuse_fs_factory.cc",
        "getdents_helper.cc",
        "h_errno.cc",
        "host_resolver.cc",
        "html5fs/html5_fs.cc",
        "html5fs/html5_fs_node.cc",
        "httpfs/http_fs.cc",
        "httpfs/http_fs_node.cc",
        "in6_addr.c",
        "kernel_handle.cc",
        "kernel_intercept.cc",
        "kernel_object.cc",
        "kernel_proxy.cc",
        "kernel_wrap_dummy.cc",
        "kernel_wrap_glibc.cc",
        "kernel_wrap_newlib.cc",
        "kernel_wrap_win.cc",
        "memfs/mem_fs.cc",
        "memfs/mem_fs_node.cc",
        "nacl_io.cc",
        "node.cc",
        "passthroughfs/passthrough_fs.cc",
        "path.cc",
        "pepper_interface.cc",
        "pepper_interface_delegate.cc",
        "pipe/pipe_event_emitter.cc",
        "pipe/pipe_node.cc",
        "real_pepper_interface.cc",
        "socket/fifo_packet.cc",
        "socket/packet.cc",
        "socket/socket_node.cc",
        "socket/tcp_event_emitter.cc",
        "socket/tcp_node.cc",
        "socket/udp_event_emitter.cc",
        "socket/udp_node.cc",
        "stream/stream_event_emitter.cc",
        "stream/stream_fs.cc",
        "stream/stream_node.cc",
        "syscalls/accept.c",
        "syscalls/access.c",
        "syscalls/bind.c",
        "syscalls/cfgetispeed.c",
        "syscalls/cfgetospeed.c",
        "syscalls/cfsetspeed.c",
        "syscalls/cfsetispeed.c",
        "syscalls/cfsetospeed.c",
        "syscalls/chdir.c",
        "syscalls/chmod.c",
        "syscalls/chown.c",
        "syscalls/connect.c",
        "syscalls/fchdir.c",
        "syscalls/fchmod.c",
        "syscalls/fchown.c",
        "syscalls/fcntl.c",
        "syscalls/fdatasync.c",
        "syscalls/fdopen.c",
        "syscalls/fsync.c",
        "syscalls/ftruncate.c",
        "syscalls/getcwd.c",
        "syscalls/gethostbyname.c",
        "syscalls/getpeername.c",
        "syscalls/getsockname.c",
        "syscalls/getsockopt.c",
        "syscalls/getwd.c",
        "syscalls/herror.c",
        "syscalls/hstrerror.c",
        "syscalls/htonl.c",
        "syscalls/htons.c",
        "syscalls/inet_addr.c",
        "syscalls/inet_aton.c",
        "syscalls/inet_ntoa.c",
        "syscalls/inet_ntop.cc",
        "syscalls/inet_pton.c",
        "syscalls/ioctl.c",
        "syscalls/isatty.c",
        "syscalls/kill.c",
        "syscalls/killpg.c",
        "syscalls/lchown.c",
        "syscalls/link.c",
        "syscalls/listen.c",
        "syscalls/lstat.c",
        "syscalls/mkdir.c",
        # Not called mount.c to avoid object file naming conflict with
        # mount.cc.
        "syscalls/syscall_mount.c",
        "syscalls/ntohl.c",
        "syscalls/ntohs.c",
        "syscalls/pipe.c",
        "syscalls/poll.c",
        "syscalls/readlink.c",
        "syscalls/rmdir.c",
        "syscalls/recv.c",
        "syscalls/recvfrom.c",
        "syscalls/recvmsg.c",
        "syscalls/remove.c",
        "syscalls/rename.c",
        "syscalls/tcdrain.c",
        "syscalls/tcflow.c",
        "syscalls/tcflush.c",
        "syscalls/tcgetattr.c",
        "syscalls/tcsendbreak.c",
        "syscalls/tcsetattr.c",
        "syscalls/truncate.c",
        "syscalls/select.c",
        "syscalls/send.c",
        "syscalls/sendmsg.c",
        "syscalls/sendto.c",
        "syscalls/setsockopt.c",
        "syscalls/shutdown.c",
        "syscalls/sigaction.c",
        "syscalls/sigaddset.c",
        "syscalls/sigdelset.c",
        "syscalls/sigemptyset.c",
        "syscalls/sigfillset.c",
        "syscalls/sigismember.c",
        "syscalls/signal.c",
        "syscalls/sigpause.c",
        "syscalls/sigpending.c",
        "syscalls/sigset.c",
        "syscalls/sigsuspend.c",
        "syscalls/socket.c",
        "syscalls/socketpair.c",
        "syscalls/symlink.c",
        "syscalls/unlink.c",
        "syscalls/umount.c",
        "syscalls/uname.c",
        "syscalls/utime.c",
        "syscalls/utimes.c",
      ],
    }
  ],
  'HEADERS': [
    {
      'FILES': [
        "char_node.h",
        "dbgprint.h",
        "devfs/dev_fs.h",
        "devfs/jspipe_node.h",
        "devfs/tty_event_emitter.h",
        "devfs/tty_node.h",
        "dir_node.h",
        "error.h",
        "event_emitter.h",
        "event_listener.h",
        "fifo_char.h",
        "fifo_interface.h",
        "fifo_null.h",
        "filesystem.h",
        "fs_factory.h",
        "fusefs/fuse_fs_factory.h",
        "fusefs/fuse_fs.h",
        "fuse.h",
        "getdents_helper.h",
        "host_resolver.h",
        "html5fs/html5_fs.h",
        "html5fs/html5_fs_node.h",
        "httpfs/http_fs.h",
        "httpfs/http_fs_node.h",
        "inode_pool.h",
        "ioctl.h",
        "kernel_handle.h",
        "kernel_intercept.h",
        "kernel_object.h",
        "kernel_proxy.h",
        "kernel_wrap.h",
        "kernel_wrap_real.h",
        "memfs/mem_fs.h",
        "memfs/mem_fs_node.h",
        "nacl_io.h",
        "node.h",
        "osdirent.h",
        "osinttypes.h",
        "osmman.h",
        "ossignal.h",
        "ossocket.h",
        "osstat.h",
        "ostermios.h",
        "ostime.h",
        "ostypes.h",
        "osunistd.h",
        "osutime.h",
        "passthroughfs/passthrough_fs.h",
        "path.h",
        "pepper_interface_delegate.h",
        "pepper_interface_dummy.h",
        "pepper_interface.h",
        "pipe/pipe_event_emitter.h",
        "pipe/pipe_node.h",
        "real_pepper_interface.h",
        "socket/fifo_packet.h",
        "socket/packet.h",
        "socket/socket_node.h",
        "socket/tcp_event_emitter.h",
        "socket/tcp_node.h",
        "socket/udp_event_emitter.h",
        "socket/udp_node.h",
        "stream/stream_event_emitter.h",
        "stream/stream_fs.h",
        "stream/stream_node.h",
        "typed_fs_factory.h",
      ],
      'DEST': 'include/nacl_io',
    },
    {
      'FILES': [
        "arpa/inet.h",
        "netdb.h",
        "netinet/in.h",
        "netinet/tcp.h",
        "netinet6/in6.h",
        "poll.h",
        "sys/ioctl.h",
        "sys/mount.h",
        "sys/poll.h",
        "sys/select.h",
        "sys/signal.h",
        "sys/socket.h",
        "sys/termios.h",
        "sys/utsname.h",
      ],
      'DEST': 'include/newlib',
    },
    {
      'FILES': [
        "arpa/inet.h",
        "netdb.h",
        "netinet/in.h",
        "netinet/tcp.h",
        "netinet6/in6.h",
        "poll.h",
        "sys/ioctl.h",
        "sys/mount.h",
        "sys/poll.h",
        "sys/select.h",
        "sys/socket.h",
        "sys/termios.h",
        "sys/utsname.h",
      ],
      'DEST': 'include/pnacl',
    },
    {
      'FILES': [
        "poll.h",
        "sys/poll.h",
      ],
      'DEST': 'include/win',
    },
    {
      'FILES': [
        "all_interfaces.h",
        "define_empty_macros.h",
        "undef_macros.h",
      ],
      'DEST': 'include/nacl_io/pepper',
    }
  ],
  'DEST': 'src',
  'NAME': 'nacl_io',
}
