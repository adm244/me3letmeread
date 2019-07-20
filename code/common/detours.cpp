/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

//IMPORTANT(adm244): SCRATCH VERSION JUST TO GET IT UP WORKING

#ifndef _DETOURS_CPP
#define _DETOURS_CPP

#define DETOUR_LENGTH 5
//#define DETOUR_TEMP_CODE_SIZE 512

internal void * FindWString(void *beginAddress, void *endAddress, wchar_t *str)
{
  size_t stringSize = wcslen(str) * sizeof(wchar_t);
  
  for (char *p = (char *)beginAddress; p < endAddress; ++p) {
    bool matched = true;
    char *str_p = (char *)str;
    
    for (size_t i = 0; i < stringSize; ++i) {
      if (str_p[i] != *(p + i)) {
        matched = false;
        break;
      }
    }
    
    if (matched)
      return p;
  }
  
  return 0;
}

//TODO(adm244): switch to multiple patterns search (Aho-Corasick?)
internal u64 FindSignature(MODULEINFO *moduleInfo, char *pattern, char *mask, u64 offset)
{
  u64 startAddress = (u64)moduleInfo->lpBaseOfDll;
  u64 blockSize = moduleInfo->SizeOfImage;
  u64 patternSize = strlen(mask ? mask : pattern);
  u64 endAddress = startAddress + blockSize - patternSize;

  for (u64 i = startAddress; i < endAddress; ++i) {
    bool matched = true;
    for (u64 j = 0; j < patternSize; ++j) {
      if (mask && mask[j] == '?') continue;
      if (pattern[j] != *((char *)(i + j))) {
        matched = false;
        break;
      }
    }
    
    if (matched) return (i + offset);
  }
  
  return 0;
}

internal bool WriteMemory(void *dest, void *buffer, int length)
{
  DWORD oldProtection;
  BOOL result = VirtualProtect(dest, length, PAGE_EXECUTE_READWRITE, &oldProtection);
  if (!result) {
    return false;
  }
  
  /*for (int i = 0; i < length; ++i) {
    ((u8 *)dest)[i] = ((u8 *)buffer)[i];
  }*/
  memcpy(dest, buffer, length);
  
  result = VirtualProtect(dest, length, oldProtection, &oldProtection);
  if (!result) {
    return false;
  }
  
  return true;
}

internal void * RIPRel8(void *src, u8 opcode_length)
{
  i8 offset = *((i8 *)((u64)src + opcode_length));
  u64 rip = (u64)src + opcode_length + sizeof(u8);
  
  return (void *)(rip + offset);
}

internal void * RIPRel32(void *src, u8 opcode_length)
{
  i32 offset = *((i32 *)((u64)src + opcode_length));
  u64 rip = (u64)src + opcode_length + sizeof(u32);
  
  return (void *)(rip + offset);
}

internal i32 RIPRelOffset32(void *rip, void *dest)
{
  return (i32)dest - (i32)rip;
}

/// Assembles a relative 32-bit jump
internal void * JumpRel32(void *src, void *dest)
{
  u8 *p = (u8 *)src;
  u8 *pend = (u8 *)(p + DETOUR_LENGTH);

  // assemble jmp [dest]
  p[0] = 0xE9;
  *((i32 *)(&p[1])) = RIPRelOffset32(pend, dest);
  
  return pend;
}

/// Writes a detour to dest at src nop'ing extra space
internal bool WriteDetour(void *src, void *dest, int padding)
{
  DWORD oldProtection;
  BOOL result = VirtualProtect(src, DETOUR_LENGTH, PAGE_EXECUTE_READWRITE, &oldProtection);
  if (!result) {
    return false;
  }
  
  JumpRel32(src, dest);
  
  for (int i = DETOUR_LENGTH; i < (DETOUR_LENGTH + padding); ++i) {
    ((u8 *)src)[i] = 0x90;
  }
  
  result = VirtualProtect(src, DETOUR_LENGTH, oldProtection, &oldProtection);
  if (!result) {
    return false;
  }
  
  return true;
}

/*
/// Assembles a relative 32-bit call
internal void * CallRel32(void *src, void *dest)
{
  u8 *p = (u8 *)src;
  u8 *pend = (u8 *)(p + DETOUR_LENGTH);

  // assemble call [dest]
  p[0] = 0xE8;
  *((i32 *)(&p[1])) = RIPRelOffset32(pend, dest);
  
  return pend;
}

internal __declspec(naked) void _SaveRegs_asm()
{
  _asm {
    pushfd
    push ebx
    push esi
    push edi
    push ebp
    push esp
    
    int 3
  }
}

internal __declspec(naked) void _RestoreRegs_asm()
{
  _asm {
    pop esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    popfd
    
    int 3
  }
}

internal uint GetFuncBodyLength(void *src)
{
  u8 *p = (u8 *)src;
  u8 *pend = p;
  
  //NOTE(adm244): yeah, not a wooden sword
  while (*pend != 0xCC) {
    ++pend;
  }
  
  return (uint)((u64)pend - (u64)p);
}

internal void * CopyFuncBody(void *src, void *dest)
{
  uint body_length = GetFuncBodyLength(src);
  memcpy(dest, src, body_length);
  
  return (void *)((u64)dest + body_length);
}

internal void * SaveRegs(void *src)
{
  return CopyFuncBody(&_SaveRegs_asm, src);
}

internal void * RestoreRegs(void *src)
{
  return CopyFuncBody(&_RestoreRegs_asm, src);
}

internal void * CopyOriginalCode(void *src, void *dest, u32 len, uint *jump_offsets, uint jump_offsets_length)
{
  u8 code[128] = {0};
  u8 jumps[64] = {0};
  
  u8 *p = (u8 *)src;
  u8 *pjumps = (u8 *)jumps;
  
  uint i = 0;
  for ( ; i < len; ) {
    bool is_jump = false;
    for (int j = 0; j < jump_offsets_length; ++j) {
      if (jump_offsets[j] == i) {
        is_jump = true;
        break;
      }
    }
    
    if (is_jump) {
      u32 offset = 0;
      
      switch (*p) {
        case 0x75: { // jnz cb
          offset = *(p + 1);
          code[i] = *p;
          code[i + 1] = (i8)(pjumps - jumps);
          
          p += 2;
          i += 2;
        } break;
        
        default: {
          assert(false);
        } break;
      }
      
      pjumps = (u8 *)JumpRel32(pjumps, (void *)(p + offset));
    } else {
      code[i] = *p;
      ++i;
      ++p;
    }
  }
  
  uint code_length = i;
  uint jumps_length = (uint)((u64)pjumps - (u64)jumps);
  
  memcpy(dest, code, code_length);
  memcpy((u8 *)dest + code_length, jumps, jumps_length);
  
  return (void *)((u64)dest + code_length + jumps_length);
}

/// Writes a detour to dest at src and copying len bytes of original instructions
internal bool Detour(void *src, void *dest, u32 len, uint *jump_offsets, uint jump_offsets_length)
{
  //IMPORTANT(adm244): make sure that len covers all overwritten instructions!

#ifdef DEBUG
  assert(len >= DETOUR_LENGTH);
#endif
  
  u8 code[DETOUR_TEMP_CODE_SIZE] = {0};
  void *p = (void *)&code[0];
  void *pbegin = p;
  void *pend = (void *)((u64)pbegin + DETOUR_TEMP_CODE_SIZE);
  
  p = SaveRegs(p);
#ifdef DEBUG
  assert(p < pend);
#endif
  
  p = CallRel32(p, dest);
#ifdef DEBUG
  assert(p < pend);
#endif
  
  p = RestoreRegs(p);
#ifdef DEBUG
  assert(p < pend);
#endif
  
  p = CopyOriginalCode(src, p, len, jump_offsets, jump_offsets_length);
  
  //memcpy(p, src, len);
  //p = (void *)((u64)p + len);
#ifdef DEBUG
  assert(p <= pend);
#endif
  
  p = JumpRel32(p, (u8 *)src + len);
#ifdef DEBUG
  assert(p <= pend);
#endif
  
  u64 code_size = (u64)p - (u64)pbegin;
  void *memblock = VirtualAlloc(0, code_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!memblock) {
    return false;
  }
  
  memcpy(memblock, pbegin, code_size);
  
  DWORD oldProtection;
  BOOL result = VirtualProtect(memblock, code_size, PAGE_EXECUTE, &oldProtection);
  if (!result) {
    return false;
  }
  
  result = WriteDetour(src, memblock, len - DETOUR_LENGTH);
  if (!result) {
    return false;
  }
  
  return true;
}
*/
#endif
