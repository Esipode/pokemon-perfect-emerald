#ifndef GUARD_START_MENU_H
#define GUARD_START_MENU_H

// Reorderable start menu items, in default order. Stored in SaveBlock2.startMenuOrder;
// append only, never reorder or remove.
enum StartMenuOrderItem
{
    START_MENU_ITEM_POKEMON,
    START_MENU_ITEM_POKEDEX,
    START_MENU_ITEM_DEXNAV,
    START_MENU_ITEM_BAG,
    START_MENU_ITEM_MAP,
    START_MENU_ITEM_PLAYER,
    START_MENU_ITEM_SAVE,
    START_MENU_ITEM_CHANGE_TIME,
    START_MENU_ITEM_OPTION,
    START_MENU_ITEM_ACHIEVEMENTS,
    START_MENU_ITEM_NEW_GAME_PLUS,
    START_MENU_ITEM_COUNT,
};

extern bool8 (*gMenuCallback)(void);

void ShowReturnToFieldStartMenu(void);
void Task_ShowStartMenu(u8 taskId);
void ShowStartMenu(void);
void ShowBattlePyramidStartMenu(void);
void SaveGame(void);
void AutosaveGame(void);
void CB2_SetUpSaveAfterLinkBattle(void);
void SaveForBattleTowerLink(void);
void HideStartMenu(void);
void AppendToList(u8 *list, u8 *pos, u8 newEntry);
void StartMenuOrder_Normalize(const u8 *saved, u8 *order);
void StartMenuOrder_Swap(u8 *order, u8 itemA, u8 itemB);
void StartMenuOrder_Load(u8 *order);
void StartMenuOrder_Store(const u8 *order);

#endif // GUARD_START_MENU_H
