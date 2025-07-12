global _start
extern term_main

extern XOpenDisplay
extern XDefaultScreen
extern XRootWindow
extern XCreateSimpleWindow
extern XMapWindow
extern XNextEvent

section .data
    event_space: times 64 db 0    ; XEvent (safe padding)
    error_msg db "Failed to open display", 10, 0

section .text
_start:
    ; Align stack (gcc requires 16-byte alignment before calling main)
    push rbp
    mov rbp, rsp
    and rsp, -16                  ; align to 16 bytes

    ; === XOpenDisplay(NULL) ===
    xor rdi, rdi                 ; NULL
    call XOpenDisplay
    test rax, rax
    jz .fail
    mov r12, rax                 ; Display*

    ; === XDefaultScreen(display) ===
    mov rdi, r12
    call XDefaultScreen
    mov r13d, eax                ; screen

    ; === XRootWindow(display, screen) ===
    mov rdi, r12
    mov esi, r13d
    call XRootWindow
    mov r14, rax                 ; root window

    ; === XCreateSimpleWindow(...) ===
    mov rdi, r12                 ; Display*
    mov rsi, r14                 ; parent (root)
    mov edx, 100                 ; x
    mov ecx, 100                 ; y
    mov r8d, 800                 ; width
    mov r9d, 600                 ; height

    sub rsp, 32                  ; shadow space + align
    mov dword [rsp+16], 2        ; border_width
    mov dword [rsp+24], 0xFF0000 ; border_color (red)
    mov dword [rsp+32], 0x000000 ; background_color (black)
    call XCreateSimpleWindow
    add rsp, 32
    mov r15, rax                 ; window

    ; === XMapWindow(display, window) ===
    mov rdi, r12
    mov rsi, r15
    call XMapWindow

    ; === Call C term_main(Display*, Window) ===
    mov rdi, r12    ; Display*
    mov rsi, r15    ; Window
    call term_main

    ; === Exit when C returns ===
    mov eax, 60
    xor edi, edi
    syscall

.fail:
    ; Print error and exit(1)
    mov rdi, error_msg
    call print_str
    mov eax, 60     ; syscall: exit
    mov edi, 1      ; exit code 1
    syscall

print_str:
    ; Print null-terminated string at rdi
    push rdi
.find_len:
    cmp byte [rdi], 0
    je .found_len
    inc rdi
    jmp .find_len
.found_len:
    pop rsi         ; rsi = original ptr
    mov rdx, rdi
    sub rdx, rsi    ; len = end - start
    mov rax, 1      ; syscall: write
    mov rdi, 1      ; fd: stdout
    syscall
    ret