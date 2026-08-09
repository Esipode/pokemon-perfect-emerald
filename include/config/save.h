#ifndef GUARD_CONFIG_SAVE_H
#define GUARD_CONFIG_SAVE_H

// Menu configs
#define SKIP_SAVE_CONFIRMATION              FALSE   // If TRUE, skips the "There is already a saved file" confirmation when overwriting a save.

// SaveBlock1 configs
#define FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK1    TRUE   // Free up unused Pokédex seen flags (52 bytes).
#define FREE_TRAINER_HILL                   TRUE   // Frees up Trainer Hill data (28 bytes).
#define FREE_TRAINER_TOWER                  TRUE   // Frees up Trainer Tower data (x bytes).
#define FREE_MYSTERY_EVENT_BUFFERS          TRUE   // Frees up ramScript (1104 bytes).
#define FREE_MATCH_CALL                     TRUE   // Frees up match call and rematch / VS Seeker data. (104 bytes).
#define FREE_UNION_ROOM_CHAT                TRUE   // Frees up union room chat (212 bytes).
#define FREE_ENIGMA_BERRY                   TRUE   // Frees up E-Reader Enigma Berry data (52 bytes).
#define FREE_LINK_BATTLE_RECORDS            TRUE   // Frees up link battle record data (88 bytes).
#define FREE_MYSTERY_GIFT                   TRUE   // Frees up Mystery Gift data (876 bytes).
#define FREE_EASY_CHAT_PROFILE              TRUE   // Frees up easyChatProfile and easyChatBattleStart/Won/Lost (48 bytes).
#define FREE_DEWFORD_TRENDS                 TRUE   // Frees up Dewford Trend rumor data (40 bytes).
#define FREE_GABBY_AND_TY                   TRUE   // Frees up Gabby and Ty interview data (12 bytes).
#define FREE_OLD_MAN                        TRUE   // Frees up Mauville Old Man data (64 bytes).
#define FREE_LILYCOVE_LADY                  TRUE   // Frees up Lilycove Lady data (48 bytes).
#define FREE_SECRET_BASES                   TRUE   // Frees up Secret Base data (208 bytes).
#define FREE_EXTERNAL_EVENT_DATA            TRUE   // Frees up external (e-Reader) event data and flags (41 bytes).
#define FREE_RECORD_MIXING_GIFT             TRUE   // Frees up record mixing gift data (8 bytes).
#define FREE_CONTESTS                       TRUE   // Frees up contestWinners (256 bytes). Also drops the six contest-condition bytes from PokemonSubstruct2 and the contest ribbons from PokemonSubstruct3 -- 0 bytes on its own (substructs are union-padded to a common size), but a precondition for Stage 5's BoxPokemon shrink.
#define FREE_DECORATIONS                    TRUE   // Frees up playerRoomDecoration* and the 8 decoration* arrays (102 bytes).
#define FREE_MAIL                           TRUE   // Frees up mail[MAIL_COUNT] (272 bytes) and the embedded struct Mail in both DaycareMail slots (68 bytes).
#define FREE_POKEBLOCKS                     TRUE   // Frees up pokeblocks[POKEBLOCKS_COUNT] (70 bytes).
                                            // SaveBlock1 total: 3685 bytes
// SaveBlock2 configs
#define FREE_BATTLE_TOWER_E_READER          TRUE   // Frees up Battle Tower E-Reader data (200 bytes -- struct BattleTowerEReaderTrainer's stale /*0x..*/ offsets assume the vanilla 44-byte BattleTowerPokemon; it's 48 bytes here since `level` was widened to u16).
#define FREE_POKEMON_JUMP                   TRUE   // Frees up Pokémon Jump data (16 bytes).
#define FREE_RECORD_MIXING_HALL_RECORDS     TRUE   // Frees up hall records for record mixing (1032 bytes).
#define FREE_EXTRA_SEEN_FLAGS_SAVEBLOCK2    TRUE   // Free up unused Pokédex seen flags (104 bytes).
#define FREE_BATTLE_FRONTIER                TRUE   // Frees up struct BattleFrontier, apprentices[], and playerApprentice (2,528 bytes). Also retires battlePoints/cardBattlePoints and the Battle Tents (which reuse the same struct). disableRecordBattle and lvlMode are relocated to top-level SaveBlock2 fields first, since generic (non-frontier) battle/link code still needs them.
                                            // FREE_CONTESTS also frees contestLinkResults (40 bytes) here.
                                            // SaveBlock2 total: 3920 bytes

                                            // Grand Total: 7605

#endif // GUARD_CONFIG_SAVE_H
