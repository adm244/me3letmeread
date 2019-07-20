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

#define PSAPI_VERSION 1
#include <windows.h>
#include <psapi.h>
#include <stdio.h>

#ifndef DEBUG
#define NDEBUG
#endif
#include <assert.h>

#include "types.h"
#include "detours.cpp"
#include "native_types.h"

internal _BioConversation_NeedToDisplayReplies BioConversation_NeedToDisplayReplies = 0;
internal _BioConversation_IsAmbient BioConversation_IsAmbient = 0;
internal _BioConversation_GetReplyText_Internal BioConversation_GetReplyText_Internal = (_BioConversation_GetReplyText_Internal)0x00B32440;

internal bool ShouldReply(BioConversation *conversation)
{
  return (conversation->topicFlags & Topic_Patch_DialogWheelActive);
}

internal bool IsSkipped(BioConversation *conversation)
{
  //NOTE(adm244): fixes infinite-loading bug
  BioConversationEntry entry = conversation->entryList[conversation->currentEntryIndex];
  if (entry.flags & Entry_NonTextline) {
    return true;
  }
  
  //NOTE(adm244): skipes "empty" replies
  if (conversation->currentReplyIndex >= 0) {
    BioConversationEntryReply entryReply = entry.replyList[conversation->currentReplyIndex];
  
    BioString replyText = {0};
    BioConversation_GetReplyText_Internal(1, entryReply.index, conversation, &replyText);
    
    //NOTE(adm244): probably should check if string contains only whitespaces or empty
    if (replyText.length < 3) {
      return true;
    }
  }
  
  if (!BioConversation_IsAmbient(conversation) && !ShouldReply(conversation)) {
    bool isSkipped = (conversation->dialogFlags & Dialog_Patch_ManualSkip);
    conversation->dialogFlags &= ~Dialog_Patch_ManualSkip;
    return isSkipped;
  }
  
  return true;
}

internal void SkipNode(BioConversation *conversation)
{
  //FIX(adm244): cannot skip a topic after selecting a reply (with spacebar)
  // for some reason it doesn't update currentReplyIndex
  /*if ((conversation->repliesCount > 1) && (conversation->currentReplyIndex < 0)) {
    return;
  }*/
  
  BioConversationEntry entry = conversation->entryList[conversation->currentEntryIndex];
  if (!entry.skippable && (conversation->topicFlags & Topic_IsVoicePlaying)
    && (conversation->currentReplyIndex < 0)) {
    return;
  }
  
  //FIX(adm244): don't set skip flag if entry has more than one reply
  //TODO(adm244): get replyList length
  
  if ((conversation->topicFlags & Topic_IsVoicePlaying)
    || !(conversation->topicFlags & Topic_Patch_DialogWheelActive)) {
    conversation->dialogFlags |= Dialog_Patch_ManualSkip;
  }
}

internal void *skip_jz_dest_address = 0;
internal void *skip_jnz_dest_address = 0;
internal void *skip_post_jnz_address = 0;

internal void *skip_node_address = 0;
internal void *skip_node_mov_address = 0;

internal __declspec(naked) void IsSkipped_Hook()
{
  _asm {
    push ebx
    push esi
    push edi
    push ebp
    push esp
    
    push ebx
    call IsSkipped
    add esp, 4
    
    pop esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    test al, al
    jz dont_skip_dialog
    jmp skip_dialog
    
  dont_skip_dialog:
    jmp [skip_jz_dest_address]
    
  skip_dialog:
    test esi, esi
    jnz jnz_jump
    mov eax, [esp + 20h]
    jmp [skip_post_jnz_address]
    
  jnz_jump:
    jmp [skip_jnz_dest_address]
  }
}

internal __declspec(naked) void SkipNode_Hook()
{
  _asm {
    push ebx
    push esi
    push edi
    push ebp
    push esp
    
    push ecx
    call SkipNode
    pop ecx
    
    pop esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    mov eax, [skip_node_mov_address]
    mov eax, [eax]
    push esi
    push edi
    mov edi, ecx
    
    mov edx, skip_node_address
    add edx, 9
    jmp edx
  }
}

internal __declspec(naked) void RepliesActive_Hook()
{
  _asm {
    mov eax, Topic_Patch_DialogWheelActive
    or [esi+1A8h], eax
    
    mov eax, 1
    pop esi
    retn
  }
}

internal __declspec(naked) void RepliesInactive_Hook()
{
  _asm {
    mov eax, Topic_Patch_DialogWheelActive
    not eax
    and [esi+1A8h], eax
    
    xor eax, eax
    pop esi
    retn
  }
}

/*
  NOTE(adm244):
    - BioConversationController::UpdateConversation()
      - jz
      - jnz
    - BioConversationController::NeedToDisplayReplies()
      - true
      - false
    - BioConversationController::SkipNode()
    - Patch 0x00CBE877 to jmp (remove hiding of subtitle when VO is over)
*/

internal void * GetBioConversationControllerVTable(MODULEINFO *baseModuleInfo, void *beginAddress, void *endAddress)
{
  wchar_t *name = L"BioConversationController";
  void *stringLocation = FindWString(beginAddress, endAddress, name);
  assert(stringLocation != 0);
  
  char pattern[5];
  *((u32 *)pattern) = (u32)stringLocation;
  pattern[4] = 0;
  
  void *stringMOVLocation = (void *)FindSignature(baseModuleInfo, pattern, 0, -1);
  assert(stringMOVLocation != 0);
  assert(*((u8 *)stringMOVLocation) == 0xB9);
  
  void *constructor = (void *)(*((u32 *)((u8 *)stringMOVLocation - 0x16)));
  assert(constructor != 0);
  
  void *constructor_inner = RIPRel32((u8 *)constructor + 0xC, 1);
  assert(constructor_inner != 0);
  
  void *vtable = (void *)(*((u32 *)((u8 *)constructor_inner + 0x33)));
  assert(vtable != 0);
  
  assert((u32)vtable == 0x01803F50);
  
  return vtable;
}

internal void * GetBioConversationManagerVTable(MODULEINFO *baseModuleInfo, void *beginAddress, void *endAddress)
{
  wchar_t *name = L"BioConversationManager";
  void *stringLocation = FindWString(beginAddress, endAddress, name);
  assert(stringLocation != 0);
  
  char pattern[5];
  *((u32 *)pattern) = (u32)stringLocation;
  pattern[4] = 0;
  
  void *stringMOVLocation = (void *)FindSignature(baseModuleInfo, pattern, 0, -1);
  assert(stringMOVLocation != 0);
  assert(*((u8 *)stringMOVLocation) == 0xB9);
  
  void *constructor = (void *)(*((u32 *)((u8 *)stringMOVLocation - 0x16)));
  assert(constructor != 0);
  
  void *constructor_inner = RIPRel32((u8 *)constructor + 0xB, 1);
  assert(constructor_inner != 0);
  
  void *vtable = (void *)(*((u32 *)((u8 *)constructor_inner + 0x34)));
  assert(vtable != 0);
  
  assert((u32)vtable == 0x01804E98);
  
  return vtable;
}

internal void PatchExecutable()
{
  HANDLE processHandle = GetCurrentProcess();
  HMODULE baseModuleHandle = GetModuleHandle(0);
  MODULEINFO baseModuleInfo;
  BOOL result;
  
  result = GetModuleInformation(processHandle, baseModuleHandle, &baseModuleInfo, sizeof(baseModuleInfo));
  assert(result != 0);
  
  void *beginAddress = baseModuleInfo.lpBaseOfDll;
  void *endAddress = (char *)beginAddress + baseModuleInfo.SizeOfImage;
  
  // get vtables
  void *BioConversationManager_vtable = GetBioConversationManagerVTable(&baseModuleInfo, beginAddress, endAddress);
  void *BioConversationController_vtable = GetBioConversationControllerVTable(&baseModuleInfo, beginAddress, endAddress);
}

internal BOOL WINAPI DllMain(HMODULE loader, DWORD reason, LPVOID reserved)
{
  if( reason == DLL_PROCESS_ATTACH )
  {
    PatchExecutable();
  
    //TODO(adm244): get addresses by a signature search
    /*
    // get function pointers for BioConversation object
    void *bio_conversation_vtable = (void *)0x0117DF30;
    BioConversation_NeedToDisplayReplies = (_BioConversation_NeedToDisplayReplies)(*((u32 *)bio_conversation_vtable + 102));
    BioConversation_IsAmbient = (_BioConversation_IsAmbient)(*((u32 *)bio_conversation_vtable + 90));
    
    assert((u64)BioConversation_NeedToDisplayReplies == 0x00B340C0);
    assert((u64)BioConversation_IsAmbient == 0x00B34030);
    
    // hook NeedToDisplayReplies function
    void *replies_active_patch_address = (u8 *)BioConversation_NeedToDisplayReplies + 0x49;
    void *replies_inactive_patch_address = (u8 *)BioConversation_NeedToDisplayReplies + 0x81;
    
    WriteDetour(replies_active_patch_address, &RepliesActive_Hook, 2);
    WriteDetour(replies_inactive_patch_address, &RepliesInactive_Hook, 0);
    
    // hook UpdateConversation function
    void *skip_jz_address = (void *)0x00B2FE97;
    skip_jz_dest_address = RIPRel8(skip_jz_address, 1);
    assert((u64)skip_jz_dest_address == 0x00B2FEF4);
    
    void *skip_address = (void *)((u64)skip_jz_address + 2);
    void *skip_jnz_address = (void *)((u64)skip_address + 2);
    skip_jnz_dest_address = RIPRel8(skip_jnz_address, 1);
    skip_post_jnz_address = (void *)((u64)skip_jnz_address + 6);
    
    assert((u64)skip_jnz_address == 0x00B2FE9B);
    assert((u64)skip_jnz_dest_address == 0x00B2FECA);
    assert((u64)skip_address == 0x00B2FE99);
    
    WriteDetour(skip_address, &IsSkipped_Hook, 3);
    
    // hook SkipNode function
    skip_node_address = (void *)(*((u32 *)((u8 *)bio_conversation_vtable + 0x160)));
    assert((u64)skip_node_address == 0x00B30350);
    
    skip_node_mov_address = (void *)(*((u32 *)((u8 *)skip_node_address + 1)));
    assert((u64)skip_node_mov_address == 0x0126A7BC);
    
    WriteDetour(skip_node_address, &SkipNode_Hook, 4);
    
    // patch BioEvtSysTrackVOElements so it won't call SkipNode
    void *bio_event_vo_jb_address = (void *)(0x00AA23BF);
    char *jmp8_opcode = "\xEB";
    WriteMemory(bio_event_vo_jb_address, jmp8_opcode, 1);
    */
  }

  return TRUE;
}
