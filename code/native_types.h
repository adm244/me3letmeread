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

enum SeqAct_Interp_Flags {
  SeqAct_IsRunning = 0x2,
  SeqAct_IsPaused = 0x4,
  SeqAct_IsLooped = 0x40,
  SeqAct_IsCountDown = 0x400,
  SeqAct_Patch_FirstFOVOPlayed = 0x20000000,
  SeqAct_Patch_WasFOVOPaused = 0x40000000,
  SeqAct_Patch_WasPaused = 0x80000000,
};

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

enum BioSeqAct_FaceOnlyVO_Flags {
  //FOVO_Ambient = 0x8000,
  FOVO_SubConversation = 0x800,
};

enum GameModes {
  GameMode_Default = 0,
  GameMode_Conversation = 7,
  GameMode_Cinematic = 8,
};

struct BioConversationManager;
struct BioConversationController;
struct BioWorldInfo;
struct BioSeqAct_FaceOnlyVO;
struct BioPlayerController;

typedef void (__thiscall *_BioConversationManager_UpdateConversation)(BioConversationManager *manager, r32 dt);
typedef void (__stdcall *_BioConversationController_UpdateConversation)(BioConversationController *controller, r32 dt);
typedef void (__thiscall *_BioConversationController_SkipNode)(BioConversationController *controller);
typedef bool (__thiscall *_BioConversationController_IsCurrentlyAmbient)(BioConversationController *controller);
typedef bool (__thiscall *_BioConversationController_NeedToDisplayReplies)(BioConversationController *controller);
typedef void (__thiscall *_BioWorldInfo_SetFOVOAsPlaying)(BioWorldInfo *worldInfo, BioSeqAct_FaceOnlyVO *fovo);
typedef BioPlayerController * (__thiscall *_BioWorldInfo_GetPlayerController)(BioWorldInfo *worldInfo);

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

struct StaticString {
  u32 unk00;
  u32 unk04;
  char text[0]; // 0x08, null-terminated
};
//NOTE(adm244) last member sizeof is 0, since it has 0 elements (sizeof(char *) * 0)
assert_size(StaticString, 0x08);

struct BioConversationEntryReply {
  u32 unk00;
  u32 unk04;
  u32 unk08;
  u32 index; // 0x0C
  u32 textRefId; // 0x10
  // 0x0 - Default, 0x5 - Investigate
  u32 category; // 0x14
};
assert_size(BioConversationEntryReply, 0x18);

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
  BioConversationEntryReply *replies; // 0x38
  u32 repliesCount; // 0x3C
  u32 unk40;
  void *speaker; // 0x44
  u32 speakersCount; // 0x48
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
  BioConversationManager *manager; // 0x204
  u32 currentEntryIndex; // 0x208
  u8 unk20C[0x224-0x20C];
  u32 currentReplyIndex; // 0x224
  void *seqAct_startconv; // 0x228
  u8 unk22C[0x254-0x22C];
  u32 topicFlags; // 0x254
  u8 unk258[0x2A0-0x258];
  u32 dialogFlags; // 0x2A0
  u32 unkFlags; // 0x2A4
};
assert_size(BioConversationController, 0x2A8);

struct BioConversationManager {
  BioConversationManagerVTable *vtable; // 0x0
  u8 unk04[0x90-0x4];
  BioConversationController **controllers; // 0x90
  u32 controllersCount; // 0x94
  u8 unk98[0xAC-0x98];
};
assert_size(BioConversationManager, 0xAC);

struct BioWorldInfoVTable {
  u8 unk00[0x444];
  _BioWorldInfo_GetPlayerController GetPlayerController; // 0x444
  u8 unk448[0x458-0x448];
  _BioWorldInfo_SetFOVOAsPlaying SetFOVOAsPlaying; // 0x458
  u8 unk45C[0x468-0x45C];
};
assert_size(BioWorldInfoVTable, 0x468);

struct BioSubtitlesTextInfo {
  BioString text; // 0x0
  u32 unk0C;
  float duration; // 0x10
};

struct BioSubtitles {
  void *vtable; // 0x0
  u8 unk04[0x3C-0x04];
  BioSubtitlesTextInfo *textInfo; // 0x3C
  u32 unk40;
  u32 unk44;
  u32 unk48;
  u32 visible; // 0x4C
};

struct BioGameModeBase {
  void *vtable; // 0x0
  u8 unk04[0x2C-0x04];
  StaticString *name; // 0x2C
  u8 unk30[0x98-0x30];
  u32 flags; // 0x98
  u32 unk9C;
};
assert_size(BioGameModeBase, 0xA0);

struct SFXGameModeManager {
  void *vtable; // 0x0
  u8 unk04[0x48-0x04];
  BioGameModeBase **gameModes; // 0x48
  u32 gameModesCount; // 0x4C
  u8 unk50[0x70-0x50];
  u16 currentGameMode; // 0x70
  u16 unk72;
};
assert_size(SFXGameModeManager, 0x74);

struct BioPlayerController {
  void *vtable; // 0x0
  u8 unk04[0xA90-0x04];
  SFXGameModeManager *gameModeManager; // 0xA90
  u8 unkA94[0xAFC-0xA94];
};
assert_size(BioPlayerController, 0xAFC);

struct BioWorldInfo {
  BioWorldInfoVTable *vtable; // 0x0
  u8 unk04[0x648-0x04];
  u32 flags; // 0x648
  u8 unk64C[0x758-0x64C];
  BioPlayerController *playerController; // 0x758
  u8 unk75C[0x7A8-0x75C];
  BioConversationManager *conversationManager; // 0x7A8
  u32 unk7AC;
  u32 unk7B0;
  BioSubtitles *subtitles; // 0x7B4
  u8 unk7B8[0x7D4-0x7B8];
};
assert_size(BioWorldInfo, 0x7D4);

struct LevelUnk {
  BioWorldInfo *worldInfo; // 0x0
};

struct Sequence {
  void *vtable; // 0x0
  u8 unk04[0x140-0x04];
};
assert_size(Sequence, 0x140);

struct InterpData {
  void *vtable; // 0x0
  u8 unk04[0x28-0x04];
  Sequence *sequence; // 0x28
  u8 unk2C[0x74-0x2C];
  r32 duration; // 0x74
  u8 unk78[0x94-0x78];
};
assert_size(InterpData, 0x94);

struct BioTrackKey {
  StaticString *name;
  u32 unk04;
  r32 time; // 0x08
};
assert_size(BioTrackKey, 0x0C);

struct BioInterpTrack {
  void *vtable; // 0x0
  u8 unk04[0x5C-0x04];
  BioTrackKey *trackKeys; // 0x5C
  u32 trackKeysCount; // 0x60
};

struct BioEvtSysTrackVOElements {
  BioInterpTrack interpTrack; // 0x0
  u32 unk64;
  u32 unk68;
  u32 unk6C;
  u32 textRefId; // 0x70
};

struct SFXInterpTrackPlayFaceOnlyVO {
  BioInterpTrack interpTrack; // 0x0
};

struct InterpGroup {
  void *vtable; // 0x0
  u8 unk04[0x28-0x04];
  InterpData *interpData; // 0x28
  u32 unk2C;
  u32 unk30;
  u32 unk34;
  u32 unk38;
  u32 unk3C;
  BioInterpTrack **tracks; // 0x40
  u32 tracksCount; // 0x44
};

struct InterpGroupInst {
  void *vtable; // 0x0
  u8 unk04[0x48-0x04];
  InterpGroup *group; // 0x48
};

struct SeqAct_Interp {
  void *vtable; // 0x0
  u8 unk04[0xD8-0x04];
  InterpGroupInst **groupInsts; // 0xD8
  u32 groupInstsCount; // 0xDC
  u8 unkE0[0xF4-0xE0];
  r32 speedMultiplier; // 0xF4
  r32 currentTime; // 0xF8
  u32 unkFC;
  InterpData *interpData; // 0x100
  void *matineeActor; // 0x104
  u32 unk108;
  u32 unk10C;
  u32 unk110;
  u32 flags; // 0x114
};

struct Level {
  void *vtable; // 0x0
  u8 unk04[0x3C-0x04];
  LevelUnk *unk; // 0x3C
  u8 unk40[0xA0-0x40];
  Sequence *sequences; // 0xA0
  u32 sequencesCount; // 0xA4
};

struct World {
  void *vtable; // 0x0
  u8 unk04[0x54-0x04];
  Level *level; // 0x54
  u32 unk58;
  Level *level2; // 0x5C (same as level at 0x54)
  u8 unk60[0x194-0x60];
  u32 cleanup; // 0x194
};

struct BioSeqAct_FaceOnlyVO {
  void *vtable; // 0x0
  u8 unk04[0x10C-0x04];
  BioString text; // 0x10C
  u8 unk118[0x150-0x118];
  u32 flags; // 0x150
  u32 unk154;
};
assert_size(BioSeqAct_FaceOnlyVO, 0x158);

/*
struct BioConversationEntryReply {
  u32 index; // 0x0
  u32 textRefId; // 0x4
  u32 unk8;
  u32 unkC;
  u32 unk10;
  // 0x0 - Default, 0x5 - Investigate
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
