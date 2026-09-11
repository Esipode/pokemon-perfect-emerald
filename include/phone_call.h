#ifndef GUARD_PHONE_CALL_H
#define GUARD_PHONE_CALL_H

// The sliding call window used by the pokenavcall script command.
bool32 IsPhoneCallTaskActive(void);
void StartPhoneCallFromScript(void);
void LoadPhoneCallWindowGfx(u32 windowId, u32 destOffset, u32 paletteId);
void DrawPhoneCallTextBoxBorder(u32 windowId, u32 tileOffset, u32 paletteId);
void RedrawPhoneCallTextBoxBorder(void);

#endif //GUARD_PHONE_CALL_H
