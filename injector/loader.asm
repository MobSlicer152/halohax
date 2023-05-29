BITS 64

; Thread data
;    dllName 0 8
;    GetLastError 8 8
;    LoadLibraryA 16 8
;    ExitThread 24 8

LoadHaxDll:
    ; No register saving because the thread exits
    PUSH RBP
    MOV RBP, RSP

    ; Thread data is the only argument
    MOV R12, RCX

    ; Load the DLL
    MOV RCX, QWORD [R12]
    MOV R11, QWORD [R12 + 16]
    CALL R11
    TEST RAX, RAX
    JZ LoadHaxDllError
    
    MOV RCX, 7777777H
    JMP LoadHaxDllEnd

LoadHaxDllError:
    MOV R11, QWORD [R12 + 8]
    CALL R11
    MOV RCX, RAX
LoadHaxDllEnd:
    MOV R11, QWORD [R12 + 24]
    CALL R11
