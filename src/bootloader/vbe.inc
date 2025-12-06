struc VesaInfoBlock
    .Signature resb 4
    .Version resw 1
    .OEMNamePtr resd 1
    .Capabilities resd 1
    .VideoModesOffset resw 1
    .VideoModesSegment resw 1
    .CountOf64KBlocks resw 1
    .OEMSoftwareRevision resw 1
    .OEMVendorNamePtr resd 1
    .OEMProductNamePtr resd 1
    .OEMProductRevisionPtr resd 1
    .Reserved resb 222
    .OEMData resb 256
endstruc

struc VesaModeInfoBlock
    .ModeAttributes resw 1
    .FirstWindowAttributes resb 1
    .SecondWindowAttributes resb 1
    .WindowGranularity resw 1
    .WindowSize resw 1
    .FirstWindowSegment resw 1
    .SecondWindowSegment resw 1
    .WindowFunctionPtr resd 1
    .BytesPerScanLine resw 1
    .Width resw 1
    .Height resw 1
    .CharWidth resb 1
    .CharHeight resb 1
    .PlanesCount resb 1
    .BitsPerPixel resb 1
    .BanksCount resb 1
    .MemoryModel resb 1
    .BankSize resb 1
    .ImagePagesCount resb 1
    .Reserved1 resb 1

    .RedMaskSize resb 1
    .RedFieldPosition resb 1
    .GreenMaskSize resb 1
    .GreenFieldPosition resb 1
    .BlueMaskSize resb 1
    .BlueFieldPosition resb 1
    .ReservedMaskSize resb 1
    .ReservedMaskPosition resb 1
    .DirectColorModeInfo resb 1
    
    .LFBAddress resd 1
    .OffscreenMemoryOffset resd 1
    .OffscreenMemorySize resw 1
    .Reserved2 resb 206
endstruc

get_vesa_info:
    clc
    mov ax, 0x4f00
    int 0x10
    cmp ax, 0x004f
    jne .failed
    ret
    .failed:
        stc
        ret

get_vesa_mode_info:
    clc
    mov ax, 0x4f01
    int 0x10
    cmp ax, 0x004f
    jne .failed
    ret
    .failed:
        stc
        ret

VbeInit:
  pusha

  mov di, VesaInfoBlockBuffer

  call get_vesa_info

  push word [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesSegment]
  pop es
  mov di, VesaModeInfoBlockBuffer
  mov bx, [VesaInfoBlockBuffer + VesaInfoBlock.VideoModesOffset]
  sub bx, 2

  get_next_mode:
  add bx, 2
  mov cx, [bx]
  
  cmp cx, 0xffff
  je .NoModes
  
  push bx
  call get_vesa_mode_info
  pop bx

  jc .NoModes

  cmp byte [VesaModeInfoBlockBuffer + VesaModeInfoBlock.MemoryModel], 6
  jne get_next_mode

  cmp byte [VesaModeInfoBlockBuffer + VesaModeInfoBlock.BitsPerPixel], 15
  jne get_next_mode

  cmp word [VesaModeInfoBlockBuffer + VesaModeInfoBlock.Width], 800
  jne get_next_mode

  cmp dword [VesaModeInfoBlockBuffer + VesaModeInfoBlock.LFBAddress], 0
  je .NoModes

  mov ax, 0x4f02
  or cx, 0x4000
  mov bx, cx
  int 0x10

  cmp ax, 0x004f
  jne .NoModes

  jmp .Finished

  .NoModes:
  stc
  
  .Finished:

  popa
  ret

ALIGN(4)

vbe_info_block:
VesaInfoBlockBuffer: istruc VesaInfoBlock
    at VesaInfoBlock.Signature, db "VESA"
    times 508 db 0
iend

vbe_mode_info:
VesaModeInfoBlockBuffer: istruc VesaModeInfoBlock
    times VesaModeInfoBlock_size db 0
iend