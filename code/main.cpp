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

//internal _BioConversation_NeedToDisplayReplies BioConversation_NeedToDisplayReplies = 0;
//internal _BioConversation_IsAmbient BioConversation_IsAmbient = 0;
//internal _BioConversation_GetReplyText_Internal BioConversation_GetReplyText_Internal = (_BioConversation_GetReplyText_Internal)0x00B32440;

internal bool PauseWorld = false;

internal BioWorldInfo * GetBioWorldInfo()
{
  World *world = *(World **)0x1AA0D20;
  Level *level = world->level;
  return level->unk->worldInfo;
}

internal bool ShouldReply(BioConversationController *conversation)
{
  return (conversation->topicFlags & Topic_Patch_DialogWheelActive);
}

internal bool __cdecl IsSkipped(BioConversationController *controller)
{
  /*//NOTE(adm244): fixes infinite-loading bug
  BioConversationEntry entry = conversation->entryList[conversation->currentEntryIndex];
  if (entry.flags & Entry_NonTextline) {
    return true;
  }
  */
  
  /*
  //NOTE(adm244): skipes "empty" replies
  if (conversation->currentReplyIndex >= 0) {
    BioConversationEntryReply entryReply = entry.replyList[conversation->currentReplyIndex];
  
    BioString replyText = {0};
    BioConversationController_GetReplyText_Internal(1, entryReply.index, conversation, &replyText);
    
    //NOTE(adm244): probably should check if string contains only whitespaces or empty
    if (replyText.length < 3) {
      return true;
    }
  }
  */
  
  if (!controller->vtable->IsCurrentlyAmbient(controller) && !ShouldReply(controller)) {
    bool isSkipped = (controller->topicFlags & Topic_Patch_ManualSkip);
    controller->topicFlags &= ~Topic_Patch_ManualSkip;
    
    if (!isSkipped && !PauseWorld && !(controller->topicFlags & Topic_IsVoicePlaying)) {
      PauseWorld = true;
      
      BioWorldInfo *worldInfo = GetBioWorldInfo();
      worldInfo->flags |= WorldInfo_IsPaused;
    }
    
    return isSkipped;
  }
  
  return true;
}

internal bool FOVOPlaying = false;

internal bool __cdecl IsSkippedPreFOVO(BioSeqAct_FaceOnlyVO *fovo, BioWorldInfo *worldInfo)
{
  if (!PauseWorld && !FOVOPlaying && (fovo->flags & FOVO_SubConversation)) {
    PauseWorld = true;
  
    //BioWorldInfo *worldInfo = GetBioWorldInfo();
    worldInfo->flags |= WorldInfo_IsPaused;
    
    FOVOPlaying = true;
    
    worldInfo->vtable->SetFOVOAsPlaying(worldInfo, fovo);
  
    return false;
  }
  
  return true;
}

internal bool __cdecl SkipNode(BioConversationController *controller)
{
  //FIX(adm244): cannot skip a topic after selecting a reply (with spacebar)
  // for some reason it doesn't update currentReplyIndex
  /*if ((conversation->repliesCount > 1) && (conversation->currentReplyIndex < 0)) {
    return;
  }*/
  
  BioConversationEntry entry = controller->conversation->entriesList[controller->currentEntryIndex];
  if (!entry.skippable && (controller->topicFlags & Topic_IsVoicePlaying)
    && (controller->currentReplyIndex < 0)) {
    return false;
  }
  
  //FIX(adm244): don't set skip flag if entry has more than one reply
  //TODO(adm244): get replyList length
  
  if ((controller->topicFlags & Topic_IsVoicePlaying)
    || !(controller->topicFlags & Topic_Patch_DialogWheelActive)) {
    controller->topicFlags |= Topic_Patch_ManualSkip;
  }
  
  if (PauseWorld) {
    PauseWorld = false;
    
    //FIX(adm244): animations seems to get reset on resume (just set\clear some flag?)
    
    BioWorldInfo *worldInfo = GetBioWorldInfo();
    worldInfo->flags &= ~WorldInfo_IsPaused;
  }
  
  if (FOVOPlaying) {
    FOVOPlaying = false;
    return false;
  }
  
  return true;
}

/*internal void __cdecl WorldTick(BioWorldInfo *world)
{
  if (PauseWorld) {
    if(!(world->flags & WorldInfo_IsPaused)) {
      world->flags |= WorldInfo_IsPaused;
    }
  } else {
    if(world->flags & WorldInfo_IsPaused) {
      world->flags &= ~WorldInfo_IsPaused;
    }
  }
}*/

internal void *skip_jz_dest_address = 0;
//internal void *skip_jnz_dest_address = 0;
internal void *skip_post_jnz_address = 0;

internal void *skip_node_post_address = 0;
//internal void *world_tick_post_address = 0;

internal void *fovo_jz_dest_address = 0;
internal void *fovo_post_address = 0;

internal __declspec(naked) void IsSkipped_Hook()
{
  __asm {
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
    cmp [ebp-10], edi
    je jz_jump
    jmp [skip_post_jnz_address]
  
  jz_jump:
    jmp [skip_jz_dest_address]
  }
}

internal __declspec(naked) void IsSkippedPreFOVO_Hook()
{
  __asm {
    push ebx
    push esi
    push edi
    push ebp
    push esp
    
    push edi
    push ebx
    call IsSkippedPreFOVO
    add esp, 8
    
    pop esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    test al, al
    jz dont_skip
    jmp skip
    
  dont_skip:
    jmp [fovo_jz_dest_address]
    
  skip:
    mov eax, [ebx+54h]
    test [eax+1Ch], 1
    jmp [fovo_post_address]
  }
}

internal __declspec(naked) void SkipNode_Hook()
{
  __asm {
    push eax
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
    
    test al, al
    pop eax
    jz dont_skip
    
    push ebx
    push esi
    push edi
    mov edi, ecx
    
    jmp [skip_node_post_address]
    
  dont_skip:
    mov eax, ecx
    retn
  }
}

internal __declspec(naked) void RepliesActive_Hook()
{
  //NOTE(adm244): is there a way to shove an 'offsetof' constant into inline asm?
  __asm {
    mov eax, Topic_Patch_DialogWheelActive
    or [esi+254h], eax
    
    mov eax, 1
    pop esi
    retn
  }
}

internal __declspec(naked) void RepliesInactive_Hook()
{
  __asm {
    mov eax, Topic_Patch_DialogWheelActive
    not eax
    and [esi+254h], eax
    
    xor eax, eax
    pop esi
    retn
  }
}

/*internal __declspec(naked) void WorldTick_Hook()
{
  __asm {
    push ebx
    push esi
    push edi
    push ebp
    push esp
    
    push esi
    call WorldTick
    pop esi
    
    pop esp
    pop ebp
    pop edi
    pop esi
    pop ebx
    
    push ebp
    mov ebp, esp
    push ecx
    mov eax, [esi]
    
    jmp [world_tick_post_address]
  }
}*/

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

internal void PatchUpdateConversation(BioConversationManagerVTable *vtable)
{
  void *BioConversationController_UpdateConversation = RIPRel32((u8 *)(*vtable->UpdateConversation) + 0x33C, 1);
  
  assert((u32)BioConversationController_UpdateConversation == 0x00C43E40);
  
  void *jz_ptr = (u8 *)BioConversationController_UpdateConversation + 0x4CC;
  void *jz_dest_ptr = RIPRel32(jz_ptr, 2);
  void *jz_post_ptr = (u8 *)jz_ptr + 0x6;
  
  assert((u32)jz_ptr == 0x00C4430C);
  assert((u32)jz_dest_ptr == 0x00C44397);
  assert((u32)jz_post_ptr == 0x00C44312);
  
  void *patch_ptr = (u8 *)jz_ptr - 0x3;
  
  assert((u32)patch_ptr == 0x00C44309);
  
  skip_jz_dest_address = jz_dest_ptr;
  skip_post_jnz_address = jz_post_ptr;
  
  WriteDetour(patch_ptr, &IsSkipped_Hook, 4);
}

internal void PatchSkipNode(BioConversationControllerVTable *vtable)
{
  void *patch_ptr = (u8 *)(*vtable->SkipNode) + 0x5;
  
  skip_node_post_address = (u8 *)patch_ptr + 0x5;
  
  assert((u32)patch_ptr == 0x00C44B25);
  assert((u32)skip_node_post_address == 0x00C44B2A);
  
  WriteDetour(patch_ptr, &SkipNode_Hook, 0);
}

internal void PatchNeedToDisplayReplies(BioConversationControllerVTable *vtable)
{
  void *patch_active_ptr = (u8 *)(*vtable->NeedToDisplayReplies) + 0x4B;
  void *patch_inactive_ptr = (u8 *)(*vtable->NeedToDisplayReplies) + 0x7B;
  
  assert((u32)patch_active_ptr == 0x00C480FB);
  assert((u32)patch_inactive_ptr == 0x00C4812B);
  
  WriteDetour(patch_active_ptr, &RepliesActive_Hook, 2);
  WriteDetour(patch_inactive_ptr, &RepliesInactive_Hook, 0);
}

internal void PatchFOVOUpdateConversation(MODULEINFO *baseModuleInfo)
{
  void *patch_ptr = (void *)FindSignature(baseModuleInfo,
    "\x8B\x17\x8B\x82\x50\x04\x00\x00\x53\x8B\xCF\xFF\xD0",
    "xxxxxxxxxxxxx", 0x15);
  
  assert((u32)patch_ptr == 0x00DC7FC8);
  
  void *jz_dest_ptr = RIPRel32((u8 *)patch_ptr - 0x4, 0);
  
  assert((u32)jz_dest_ptr == 0x00DC8283);
  
  //fovo_jz_dest_address = jz_dest_ptr;
  fovo_jz_dest_address = (void *)0x00DC8268;
  fovo_post_address = (u8 *)patch_ptr + 0x7;
  
  WriteDetour(patch_ptr, &IsSkippedPreFOVO_Hook, 2);
}

/*internal void PatchBioWorldInfoTickLocal(MODULEINFO *baseModuleInfo)
{
  void *patch_ptr = (void *)FindSignature(baseModuleInfo,
    "\x8B\x8E\xA8\x07\x00\x00\xD9\x45\x08\x8B\x01\x8B\x90\x78\x01\x00\x00",
    "xxxxxxxxxxxxxxxxx", -0x28);
  
  assert((u32)patch_ptr == 0x00CC97E0);
  
  world_tick_post_address = (u8 *)patch_ptr + 0x6;
  
  WriteDetour(patch_ptr, &WorldTick_Hook, 1);
}*/

internal void PatchIsVOOverCheck(MODULEINFO *baseModuleInfo)
{
  void *patch_ptr = (void *)FindSignature(baseModuleInfo,
    "\xF3\x0F\x5C\xC1\x0F\x2F\xD8\xF3\x0F\x11\x40\x10\x76\x1F",
    "xxxxxxxxxxxxxx", 12);
  
  assert((u32)patch_ptr == 0x00CBE877);
  
  char *jmp8_opcode = "\xEB";
  WriteMemory(patch_ptr, jmp8_opcode, 1);
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
  BioConversationManagerVTable *BioConversationManager_vtable = (BioConversationManagerVTable *)GetBioConversationManagerVTable(&baseModuleInfo, beginAddress, endAddress);
  BioConversationControllerVTable *BioConversationController_vtable = (BioConversationControllerVTable *)GetBioConversationControllerVTable(&baseModuleInfo, beginAddress, endAddress);
  
  assert((u32)BioConversationManager_vtable == 0x01804E98);
  assert((u32)BioConversationController_vtable == 0x01803F50);
  
  PatchUpdateConversation(BioConversationManager_vtable);
  PatchSkipNode(BioConversationController_vtable);
  PatchNeedToDisplayReplies(BioConversationController_vtable);
  PatchFOVOUpdateConversation(&baseModuleInfo);
  
  //NOTE(adm244): allows us to pause world updates
  //PatchBioWorldInfoTickLocal(&baseModuleInfo);
  
  //NOTE(adm244): fixes subtitles disappearing after VO is finished
  PatchIsVOOverCheck(&baseModuleInfo);
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
