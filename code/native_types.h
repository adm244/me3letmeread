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

#ifndef _NATIVE_TYPES_H_
#define _NATIVE_TYPES_H_

#define assert_size(obj, size) static_assert(sizeof(obj) == size, "Size of " #obj " should be " #size)

enum BioConversation_DialogFlags {
  Dialog_Ambient = 0x80,
};

enum BioConversation_TopicFlags {
  Topic_IsVoicePlaying = 0x10,
  Topic_Patch_ManualSkip = 0x40000000,
  Topic_Patch_DialogWheelActive = 0x80000000,
};

/*
enum BioConversationEntry_Flags {
  Entry_NonTextline = 0x2,
};*/

enum BioWorldInfo_Flags {
  WorldInfo_IsPaused = 0x400,
};

struct BioConversationManager;
struct BioConversationController;

typedef void (__stdcall *_BioConversationManager_UpdateConversation)(BioConversationManager *manager, r32 dt);
typedef void (__stdcall *_BioConversationController_UpdateConversation)(BioConversationController *controller, r32 dt);
typedef void (__thiscall *_BioConversationController_SkipNode)(BioConversationController *controller);
typedef bool (__thiscall *_BioConversationController_IsCurrentlyAmbient)(BioConversationController *controller);
typedef bool (__thiscall *_BioConversationController_NeedToDisplayReplies)(BioConversationController *controller);

/*
  Text is in UTF-16 encoding
  BioSubtitles::AddSubtitle(BioSubtitles *ptr, BioString *text, BioConversationController *controller, BioPawn *actor, u32 unk04, u32 unk05, u32 unk06, u32 unk07, float unk08, float unk09);
*/

#pragma pack(1)

struct BioString {
  //NOTE(adm244): text is in UTF-16 encoding
  u16 *text; // 0x0
  u32 length; // 0x4
  u32 capacity; //??? 0x8
}; // 0xC
assert_size(BioString, 0xC);

struct BioConversationEntry {
  u32 unk00;
  u32 unk04;
  u32 unk08;
  u32 textRefId; // 0x0C
  u32 conditionalFunc; // 0x10
  u32 unk14;
  u32 unk18;
  u32 unk1C;
  u32 exportId; // 0x20
  u32 unk24;
  u32 unk28;
  u32 cameraIntimacy; // 0x2C
  u32 unk30;
  u32 unk34;
  void *replyList; // 0x38
  u32 unk3C;
  u32 unk40;
  void *speakerList; // 0x44
  u32 unk48;
  u32 unk4C;
  u32 speakerIndex; // 0x50
  u32 listenerIndex; // 0x54
  u32 skippable; // 0x58
}; // 0x5C
assert_size(BioConversationEntry, 0x5C);

struct BioConversationReply {
  u32 unk00;
  u32 unk04;
  u32 unk08;
  u32 textRefId; // 0x0C
  u8 unk10[0x50-0x10];
};
assert_size(BioConversationReply, 0x50);

struct BioConversationVTable {
  u8 unk00[0x158];
};
assert_size(BioConversationVTable, 0x158);

struct BioConversation {
  BioConversationVTable *vtable; // 0x0
  u8 unk04[0x84-0x4];
  BioConversationEntry *entriesList; // 0x84
  u32 entriesCount; // 0x88
  u32 unk8C;
  BioConversationReply *repliesList; // 0x90
  u32 repliesCount; // 0x94
  u8 unk98[0x108-0x98];
};
assert_size(BioConversation, 0x108);

struct BioConversationManagerVTable {
  u8 unk0[0x178];
  _BioConversationManager_UpdateConversation UpdateConversation; // 0x178
};
assert_size(BioConversationManagerVTable, 0x17C);

struct BioConversationManager {
  BioConversationManagerVTable *vtable; // 0x0
  u8 unk04[0xAC-0x4];
};
assert_size(BioConversationManager, 0xAC);

struct BioConversationControllerVTable {
  u8 unk0[0x15C];
  _BioConversationController_SkipNode SkipNode; // 0x15C
  _BioConversationController_IsCurrentlyAmbient IsCurrentlyAmbient; // 0x160
  void *GetReplyParaphraseText;
  void *unk168;
  _BioConversationController_NeedToDisplayReplies NeedToDisplayReplies; // 0x16C
  void *unk170;
  void *unk174;
  void *unk178;
  void *unk17C;
};
assert_size(BioConversationControllerVTable, 0x180);

struct BioConversationController {
  BioConversationControllerVTable *vtable; // 0x0
  u8 unk04[0x200-0x4];
  BioConversation *conversation; // 0x200
  void *unk204;
  u32 currentEntryIndex; // 0x208
  u8 unk20C[0x224-0x20C];
  u32 currentReplyIndex; // 0x224
  u8 unk228[0x254-0x228];
  u32 topicFlags; // 0x254
  u8 unk258[0x2A0-0x258];
  u32 dialogFlags; // 0x2A0
  u32 unkFlags; // 0x2A4
};
assert_size(BioConversationController, 0x2A8);

struct BioWorldInfo {
  void *vtable; // 0x0
  u8 unk04[0x648-0x04];
  u32 flags; // 0x648
  u8 unk64C[0x7D4-0x64C];
};
assert_size(BioWorldInfo, 0x7D4);

/*
struct BioConversationEntryReply {
  u32 index; // 0x0
  u32 textRefId; // 0x4
  u32 unk8;
  u32 unkC;
  u32 unk10;
  // 0x0 - Defualt, 0x5 - Investigate
  u32 category; // 0x14
}; // 0x18
assert_size(BioConversationEntryReply, 0x18);

//NOTE(adm244): same as BioConversationReply, but with a BioString appended?
struct BioConversationEntry {
  u32 textRefId; // 0x0
  BioString text; // 0x4
  u8 unk10[0x24-0x10];
  u32 exportId; // 0x24
  u32 unk28;
  u32 flags; // 0x2C
  u8 unk30[0x44-0x30];
  BioConversationEntryReply *replyList; // 0x44
  u8 unk48[0x58-0x48];
  i32 skippable; // 0x58
  BioString unk5C;
}; // 0x68
assert_size(BioConversationEntry, 0x68);

struct BioConversation {
  void *vtable;
  u8 unk04[0x48 - 0x4];
  BioConversationEntry *entryList; // 0x48
  u32 unk4C;
  u32 unk50;
  BioConversationReply *replyList; // 0x54
  u8 unk58[0x8C - 0x58];
  i32 currentEntryIndex; // 0x8C
  u32 unk90;
  void *unk94;
  i32 repliesCount; // 0x98
  u8 unk9C[0xC0 - 0x9C];
  i32 currentReplyIndex; // 0xC0
  u32 unkC4;
  u32 unkC8;
  u32 dialogFlags; // 0xCC
  u8 unkD0[0x1A8 - 0xD0];
  u32 topicFlags; // 0x1A8
  u8 unk1AC[0x238 - 0x1AC];
}; // 0x238?
assert_size(BioConversation, 0x238);
*/
#pragma pack(pop)

/*typedef bool (__thiscall *_BioConversation_NeedToDisplayReplies)(BioConversation *conversation);
typedef bool (__thiscall *_BioConversation_IsAmbient)(BioConversation *conversation);
typedef BioString * (__fastcall *_BioConversation_GetReplyText_Internal)
(int unk, int replyIndex, BioConversation *conversation, BioString *dest);*/

#endif
