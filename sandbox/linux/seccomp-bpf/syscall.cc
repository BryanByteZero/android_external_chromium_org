// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/seccomp-bpf/syscall.h"

#include <asm/unistd.h>
#include <errno.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"

namespace sandbox {

namespace {

#if defined(ARCH_CPU_X86_FAMILY) || defined(ARCH_CPU_ARM_FAMILY) || \
    defined(ARCH_CPU_MIPS_FAMILY)
// Number that's not currently used by any Linux kernel ABIs.
const int kInvalidSyscallNumber = 0x351d3;
#else
#error Unrecognized architecture
#endif

asm(// We need to be able to tell the kernel exactly where we made a
    // system call. The C++ compiler likes to sometimes clone or
    // inline code, which would inadvertently end up duplicating
    // the entry point.
    // "gcc" can suppress code duplication with suitable function
    // attributes, but "clang" doesn't have this ability.
    // The "clang" developer mailing list suggested that the correct
    // and portable solution is a file-scope assembly block.
    // N.B. We do mark our code as a proper function so that backtraces
    // work correctly. But we make absolutely no attempt to use the
    // ABI's calling conventions for passing arguments. We will only
    // ever be called from assembly code and thus can pick more
    // suitable calling conventions.
#if defined(__i386__)
    ".text\n"
    ".align 16, 0x90\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.cfi_startproc\n"
    // Check if "%eax" is negative. If so, do not attempt to make a
    // system call. Instead, compute the return address that is visible
    // to the kernel after we execute "int $0x80". This address can be
    // used as a marker that BPF code inspects.
    "test %eax, %eax\n"
    "jge  1f\n"
    // Always, make sure that our code is position-independent, or
    // address space randomization might not work on i386. This means,
    // we can't use "lea", but instead have to rely on "call/pop".
    "call 0f;   .cfi_adjust_cfa_offset  4\n"
    "0:pop  %eax; .cfi_adjust_cfa_offset -4\n"
    "addl $2f-0b, %eax\n"
    "ret\n"
    // Save register that we don't want to clobber. On i386, we need to
    // save relatively aggressively, as there are a couple or registers
    // that are used internally (e.g. %ebx for position-independent
    // code, and %ebp for the frame pointer), and as we need to keep at
    // least a few registers available for the register allocator.
    "1:push %esi; .cfi_adjust_cfa_offset 4\n"
    "push %edi; .cfi_adjust_cfa_offset 4\n"
    "push %ebx; .cfi_adjust_cfa_offset 4\n"
    "push %ebp; .cfi_adjust_cfa_offset 4\n"
    // Copy entries from the array holding the arguments into the
    // correct CPU registers.
    "movl  0(%edi), %ebx\n"
    "movl  4(%edi), %ecx\n"
    "movl  8(%edi), %edx\n"
    "movl 12(%edi), %esi\n"
    "movl 20(%edi), %ebp\n"
    "movl 16(%edi), %edi\n"
    // Enter the kernel.
    "int  $0x80\n"
    // This is our "magic" return address that the BPF filter sees.
    "2:"
    // Restore any clobbered registers that we didn't declare to the
    // compiler.
    "pop  %ebp; .cfi_adjust_cfa_offset -4\n"
    "pop  %ebx; .cfi_adjust_cfa_offset -4\n"
    "pop  %edi; .cfi_adjust_cfa_offset -4\n"
    "pop  %esi; .cfi_adjust_cfa_offset -4\n"
    "ret\n"
    ".cfi_endproc\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__x86_64__)
    ".text\n"
    ".align 16, 0x90\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.cfi_startproc\n"
    // Check if "%rax" is negative. If so, do not attempt to make a
    // system call. Instead, compute the return address that is visible
    // to the kernel after we execute "syscall". This address can be
    // used as a marker that BPF code inspects.
    "test %rax, %rax\n"
    "jge  1f\n"
    // Always make sure that our code is position-independent, or the
    // linker will throw a hissy fit on x86-64.
    "call 0f;   .cfi_adjust_cfa_offset  8\n"
    "0:pop  %rax; .cfi_adjust_cfa_offset -8\n"
    "addq $2f-0b, %rax\n"
    "ret\n"
    // We declared all clobbered registers to the compiler. On x86-64,
    // there really isn't much of a problem with register pressure. So,
    // we can go ahead and directly copy the entries from the arguments
    // array into the appropriate CPU registers.
    "1:movq  0(%r12), %rdi\n"
    "movq  8(%r12), %rsi\n"
    "movq 16(%r12), %rdx\n"
    "movq 24(%r12), %r10\n"
    "movq 32(%r12), %r8\n"
    "movq 40(%r12), %r9\n"
    // Enter the kernel.
    "syscall\n"
    // This is our "magic" return address that the BPF filter sees.
    "2:ret\n"
    ".cfi_endproc\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__arm__)
    // Throughout this file, we use the same mode (ARM vs. thumb)
    // that the C++ compiler uses. This means, when transfering control
    // from C++ to assembly code, we do not need to switch modes (e.g.
    // by using the "bx" instruction). It also means that our assembly
    // code should not be invoked directly from code that lives in
    // other compilation units, as we don't bother implementing thumb
    // interworking. That's OK, as we don't make any of the assembly
    // symbols public. They are all local to this file.
    ".text\n"
    ".align 2\n"
    ".type SyscallAsm, %function\n"
#if defined(__thumb__)
    ".thumb_func\n"
#else
    ".arm\n"
#endif
    "SyscallAsm:.fnstart\n"
    "@ args = 0, pretend = 0, frame = 8\n"
    "@ frame_needed = 1, uses_anonymous_args = 0\n"
#if defined(__thumb__)
    ".cfi_startproc\n"
    "push {r7, lr}\n"
    ".cfi_offset 14, -4\n"
    ".cfi_offset  7, -8\n"
    "mov r7, sp\n"
    ".cfi_def_cfa_register 7\n"
    ".cfi_def_cfa_offset 8\n"
#else
    "stmfd sp!, {fp, lr}\n"
    "add fp, sp, #4\n"
#endif
    // Check if "r0" is negative. If so, do not attempt to make a
    // system call. Instead, compute the return address that is visible
    // to the kernel after we execute "swi 0". This address can be
    // used as a marker that BPF code inspects.
    "cmp r0, #0\n"
    "bge 1f\n"
    "adr r0, 2f\n"
    "b   2f\n"
    // We declared (almost) all clobbered registers to the compiler. On
    // ARM there is no particular register pressure. So, we can go
    // ahead and directly copy the entries from the arguments array
    // into the appropriate CPU registers.
    "1:ldr r5, [r6, #20]\n"
    "ldr r4, [r6, #16]\n"
    "ldr r3, [r6, #12]\n"
    "ldr r2, [r6, #8]\n"
    "ldr r1, [r6, #4]\n"
    "mov r7, r0\n"
    "ldr r0, [r6, #0]\n"
    // Enter the kernel
    "swi 0\n"
// Restore the frame pointer. Also restore the program counter from
// the link register; this makes us return to the caller.
#if defined(__thumb__)
    "2:pop {r7, pc}\n"
    ".cfi_endproc\n"
#else
    "2:ldmfd sp!, {fp, pc}\n"
#endif
    ".fnend\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__mips__)
    ".text\n"
    ".align 4\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.ent SyscallAsm\n"
    ".frame  $sp, 40, $ra\n"
    ".set   push\n"
    ".set   noreorder\n"
    "addiu  $sp, $sp, -40\n"
    "sw     $ra, 36($sp)\n"
    // Check if "v0" is negative. If so, do not attempt to make a
    // system call. Instead, compute the return address that is visible
    // to the kernel after we execute "syscall". This address can be
    // used as a marker that BPF code inspects.
    "bgez   $v0, 1f\n"
    " nop\n"
    "la     $v0, 2f\n"
    "b      2f\n"
    " nop\n"
    // On MIPS first four arguments go to registers a0 - a3 and any
    // argument after that goes to stack. We can go ahead and directly
    // copy the entries from the arguments array into the appropriate
    // CPU registers and on the stack.
    "1:lw     $a3, 28($a0)\n"
    "lw     $a2, 24($a0)\n"
    "lw     $a1, 20($a0)\n"
    "lw     $t0, 16($a0)\n"
    "sw     $a3, 28($sp)\n"
    "sw     $a2, 24($sp)\n"
    "sw     $a1, 20($sp)\n"
    "sw     $t0, 16($sp)\n"
    "lw     $a3, 12($a0)\n"
    "lw     $a2, 8($a0)\n"
    "lw     $a1, 4($a0)\n"
    "lw     $a0, 0($a0)\n"
    // Enter the kernel
    "syscall\n"
    // This is our "magic" return address that the BPF filter sees.
    // Restore the return address from the stack.
    "2:lw     $ra, 36($sp)\n"
    "jr     $ra\n"
    " addiu  $sp, $sp, 40\n"
    ".set    pop\n"
    ".end    SyscallAsm\n"
    ".size   SyscallAsm,.-SyscallAsm\n"
#endif
    );  // asm

}  // namespace

intptr_t Syscall::InvalidCall() {
  // Explicitly pass eight zero arguments just in case.
  return Call(kInvalidSyscallNumber, 0, 0, 0, 0, 0, 0, 0, 0);
}

intptr_t Syscall::Call(int nr,
                       intptr_t p0,
                       intptr_t p1,
                       intptr_t p2,
                       intptr_t p3,
                       intptr_t p4,
                       intptr_t p5,
                       intptr_t p6,
                       intptr_t p7) {
  // We rely on "intptr_t" to be the exact size as a "void *". This is
  // typically true, but just in case, we add a check. The language
  // specification allows platforms some leeway in cases, where
  // "sizeof(void *)" is not the same as "sizeof(void (*)())". We expect
  // that this would only be an issue for IA64, which we are currently not
  // planning on supporting. And it is even possible that this would work
  // on IA64, but for lack of actual hardware, I cannot test.
  COMPILE_ASSERT(sizeof(void*) == sizeof(intptr_t),
                 pointer_types_and_intptr_must_be_exactly_the_same_size);

  // TODO(nedeljko): Enable use of more than six parameters on architectures
  //                 where that makes sense.
#if defined(__mips__)
  const intptr_t args[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
#else
  DCHECK_EQ(p6, 0) << " Support for syscalls with more than six arguments not "
                      "added for this architecture";
  DCHECK_EQ(p7, 0) << " Support for syscalls with more than six arguments not "
                      "added for this architecture";
  const intptr_t args[6] = {p0, p1, p2, p3, p4, p5};
#endif  // defined(__mips__)

// Invoke our file-scope assembly code. The constraints have been picked
// carefully to match what the rest of the assembly code expects in input,
// output, and clobbered registers.
#if defined(__i386__)
  intptr_t ret = nr;
  asm volatile(
      "call SyscallAsm\n"
      // N.B. These are not the calling conventions normally used by the ABI.
      : "=a"(ret)
      : "0"(ret), "D"(args)
      : "cc", "esp", "memory", "ecx", "edx");
#elif defined(__x86_64__)
  intptr_t ret = nr;
  {
    register const intptr_t* data __asm__("r12") = args;
    asm volatile(
        "lea  -128(%%rsp), %%rsp\n"  // Avoid red zone.
        "call SyscallAsm\n"
        "lea  128(%%rsp), %%rsp\n"
        // N.B. These are not the calling conventions normally used by the ABI.
        : "=a"(ret)
        : "0"(ret), "r"(data)
        : "cc",
          "rsp",
          "memory",
          "rcx",
          "rdi",
          "rsi",
          "rdx",
          "r8",
          "r9",
          "r10",
          "r11");
  }
#elif defined(__arm__)
  intptr_t ret;
  {
    register intptr_t inout __asm__("r0") = nr;
    register const intptr_t* data __asm__("r6") = args;
    asm volatile(
        "bl SyscallAsm\n"
        // N.B. These are not the calling conventions normally used by the ABI.
        : "=r"(inout)
        : "0"(inout), "r"(data)
        : "cc",
          "lr",
          "memory",
          "r1",
          "r2",
          "r3",
          "r4",
          "r5"
#if !defined(__thumb__)
          // In thumb mode, we cannot use "r7" as a general purpose register, as
          // it is our frame pointer. We have to manually manage and preserve
          // it.
          // In ARM mode, we have a dedicated frame pointer register and "r7" is
          // thus available as a general purpose register. We don't preserve it,
          // but instead mark it as clobbered.
          ,
          "r7"
#endif  // !defined(__thumb__)
        );
    ret = inout;
  }
#elif defined(__mips__)
  int err_status;
  intptr_t ret = Syscall::SandboxSyscallRaw(nr, args, &err_status);

  if (err_status) {
    // On error, MIPS returns errno from syscall instead of -errno.
    // The purpose of this negation is for SandboxSyscall() to behave
    // more like it would on other architectures.
    ret = -ret;
  }
#else
#error "Unimplemented architecture"
#endif
  return ret;
}

void Syscall::PutValueInUcontext(intptr_t ret_val, ucontext_t* ctx) {
#if defined(__mips__)
  // Mips ABI states that on error a3 CPU register has non zero value and if
  // there is no error, it should be zero.
  if (ret_val <= -1 && ret_val >= -4095) {
    // |ret_val| followes the Syscall::Call() convention of being -errno on
    // errors. In order to write correct value to return register this sign
    // needs to be changed back.
    ret_val = -ret_val;
    SECCOMP_PARM4(ctx) = 1;
  } else
    SECCOMP_PARM4(ctx) = 0;
#endif
  SECCOMP_RESULT(ctx) = static_cast<greg_t>(ret_val);
}

#if defined(__mips__)
intptr_t Syscall::SandboxSyscallRaw(int nr,
                                    const intptr_t* args,
                                    intptr_t* err_ret) {
  register intptr_t ret __asm__("v0") = nr;
  // a3 register becomes non zero on error.
  register intptr_t err_stat __asm__("a3") = 0;
  {
    register const intptr_t* data __asm__("a0") = args;
    asm volatile(
        "la $t9, SyscallAsm\n"
        "jalr $t9\n"
        " nop\n"
        : "=r"(ret), "=r"(err_stat)
        : "0"(ret),
          "r"(data)
          // a2 is in the clober list so inline assembly can not change its
          // value.
        : "memory", "ra", "t9", "a2");
  }

  // Set an error status so it can be used outside of this function
  *err_ret = err_stat;

  return ret;
}
#endif  // defined(__mips__)

}  // namespace sandbox
