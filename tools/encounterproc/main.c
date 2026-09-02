/* encounterproc
 * Parses the '.encounter' authoring format (Encounter / Properties / Trigger / Conditions blocks)
 * into the C initializers 'src/battle_encounter.c' expects: struct EncounterCondition, struct
 * EncounterTrigger, struct EncounterProperties, struct Encounter, gEncounters[]. See
 * include/battle_encounter.h and include/constants/battle_encounter.h for what those mean at runtime.
 *
 * Infrastructure (String/Token/Parser, the match_* primitives, show_parse_error) is copied from
 * tools/trainerproc/main.c, which solves the same "readable text -> C initializer, errors point
 * back to the authored source" problem. What's new here is indentation-sensitive parsing for
 * Conditions:/All:/Any:/Not: blocks - trainerproc's format has no nesting.
 *
 * Values on the right of a comparison (and stat names inside Stat(...)) are copied verbatim into
 * the generated C, unresolved - like Script:, this tool can't check whether e.g. MOVE_EARTHQUAKE
 * or B_WEATHER_RAIN_NORMAL exist; an unknown one is a clean compiler error. Only this tool's own
 * DSL keywords (checkpoints, flags, operand names, battler refs) are validated here.
 *
 * To add a new operand:
 * 1. Add the ENC_OP_* to constants/battle_encounter.h and handle it in GetEncounterOperand.
 * 2. Add its syntax to parse_operand below.
 */
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_TRIGGERS_PER_ENCOUNTER    32  // matches MAX_ENCOUNTER_TRIGGERS
#define MAX_VARS_PER_ENCOUNTER        16  // matches MAX_ENCOUNTER_VARS
#define MAX_COND_DEPTH                 4  // matches MAX_ENCOUNTER_COND_DEPTH
#define MAX_COND_LINES                256
#define MAX_ENCOUNTER_LEVEL          1000  // matches MAX_LEVEL (constants/pokemon.h)
#define MAX_DAMAGE_REDUCTION           99  // matches ENC_MAX_DAMAGE_REDUCTION
#define MAX_MOVES_PER_ENCOUNTER         4  // matches MAX_MON_MOVES (constants/global.h)
#define ARG_EXPR_N                    128
// 'Moves:' and 'AiFlags:' hold a whole list, not one value, so they need more room than the
// single-expression property fields ARG_EXPR_N was sized for.
#define LIST_EXPR_N                   512
#define COND_LINE_N                   256

struct String
{
    const unsigned char *string;
    int string_n;
};

static bool is_constant(struct String s, const char *prefix)
{
    for (int i = 0;; i++)
    {
        if (i == s.string_n)
            return false;
        else if (prefix[i] == '\0')
            return s.string[i] == '_';
        else if (prefix[i] != s.string[i])
            return false;
    }
}

struct Source
{
    const char *path;
    const unsigned char *buffer;
    int buffer_n;
};

struct SourceLocation
{
    int line;
    int column;
};

struct Token
{
    const struct Source *source;
    struct SourceLocation location;
    int begin;
    int end;
};

static bool is_empty_token(const struct Token *t)
{
    return t->begin == t->end;
}

static bool is_literal_token(const struct Token *t, const char *s)
{
    int i = t->begin;
    for (;;)
    {
        if (*s == '\0' && i == t->end)
            return true;
        else if (*s == '\0' || i == t->end)
            return false;
        else if (*s != t->source->buffer[i])
            return false;
        i++;
        s++;
    }
}

static bool tokens_equal(const struct Token *a, const struct Token *b)
{
    int an = a->end - a->begin, bn = b->end - b->begin;
    if (an != bn)
        return false;
    return memcmp(&a->source->buffer[a->begin], &b->source->buffer[b->begin], an) == 0;
}

static struct String token_string(const struct Token *t)
{
    return (struct String) {
        .string = &t->source->buffer[t->begin],
        .string_n = t->end - t->begin,
    };
}

struct Parser
{
    struct Source *source;
    struct SourceLocation location;
    int offset;
    struct SourceLocation error_location;
    const char *error;
    bool fatal_error;
};

static bool set_parse_error(struct Parser *p, struct SourceLocation location, const char *error)
{
    p->error = error;
    p->error_location = location;
    return false;
}

__attribute__((warn_unused_result))
static bool peek_char(struct Parser *p, unsigned char *c)
{
    assert(p && c);
    if (p->offset == p->source->buffer_n)
        return false;
    *c = p->source->buffer[p->offset];
    return true;
}

__attribute__((warn_unused_result))
static bool pop_char(struct Parser *p, unsigned char *c)
{
    assert(p && c);
    if (p->offset == p->source->buffer_n)
        return false;
    *c = p->source->buffer[p->offset++];
    if (*c == '\n')
    {
        p->location.line++;
        p->location.column = 1;
    }
    else
    {
        p->location.column++;
    }
    return true;
}

static void skip_whitespace(struct Parser *p)
{
    unsigned char c;
    for (;;)
    {
        if (!peek_char(p, &c))
            break;
        if (c != ' ' && c != '\t')
            break;
        if (!pop_char(p, &c))
            assert(false);
    }
}

static void skip_line(struct Parser *p)
{
    unsigned char c;
    for (;;)
    {
        if (!pop_char(p, &c))
            break;
        if (c == '\n')
            break;
    }
}

__attribute__((warn_unused_result))
static bool match_eof(struct Parser *p)
{
    return p->offset == p->source->buffer_n;
}

__attribute__((warn_unused_result))
static bool match_exact(struct Parser *p, const char *s)
{
    struct Parser p_ = *p;
    unsigned char c;

    for (; *s != '\0'; s++)
    {
        if (!pop_char(&p_, &c))
            return false;
        if (*s != c)
            return false;
    }

    *p = p_;
    return true;
}

static void match_until_eol(struct Parser *p, struct Token *t)
{
    unsigned char c;

    skip_whitespace(p);

    t->source = p->source;
    t->location = p->location;
    t->begin = p->offset;
    t->end = p->offset;

    for (;;)
    {
        if (!peek_char(p, &c))
            break;
        if (c == '\n' || c == '#')
            break;
        if (!pop_char(p, &c))
            assert(false);
        if (c != ' ' && c != '\t')
            t->end = p->offset;
    }
}

__attribute__((warn_unused_result))
static bool match_eol(struct Parser *p)
{
    struct Parser p_ = *p;
    unsigned char c;

    skip_whitespace(&p_);
    for (;;)
    {
        if (!pop_char(&p_, &c))
            return false;
        if (c == '\n')
            break;
        else if (c == '#')
        {
            skip_line(&p_);
            break;
        }
        else
            return false;
    }

    *p = p_;
    return true;
}

__attribute__((warn_unused_result))
static bool match_int(struct Parser *p, int *i)
{
    struct Parser p_ = *p;
    unsigned char c;

    *i = 0;
    for (;;)
    {
        if (!peek_char(&p_, &c))
            break;
        if (!('0' <= c && c <= '9'))
            break;
        *i = *i * 10 + (c - '0');
        if (!pop_char(&p_, &c))
            assert(false);
    }

    if (p->offset == p_.offset)
        return false;

    *p = p_;
    return true;
}

__attribute__((warn_unused_result))
// A blank line, or one holding only a '#' comment - match_eol skips a comment to end of line.
static bool match_empty_line(struct Parser *p)
{
    return match_eol(p);
}

// Strict C identifier: [A-Za-z_][A-Za-z0-9_]*. Encounter/Var/Script names all become C symbols,
// so unlike trainerproc's match_identifier (which allows "'" for Pokemon nicknames), this rejects it.
__attribute__((warn_unused_result))
static bool match_c_identifier(struct Parser *p, struct Token *t)
{
    struct Parser p_ = *p;
    unsigned char c;

    t->source = p->source;
    t->location = p->location;
    t->begin = p->offset;

    if (!peek_char(&p_, &c))
        return false;
    if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '_'))
        return false;

    for (;;)
    {
        if (!peek_char(&p_, &c))
            break;
        if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_')
            ;
        else
            break;
        if (!pop_char(&p_, &c))
            assert(false);
    }

    t->end = p_.offset;
    *p = p_;
    return true;
}

static bool show_parse_error(struct Parser *p)
{
    int n = fprintf(stderr, "%s:%d: ", p->source->path, p->error_location.line);
    fprintf(stderr, "error: %s\n", p->error);

    struct Parser p_ = {
        .source = p->source,
        .location = { .line = 1, .column = 1 },
        .offset = 0,
    };

    for (;;) {
        if (p->error_location.line == p_.location.line)
            break;
        if (!match_empty_line(&p_))
            skip_line(&p_);
        if (match_eof(&p_))
            assert(false);
    }

    int begin = p_.offset;
    int end;
    for (end = begin; end < p->source->buffer_n; end++)
    {
        if (p->source->buffer[end] == '\n')
            break;
    }

    fprintf(stderr, "%s:%d: %.*s\n", p->source->path, p->error_location.line, end - begin, &p->source->buffer[begin]);

    fprintf(stderr, "%*s", n, "");
    for (int column = 1; column < p->error_location.column && begin + column < end; column++)
    {
        unsigned char c = p->source->buffer[begin + column];
        fputc(c == '\t' ? c : ' ', stderr);
    }
    fprintf(stderr, "^\n");

    p->error = NULL;
    p->fatal_error = true;

    return false;
}

static bool set_show_parse_error(struct Parser *p, struct SourceLocation location, const char *error)
{
    set_parse_error(p, location, error);
    return show_parse_error(p);
}

// --- Parsed data model ------------------------------------------------------------------------

struct VarEntry
{
    struct Token name;
    int index;
};

struct Trigger
{
    const char *checkpoint_const;    // e.g. "ENC_ON_MOVE_END"
    int checkpoint_line;

    int priority;
    bool has_priority;

    char flags_expr[ARG_EXPR_N];     // e.g. "ENC_TRIGGER_ONCE | ENC_TRIGGER_ON_ENTER", or "0"

    struct Token script;
    bool has_script;

    bool has_conditions;
    char *cond_lines[MAX_COND_LINES];
    int cond_lines_n;

    int trigger_keyword_line;        // for the TESTING source-location table
};

// The 'Properties:' block, one field per struct EncounterProperties member (include/
// battle_encounter.h). Each holds the C expression to emit; parse_encounter seeds them with the
// "change nothing" defaults, so an encounter with no Properties: block emits an inert initializer.
struct Properties
{
    char level[ARG_EXPR_N];
    char catch_rate[ARG_EXPR_N];
    char ball_policy[ARG_EXPR_N];
    char damage_reduction[ARG_EXPR_N];
    char immunities[ARG_EXPR_N];
    char cap_type_effectiveness[ARG_EXPR_N];
    char flat_toxic_damage[ARG_EXPR_N];
    char survive[ARG_EXPR_N];
    char ability[ARG_EXPR_N];     // an ABILITY_* constant, passed through to the compiler
    char moves[LIST_EXPR_N];      // a brace initializer for the moves[] array, e.g. "{ MOVE_TACKLE }"
    char ai_flags[LIST_EXPR_N];   // an OR of AI_FLAG_* constants, passed through to the compiler
};

struct EncounterDef
{
    struct Token name;
    int name_line;

    struct Trigger triggers[MAX_TRIGGERS_PER_ENCOUNTER];
    int triggers_n;

    struct VarEntry vars[MAX_VARS_PER_ENCOUNTER];
    int vars_n;

    struct Properties properties;
    bool has_properties;
};

struct Parsed
{
    // Heap-allocated and grown, not embedded: each EncounterDef is tens of KB (32 triggers, each
    // with a 256-entry cond_lines array), so a fixed-size array of these here would put several
    // MB on the stack.
    struct EncounterDef *encounters;
    int encounters_n;
    int encounters_c;
};

static void append_cond_line(struct Trigger *trig, const char *text)
{
    if (trig->cond_lines_n == MAX_COND_LINES)
    {
        fprintf(stderr, "error: too many conditions in one trigger (max %d)\n", MAX_COND_LINES);
        exit(1);
    }
    trig->cond_lines[trig->cond_lines_n++] = strdup(text);
}

// Peeks past blank and comment lines (without consuming) and reports the indent (in columns,
// 0-based) of the next real content line. Returns false at EOF.
static bool peek_indent_after_blanks(struct Parser *p, int *indent_out)
{
    struct Parser tmp = *p;
    while (match_empty_line(&tmp)) {}
    if (match_eof(&tmp))
        return false;
    skip_whitespace(&tmp);
    *indent_out = tmp.location.column - 1;
    return true;
}

static bool find_or_add_var(struct Parser *p, struct EncounterDef *enc, struct Token *name, int *index_out)
{
    for (int i = 0; i < enc->vars_n; i++)
    {
        if (tokens_equal(&enc->vars[i].name, name))
        {
            *index_out = enc->vars[i].index;
            return true;
        }
    }
    if (enc->vars_n == MAX_VARS_PER_ENCOUNTER)
        return set_show_parse_error(p, name->location, "too many named variables in this encounter (max 16)");

    enc->vars[enc->vars_n].name = *name;
    enc->vars[enc->vars_n].index = enc->vars_n;
    *index_out = enc->vars_n;
    enc->vars_n++;
    return true;
}

static bool battler_ref_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "Boss"))          { *out = "ENC_BOSS";           return true; }
    if (is_literal_token(t, "Self"))          { *out = "ENC_SELF";           return true; }
    if (is_literal_token(t, "PlayerLeft"))    { *out = "ENC_PLAYER_LEFT";    return true; }
    if (is_literal_token(t, "PlayerRight"))   { *out = "ENC_PLAYER_RIGHT";   return true; }
    if (is_literal_token(t, "OpponentLeft"))  { *out = "ENC_OPPONENT_LEFT";  return true; }
    if (is_literal_token(t, "OpponentRight")) { *out = "ENC_OPPONENT_RIGHT"; return true; }
    return false;
}

static bool checkpoint_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "OnBattleStart")) { *out = "ENC_ON_BATTLE_START"; return true; }
    if (is_literal_token(t, "OnTurnStart"))   { *out = "ENC_ON_TURN_START";   return true; }
    if (is_literal_token(t, "OnMoveEnd"))     { *out = "ENC_ON_MOVE_END";     return true; }
    if (is_literal_token(t, "OnFaint"))       { *out = "ENC_ON_FAINT";        return true; }
    if (is_literal_token(t, "OnSwitchIn"))    { *out = "ENC_ON_SWITCH_IN";    return true; }
    if (is_literal_token(t, "OnTurnEnd"))     { *out = "ENC_ON_TURN_END";     return true; }
    if (is_literal_token(t, "OnBattleEnd"))   { *out = "ENC_ON_BATTLE_END";   return true; }
    return false;
}

static bool flag_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "Once"))    { *out = "ENC_TRIGGER_ONCE";     return true; }
    if (is_literal_token(t, "OnEnter")) { *out = "ENC_TRIGGER_ON_ENTER"; return true; }
    return false;
}

static bool ball_policy_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "Default")) { *out = "ENC_BALLS_DEFAULT"; return true; }
    if (is_literal_token(t, "Blocked")) { *out = "ENC_BALLS_BLOCKED"; return true; }
    if (is_literal_token(t, "Allowed")) { *out = "ENC_BALLS_ALLOWED"; return true; }
    return false;
}

static bool bool_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "True"))  { *out = "TRUE";  return true; }
    if (is_literal_token(t, "False")) { *out = "FALSE"; return true; }
    return false;
}

static bool immunity_const(const struct Token *t, const char **out)
{
    if (is_literal_token(t, "None"))        { *out = "0";                       return true; }
    if (is_literal_token(t, "Ohko"))        { *out = "ENC_IMMUNE_OHKO";         return true; }
    if (is_literal_token(t, "FixedDamage")) { *out = "ENC_IMMUNE_FIXED_DAMAGE"; return true; }
    if (is_literal_token(t, "HpSwap"))      { *out = "ENC_IMMUNE_HP_SWAP";      return true; }
    if (is_literal_token(t, "SharedKo"))    { *out = "ENC_IMMUNE_SHARED_KO";    return true; }
    if (is_literal_token(t, "All"))         { *out = "ENC_IMMUNE_ALL";          return true; }
    return false;
}

// Appends a token's raw source text to a bounded C string.
static void append_token_text(char *buf, size_t buf_n, const struct Token *t)
{
    size_t len = strlen(buf);
    int n = t->end - t->begin;
    if (len + 1 >= buf_n)
        return;
    if ((size_t)n > buf_n - len - 1)
        n = buf_n - len - 1;
    memcpy(buf + len, &t->source->buffer[t->begin], n);
    buf[len + n] = '\0';
}

// Parses one operand (the left-hand side of a condition) into an ENC_OP_* constant and its
// '.arg' expression. See the module comment for the "values pass through verbatim" rule.
__attribute__((warn_unused_result))
static bool parse_operand(struct Parser *p, struct EncounterDef *enc, const char **operand_const, char *arg_expr)
{
    struct Parser p_ = *p;
    struct Token id;

    if (match_exact(&p_, "Battler"))
    {
        skip_whitespace(&p_);
        if (!match_exact(&p_, "("))
            return set_parse_error(p, p_.location, "expected '(' after 'Battler'");
        skip_whitespace(&p_);
        struct Token ref;
        if (!match_c_identifier(&p_, &ref))
            return set_parse_error(p, p_.location, "expected a battler reference");
        const char *ref_const;
        if (!battler_ref_const(&ref, &ref_const))
            return set_parse_error(p, ref.location, "unknown battler reference (expected Boss, Self, PlayerLeft, PlayerRight, OpponentLeft, or OpponentRight)");
        skip_whitespace(&p_);
        if (!match_exact(&p_, ")"))
            return set_parse_error(p, p_.location, "expected ')'");
        skip_whitespace(&p_);
        if (!match_exact(&p_, "."))
            return set_parse_error(p, p_.location, "expected '.' after 'Battler(...)'");
        skip_whitespace(&p_);
        struct Token field;
        if (!match_c_identifier(&p_, &field))
            return set_parse_error(p, p_.location, "expected an operand field after 'Battler(...).'");

        if (is_literal_token(&field, "HP"))
        {
            *operand_const = "ENC_OP_HP";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "HpPercent"))
        {
            *operand_const = "ENC_OP_HP_PERCENT";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "MaxHp"))
        {
            *operand_const = "ENC_OP_MAX_HP";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "Species"))
        {
            *operand_const = "ENC_OP_SPECIES";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "Ability"))
        {
            *operand_const = "ENC_OP_ABILITY";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "Status"))
        {
            *operand_const = "ENC_OP_STATUS";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "Type"))
        {
            *operand_const = "ENC_OP_TYPE";
            snprintf(arg_expr, ARG_EXPR_N, "%s", ref_const);
        }
        else if (is_literal_token(&field, "Stat"))
        {
            skip_whitespace(&p_);
            if (!match_exact(&p_, "("))
                return set_parse_error(p, p_.location, "expected '(' after 'Stat'");
            skip_whitespace(&p_);
            struct Token stat;
            if (!match_c_identifier(&p_, &stat))
                return set_parse_error(p, p_.location, "expected a stat name");
            skip_whitespace(&p_);
            if (!match_exact(&p_, ")"))
                return set_parse_error(p, p_.location, "expected ')'");

            *operand_const = "ENC_OP_STAT_STAGE";
            snprintf(arg_expr, ARG_EXPR_N, "ENC_PACK_STAT_ARG(%s, ", ref_const);
            append_token_text(arg_expr, ARG_EXPR_N, &stat);
            strncat(arg_expr, ")", ARG_EXPR_N - strlen(arg_expr) - 1);
        }
        else
        {
            return set_parse_error(p, field.location, "unknown operand field (expected HP, HpPercent, MaxHp, Species, Ability, Status, Stat, or Type)");
        }

        *p = p_;
        return true;
    }

    if (match_exact(&p_, "Var"))
    {
        skip_whitespace(&p_);
        if (!match_exact(&p_, "("))
            return set_parse_error(p, p_.location, "expected '(' after 'Var'");
        skip_whitespace(&p_);
        struct Token name;
        if (!match_c_identifier(&p_, &name))
            return set_parse_error(p, p_.location, "expected a variable name");
        skip_whitespace(&p_);
        if (!match_exact(&p_, ")"))
            return set_parse_error(p, p_.location, "expected ')'");

        int index = -1; // find_or_add_var leaves this unset on its error path
        if (!find_or_add_var(p, enc, &name, &index))
            return false;

        *operand_const = "ENC_OP_VAR";
        snprintf(arg_expr, ARG_EXPR_N, "%d", index);
        *p = p_;
        return true;
    }

    if (match_exact(&p_, "Event"))
    {
        skip_whitespace(&p_);
        if (!match_exact(&p_, "."))
            return set_parse_error(p, p_.location, "expected '.' after 'Event'");
        skip_whitespace(&p_);
        struct Token field;
        if (!match_c_identifier(&p_, &field))
            return set_parse_error(p, p_.location, "expected an event field");

        if (is_literal_token(&field, "Battler"))       *operand_const = "ENC_OP_EVENT_BATTLER";
        else if (is_literal_token(&field, "Target"))   *operand_const = "ENC_OP_EVENT_TARGET";
        else if (is_literal_token(&field, "Move"))     *operand_const = "ENC_OP_EVENT_MOVE";
        else if (is_literal_token(&field, "MoveType")) *operand_const = "ENC_OP_EVENT_MOVE_TYPE";
        else if (is_literal_token(&field, "MoveCategory")) *operand_const = "ENC_OP_EVENT_MOVE_CATEGORY";
        else if (is_literal_token(&field, "Cause"))    *operand_const = "ENC_OP_EVENT_CAUSE";
        else if (is_literal_token(&field, "OldValue")) *operand_const = "ENC_OP_EVENT_OLD_VALUE";
        else if (is_literal_token(&field, "NewValue")) *operand_const = "ENC_OP_EVENT_NEW_VALUE";
        else
            return set_parse_error(p, field.location, "unknown event field (expected Battler, Target, Move, MoveType, MoveCategory, Cause, OldValue, or NewValue)");

        strcpy(arg_expr, "0");
        *p = p_;
        return true;
    }

    if (!match_c_identifier(&p_, &id))
        return set_parse_error(p, p_.location, "expected an operand");

    if (is_literal_token(&id, "Weather"))           *operand_const = "ENC_OP_WEATHER";
    else if (is_literal_token(&id, "Terrain"))      *operand_const = "ENC_OP_TERRAIN";
    else if (is_literal_token(&id, "Turn"))         *operand_const = "ENC_OP_TURN";
    else if (is_literal_token(&id, "BattlerCount")) *operand_const = "ENC_OP_BATTLER_COUNT";
    else
        return set_parse_error(p, id.location, "unknown operand (expected Battler(...), Var(...), Event...., Weather, Terrain, Turn, or BattlerCount)");

    strcpy(arg_expr, "0");
    *p = p_;
    return true;
}

__attribute__((warn_unused_result))
static bool parse_cmp(struct Parser *p, const char **cmp_const)
{
    // Longer operators first so e.g. '<=' isn't matched as '<' followed by a stray '='.
    if (match_exact(p, "==")) { *cmp_const = "ENC_CMP_EQ"; return true; }
    if (match_exact(p, "!=")) { *cmp_const = "ENC_CMP_NE"; return true; }
    if (match_exact(p, "<=")) { *cmp_const = "ENC_CMP_LE"; return true; }
    if (match_exact(p, ">=")) { *cmp_const = "ENC_CMP_GE"; return true; }
    if (match_exact(p, "<"))  { *cmp_const = "ENC_CMP_LT"; return true; }
    if (match_exact(p, ">"))  { *cmp_const = "ENC_CMP_GT"; return true; }
    return false;
}

// One leaf line: '<operand> <cmp> <value>'. Emits one flat-array initializer.
__attribute__((warn_unused_result))
static bool parse_condition_leaf(struct Parser *p, struct EncounterDef *enc, char *line_out, size_t line_out_n)
{
    const char *operand_const;
    char arg_expr[ARG_EXPR_N] = "0";
    if (!parse_operand(p, enc, &operand_const, arg_expr))
        return show_parse_error(p);

    skip_whitespace(p);
    struct SourceLocation cmp_loc = p->location;
    const char *cmp_const;
    if (!parse_cmp(p, &cmp_const))
        return set_show_parse_error(p, cmp_loc, "expected a comparison operator ('==', '!=', '<', '<=', '>', or '>=')");

    struct Token value;
    match_until_eol(p, &value);
    if (is_empty_token(&value))
        return set_show_parse_error(p, p->location, "expected a value");
    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after condition");

    snprintf(line_out, line_out_n, "{ .operand = %s, .cmp = %s, .arg = %s, .value = %.*s },",
             operand_const, cmp_const, arg_expr, value.end - value.begin, &value.source->buffer[value.begin]);
    return true;
}

enum GroupKind { GROUP_ALL, GROUP_ANY, GROUP_NOT };

// Parses sibling condition lines at exactly 'indent' columns, recursing into All:/Any:/Not:
// blocks at deeper indents. Appends every node's initializer to trig->cond_lines in prefix
// order (group header, then its children) - the same flat layout EvalNode walks at runtime.
// *item_count_out is the number of immediate items at this indent, i.e. what a group node one
// level up needs for its own '.arg' child count.
static void parse_condition_items(struct Parser *p, struct EncounterDef *enc, struct Trigger *trig,
                                   int indent, int depth, int *item_count_out)
{
    *item_count_out = 0;
    for (;;)
    {
        while (match_empty_line(p)) {}
        if (match_eof(p))
            return;

        struct Parser peek = *p;
        skip_whitespace(&peek);
        int this_indent = peek.location.column - 1;

        if (this_indent < indent)
            return; // dedent: end of this block, let the caller re-read this line
        if (this_indent > indent)
        {
            set_show_parse_error(p, peek.location, "unexpected indentation");
            skip_line(p);
            continue;
        }

        skip_whitespace(p);

        struct Parser try_p = *p;
        struct Token kw;
        bool is_group = false;
        enum GroupKind kind = GROUP_ALL;
        if (match_c_identifier(&try_p, &kw)
         && (is_literal_token(&kw, "All") || is_literal_token(&kw, "Any") || is_literal_token(&kw, "Not")))
        {
            struct Parser try2 = try_p;
            skip_whitespace(&try2);
            if (match_exact(&try2, ":"))
            {
                skip_whitespace(&try2);
                if (match_eol(&try2))
                {
                    kind = is_literal_token(&kw, "All") ? GROUP_ALL : is_literal_token(&kw, "Any") ? GROUP_ANY : GROUP_NOT;
                    is_group = true;
                    *p = try2;
                }
            }
        }

        if (is_group)
        {
            struct SourceLocation kw_loc = kw.location;
            if (depth >= MAX_COND_DEPTH)
                set_show_parse_error(p, kw_loc, "condition nesting exceeds MAX_ENCOUNTER_COND_DEPTH (4)");

            int child_indent;
            if (!peek_indent_after_blanks(p, &child_indent) || child_indent <= indent)
            {
                set_show_parse_error(p, p->location, "expected an indented condition block");
                (*item_count_out)++;
                continue;
            }

            int header_index = trig->cond_lines_n;
            append_cond_line(trig, ""); // placeholder; filled in once the child count is known

            int child_count = 0;
            parse_condition_items(p, enc, trig, child_indent, depth + 1, &child_count);

            char header[64];
            if (kind == GROUP_NOT)
            {
                if (child_count != 1)
                    set_show_parse_error(p, kw_loc, "'Not:' takes exactly one condition");
                snprintf(header, sizeof(header), "{ .operand = ENC_OP_NOT },");
            }
            else
            {
                snprintf(header, sizeof(header), "{ .operand = %s, .arg = %d },",
                         kind == GROUP_ALL ? "ENC_OP_ALL" : "ENC_OP_ANY", child_count);
            }
            free(trig->cond_lines[header_index]);
            trig->cond_lines[header_index] = strdup(header);
        }
        else
        {
            char line[COND_LINE_N];
            if (!parse_condition_leaf(p, enc, line, sizeof(line)))
            {
                skip_line(p);
                (*item_count_out)++;
                continue;
            }
            append_cond_line(trig, line);
        }

        (*item_count_out)++;
    }
}

__attribute__((warn_unused_result))
static bool parse_flags(struct Parser *p, char *flags_expr)
{
    strcpy(flags_expr, "0");
    bool first = true;
    for (;;)
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected a flag name");
        const char *name;
        if (!flag_const(&id, &name))
            return set_show_parse_error(p, id.location, "unknown flag (expected 'Once' or 'OnEnter')");

        if (first)
        {
            strcpy(flags_expr, name);
            first = false;
        }
        else
        {
            strcat(flags_expr, " | ");
            strcat(flags_expr, name);
        }

        skip_whitespace(p);
        if (match_exact(p, ","))
            continue;
        break;
    }

    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after flags");
    return true;
}

// Parses a comma-separated 'Immunities:' list into an OR of ENC_IMMUNE_* constants.
static bool parse_immunities(struct Parser *p, char *expr)
{
    bool first = true;

    strcpy(expr, "0");
    for (;;)
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected an immunity name");

        const char *name;
        if (!immunity_const(&id, &name))
            return set_show_parse_error(p, id.location,
                "unknown immunity (expected None, Ohko, FixedDamage, HpSwap, SharedKo, or All)");

        if (first)
        {
            strcpy(expr, name);
            first = false;
        }
        else
        {
            strcat(expr, " | ");
            strcat(expr, name);
        }

        skip_whitespace(p);
        if (match_exact(p, ","))
            continue;
        break;
    }

    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after immunities");
    return true;
}

// Parses a comma-separated 'Moves:' list into a brace initializer for struct EncounterProperties's
// moves[]. Move names pass through to the C compiler unresolved, like a condition's right-hand side
// - only the count is this tool's business.
static bool parse_moves(struct Parser *p, char *expr)
{
    int count = 0;

    strcpy(expr, "{");
    for (;;)
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected a move constant");
        if (count == MAX_MOVES_PER_ENCOUNTER)
            return set_show_parse_error(p, loc, "too many moves (max 4)");

        strncat(expr, count > 0 ? ", " : " ", LIST_EXPR_N - strlen(expr) - 1);
        append_token_text(expr, LIST_EXPR_N, &id);
        count++;

        skip_whitespace(p);
        if (match_exact(p, ","))
            continue;
        break;
    }
    strncat(expr, " }", LIST_EXPR_N - strlen(expr) - 1);

    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after moves");
    return true;
}

// Parses a '|'-separated 'AiFlags:' list into an OR expression. Like Moves:, the AI_FLAG_* names
// pass through unresolved - an unknown one is a clean compiler error, not this tool's problem.
static bool parse_ai_flags(struct Parser *p, char *expr)
{
    bool first = true;

    expr[0] = '\0';
    for (;;)
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected an AI flag constant");

        if (!first)
            strncat(expr, " | ", LIST_EXPR_N - strlen(expr) - 1);
        append_token_text(expr, LIST_EXPR_N, &id);
        first = false;

        skip_whitespace(p);
        if (match_exact(p, "|"))
            continue;
        break;
    }

    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after AI flags");
    return true;
}

// Parses one indented 'Key: value' line of a 'Properties:' block. Most of these are a closed
// vocabulary feeding a fixed-width field of struct EncounterProperties, so an out-of-range number is
// caught here rather than truncated silently. 'Moves:'/'AiFlags:' hold game constants instead and
// pass through to the C compiler like a condition's right-hand side; only their shape is checked.
__attribute__((warn_unused_result))
static bool parse_property(struct Parser *p, struct EncounterDef *enc, const struct Token *key)
{
    struct Properties *props = &enc->properties;
    int value;

    if (is_literal_token(key, "Level"))
    {
        skip_whitespace(p);
        struct Parser p_ = *p;
        struct Token id;
        if (match_c_identifier(&p_, &id))
        {
            if (!is_literal_token(&id, "LevelCap"))
                return set_show_parse_error(p, id.location, "expected a level or 'LevelCap'");
            strcpy(props->level, "ENC_LEVEL_CAP");
            *p = p_;
        }
        else if (!match_int(p, &value))
        {
            return set_show_parse_error(p, p->location, "expected a level or 'LevelCap'");
        }
        else if (value < 1 || value > MAX_ENCOUNTER_LEVEL)
        {
            return set_show_parse_error(p, p->location, "level must be between 1 and MAX_LEVEL");
        }
        else
        {
            snprintf(props->level, ARG_EXPR_N, "%d", value);
        }
    }
    else if (is_literal_token(key, "CatchRate"))
    {
        skip_whitespace(p);
        if (!match_int(p, &value))
            return set_show_parse_error(p, p->location, "expected an integer");
        if (value < 0 || value > 255)
            return set_show_parse_error(p, p->location, "catch rate must be between 0 and 255");
        snprintf(props->catch_rate, ARG_EXPR_N, "%d", value);
    }
    else if (is_literal_token(key, "Balls"))
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        const char *name;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected a ball policy");
        if (!ball_policy_const(&id, &name))
            return set_show_parse_error(p, id.location, "unknown ball policy (expected Default, Blocked, or Allowed)");
        strcpy(props->ball_policy, name);
    }
    else if (is_literal_token(key, "DamageReduction"))
    {
        skip_whitespace(p);
        if (!match_int(p, &value))
            return set_show_parse_error(p, p->location, "expected an integer");
        if (value < 0 || value > MAX_DAMAGE_REDUCTION)
            return set_show_parse_error(p, p->location, "damage reduction must be between 0 and 99 percent");
        snprintf(props->damage_reduction, ARG_EXPR_N, "%d", value);
    }
    else if (is_literal_token(key, "Immunities"))
    {
        return parse_immunities(p, props->immunities);
    }
    else if (is_literal_token(key, "CapTypeEffectiveness"))
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        const char *name;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected 'True' or 'False'");
        if (!bool_const(&id, &name))
            return set_show_parse_error(p, id.location, "unknown value (expected True or False)");
        strcpy(props->cap_type_effectiveness, name);
    }
    else if (is_literal_token(key, "FlatToxicDamage"))
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        const char *name;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected 'True' or 'False'");
        if (!bool_const(&id, &name))
            return set_show_parse_error(p, id.location, "unknown value (expected True or False)");
        strcpy(props->flat_toxic_damage, name);
    }
    else if (is_literal_token(key, "Survive"))
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        const char *name;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected 'True' or 'False'");
        if (!bool_const(&id, &name))
            return set_show_parse_error(p, id.location, "unknown value (expected True or False)");
        strcpy(props->survive, name);
    }
    else if (is_literal_token(key, "Ability"))
    {
        skip_whitespace(p);
        struct SourceLocation loc = p->location;
        struct Token id;
        if (!match_c_identifier(p, &id))
            return set_show_parse_error(p, loc, "expected an ability constant");
        props->ability[0] = '\0';
        append_token_text(props->ability, ARG_EXPR_N, &id);
    }
    else if (is_literal_token(key, "Moves"))
    {
        return parse_moves(p, props->moves);
    }
    else if (is_literal_token(key, "AiFlags"))
    {
        return parse_ai_flags(p, props->ai_flags);
    }
    else
    {
        return set_show_parse_error(p, key->location,
            "expected one of 'Level', 'CatchRate', 'Balls', 'DamageReduction', 'Immunities', "
            "'CapTypeEffectiveness', 'FlatToxicDamage', 'Survive', 'Ability', 'Moves', or 'AiFlags'");
    }

    skip_whitespace(p);
    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after property value");
    return true;
}

// Parses the 'Properties:' block - indented 'Key: value' lines, ending at the first line that
// isn't indented further than 'Properties:' itself.
static void parse_properties(struct Parser *p, struct EncounterDef *enc, bool *any_error)
{
    int indent;

    if (!peek_indent_after_blanks(p, &indent) || indent == 0)
    {
        *any_error = !set_show_parse_error(p, p->location, "expected an indented property after 'Properties:'");
        return;
    }

    for (;;)
    {
        int next_indent;
        while (match_empty_line(p)) {}
        if (match_eof(p))
            break;
        if (!peek_indent_after_blanks(p, &next_indent) || next_indent < indent)
            break;

        skip_whitespace(p);
        struct Token key;
        struct SourceLocation loc = p->location;
        if (!match_c_identifier(p, &key))
        {
            *any_error = !set_show_parse_error(p, loc, "expected a property name");
            skip_line(p);
            continue;
        }
        skip_whitespace(p);
        if (!match_exact(p, ":"))
        {
            *any_error = !set_show_parse_error(p, p->location, "expected ':' after a property name");
            skip_line(p);
            continue;
        }
        if (!parse_property(p, enc, &key))
        {
            *any_error = true;
            skip_line(p);
        }
    }

    enc->has_properties = true;
}

// Parses one 'Trigger: <Checkpoint>' block and its attributes, up to (not including) the next
// 'Trigger:'/'Encounter:' line.
__attribute__((warn_unused_result))
static bool parse_trigger(struct Parser *p, struct EncounterDef *enc, struct Trigger *trig)
{
    // memset, not a '{}' compound-literal assignment: struct Trigger is large enough (cond_lines
    // alone is 2KB) that a temporary isn't guaranteed to be optimized away.
    memset(trig, 0, sizeof(*trig));
    strcpy(trig->flags_expr, "0");

    trig->trigger_keyword_line = p->location.line;

    if (!match_exact(p, "Trigger"))
        return false;
    skip_whitespace(p);
    if (!match_exact(p, ":"))
        return set_show_parse_error(p, p->location, "expected ':' after 'Trigger'");
    skip_whitespace(p);

    struct Token checkpoint;
    struct SourceLocation checkpoint_loc = p->location;
    if (!match_c_identifier(p, &checkpoint))
        return set_show_parse_error(p, checkpoint_loc, "expected a checkpoint name");
    if (!checkpoint_const(&checkpoint, &trig->checkpoint_const))
        return set_show_parse_error(p, checkpoint.location,
            "unknown checkpoint (expected OnBattleStart, OnTurnStart, OnMoveEnd, OnFaint, OnSwitchIn, OnTurnEnd, or OnBattleEnd)");
    trig->checkpoint_line = checkpoint.location.line;
    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after checkpoint name");

    for (;;)
    {
        while (match_empty_line(p)) {}

        struct Parser p_ = *p;
        struct Token key;
        if (!match_c_identifier(&p_, &key))
            break;

        if (is_literal_token(&key, "Trigger") || is_literal_token(&key, "Encounter"))
            break; // next block; don't consume it

        skip_whitespace(&p_);
        if (!match_exact(&p_, ":"))
        {
            set_show_parse_error(p, p_.location, "expected ':'");
            skip_line(p);
            continue;
        }

        if (is_literal_token(&key, "Priority"))
        {
            skip_whitespace(&p_);
            *p = p_;
            int value;
            if (!match_int(p, &value))
                set_show_parse_error(p, p->location, "expected an integer");
            else if (value < 0 || value > 255)
                set_show_parse_error(p, p->location, "priority must be between 0 and 255");
            else
                { trig->priority = value; trig->has_priority = true; }
            skip_whitespace(p);
            if (!match_eol(p))
                set_show_parse_error(p, p->location, "unexpected character after priority");
        }
        else if (is_literal_token(&key, "Flags"))
        {
            *p = p_;
            parse_flags(p, trig->flags_expr);
        }
        else if (is_literal_token(&key, "Script"))
        {
            skip_whitespace(&p_);
            *p = p_;
            struct SourceLocation loc = p->location;
            if (!match_c_identifier(p, &trig->script))
                set_show_parse_error(p, loc, "expected a script label");
            else
                trig->has_script = true;
            skip_whitespace(p);
            if (!match_eol(p))
                set_show_parse_error(p, p->location, "unexpected character after script label");
        }
        else if (is_literal_token(&key, "Conditions"))
        {
            skip_whitespace(&p_);
            if (!match_eol(&p_))
            {
                set_show_parse_error(p, p_.location, "unexpected character after 'Conditions:'");
                break;
            }
            *p = p_;

            int indent;
            if (!peek_indent_after_blanks(p, &indent) || indent == 0)
            {
                set_show_parse_error(p, p->location, "expected an indented condition after 'Conditions:'");
            }
            else
            {
                int count = 0;
                parse_condition_items(p, enc, trig, indent, 0, &count);
                if (count == 0)
                    set_show_parse_error(p, p->location, "expected at least one condition");
                append_cond_line(trig, "{ .operand = ENC_OP_COUNT },");
                trig->has_conditions = true;
            }
        }
        else
        {
            set_show_parse_error(p, key.location, "expected one of 'Priority', 'Flags', 'Script', or 'Conditions'");
            skip_line(p);
            continue;
        }
    }

    bool ok = true;
    if (!trig->has_priority)
        ok = set_show_parse_error(p, p->location, "expected 'Priority'");
    if (!trig->has_script)
        ok = set_show_parse_error(p, p->location, "expected 'Script'");

    return ok;
}

// Parses one 'Encounter: <Name>' block and every 'Trigger:' under it, up to (not including) the
// next 'Encounter:' line or EOF.
__attribute__((warn_unused_result))
static bool parse_encounter(struct Parser *p, struct EncounterDef *enc)
{
    // memset, not a '{}' compound-literal assignment - see parse_trigger's comment; EncounterDef
    // (32 embedded triggers) is worse, around 70KB.
    memset(enc, 0, sizeof(*enc));

    // "change nothing" defaults, so an encounter without a Properties: block still emits a valid
    // (and inert) struct EncounterProperties initializer.
    strcpy(enc->properties.level, "ENC_LEVEL_NONE");
    strcpy(enc->properties.catch_rate, "ENC_CATCH_RATE_NONE");
    strcpy(enc->properties.ball_policy, "ENC_BALLS_DEFAULT");
    strcpy(enc->properties.damage_reduction, "0");
    strcpy(enc->properties.immunities, "0");
    strcpy(enc->properties.cap_type_effectiveness, "FALSE");
    strcpy(enc->properties.flat_toxic_damage, "FALSE");
    strcpy(enc->properties.survive, "FALSE");
    strcpy(enc->properties.ability, "ENC_ABILITY_NONE");
    strcpy(enc->properties.moves, "{0}");
    strcpy(enc->properties.ai_flags, "0");

    if (!match_exact(p, "Encounter"))
        return false;
    skip_whitespace(p);
    if (!match_exact(p, ":"))
        return set_show_parse_error(p, p->location, "expected ':' after 'Encounter'");
    skip_whitespace(p);

    struct SourceLocation loc = p->location;
    if (!match_c_identifier(p, &enc->name))
        return set_show_parse_error(p, loc, "expected an encounter name");
    enc->name_line = enc->name.location.line;
    if (!match_eol(p))
        return set_show_parse_error(p, p->location, "unexpected character after encounter name");

    bool any_error = false;
    for (;;)
    {
        while (match_empty_line(p)) {}
        if (match_eof(p))
            break;

        struct Parser p_ = *p;
        struct Token key;
        if (!match_c_identifier(&p_, &key))
        {
            any_error = !set_show_parse_error(p, p->location, "expected 'Trigger:', 'Properties:', or 'Encounter:'");
            skip_line(p);
            continue;
        }
        if (is_literal_token(&key, "Encounter"))
            break; // next encounter; let the top-level loop handle it

        if (is_literal_token(&key, "Properties"))
        {
            skip_whitespace(&p_);
            if (!match_exact(&p_, ":"))
            {
                any_error = !set_show_parse_error(p, p_.location, "expected ':' after 'Properties'");
                skip_line(p);
                continue;
            }
            skip_whitespace(&p_);
            if (!match_eol(&p_))
            {
                any_error = !set_show_parse_error(p, p_.location, "unexpected character after 'Properties:'");
                skip_line(p);
                continue;
            }
            if (enc->has_properties)
            {
                any_error = !set_show_parse_error(p, key.location, "this encounter already has a 'Properties:' block");
                skip_line(p);
                continue;
            }
            *p = p_;
            parse_properties(p, enc, &any_error);
            continue;
        }

        if (!is_literal_token(&key, "Trigger"))
        {
            any_error = !set_show_parse_error(p, key.location, "expected 'Trigger:', 'Properties:', or 'Encounter:'");
            skip_line(p);
            continue;
        }

        if (enc->triggers_n == MAX_TRIGGERS_PER_ENCOUNTER)
        {
            any_error = !set_show_parse_error(p, key.location, "too many triggers in this encounter (max 32)");
            skip_line(p);
            continue;
        }

        struct Trigger *trig = &enc->triggers[enc->triggers_n];
        if (!parse_trigger(p, enc, trig))
            any_error = true;
        else
            enc->triggers_n++;
    }

    if (enc->triggers_n == 0)
        any_error = !set_show_parse_error(p, p->location, "expected at least one 'Trigger:'");

    return !any_error;
}

// Appends to 'parsed'; called once per source file, so encounters from every file end up in one
// gEncounters[]. Order doesn't matter - the array is built with designated initializers.
static void parse(struct Parser *p, struct Parsed *parsed)
{
    if (!parsed->encounters)
    {
        parsed->encounters_c = 4;
        parsed->encounters = malloc(sizeof(*parsed->encounters) * parsed->encounters_c);
        assert(parsed->encounters);
    }

    for (;;)
    {
        while (match_empty_line(p)) {}
        if (match_eof(p))
            break;

        struct Parser p_ = *p;
        struct Token key;
        if (!match_c_identifier(&p_, &key) || !is_literal_token(&key, "Encounter"))
        {
            set_show_parse_error(p, p->location, "expected 'Encounter:'");
            skip_line(p);
            continue;
        }

        if (parsed->encounters_n == parsed->encounters_c)
        {
            parsed->encounters_c *= 2;
            struct EncounterDef *encounters_ = realloc(parsed->encounters, sizeof(*parsed->encounters) * parsed->encounters_c);
            assert(encounters_);
            parsed->encounters = encounters_;
        }

        struct EncounterDef *enc = &parsed->encounters[parsed->encounters_n];
        if (parse_encounter(p, enc))
            parsed->encounters_n++;
    }
}

// --- Emission ----------------------------------------------------------------------------------

static void fprint_token(FILE *f, const struct Token *t)
{
    fprintf(f, "%.*s", t->end - t->begin, &t->source->buffer[t->begin]);
}

// Uppercases a token's text with no prefix, for the ENC_VAR_*_<NAME> defines.
static void fprint_upper_token(FILE *f, const struct Token *t)
{
    for (int i = t->begin; i < t->end; i++)
    {
        unsigned char c = t->source->buffer[i];
        if ('a' <= c && c <= 'z')
            c = c - 'a' + 'A';
        fputc(c, f);
    }
}

// Same convention as trainerproc's fprint_constant: prefixes with 'prefix_' unless already
// present, uppercases letters/digits, maps everything else to '_'.
static void constant_from_token(const char *prefix, const struct Token *t, char *out, size_t out_n)
{
    struct String s = token_string(t);
    size_t len = 0;

    if (!is_constant(s, prefix))
        len += snprintf(out, out_n, "%s_", prefix);

    for (int i = 0; i < s.string_n && len + 1 < out_n; i++)
    {
        unsigned char c = s.string[i];
        if (('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))
            out[len++] = c;
        else if ('a' <= c && c <= 'z')
            out[len++] = c - 'a' + 'A';
        else
            out[len++] = '_';
    }
    out[len] = '\0';
}

// Number of triggers with the same encounter that carry their own conditions array - determines
// whether sConditions_<Name> needs a '_<n>' suffix to stay unique.
static void cond_array_name(const struct EncounterDef *enc, int trigger_index, char *out, size_t out_n)
{
    if (enc->triggers_n > 1)
        snprintf(out, out_n, "sConditions_%.*s_%d", enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin], trigger_index + 1);
    else
        snprintf(out, out_n, "sConditions_%.*s", enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin]);
}

static void fprint_encounters(FILE *f, struct Parsed *parsed, const struct Source *sources, int sources_n)
{
    fprintf(f, "//\n// DO NOT MODIFY THIS FILE! It is auto-generated from:\n");
    for (int i = 0; i < sources_n; i++)
        fprintf(f, "//   %s\n", sources[i].path);
    fprintf(f, "//\n\n");

    fprintf(f, "#if TESTING\n");
    fprintf(f, "// Per-trigger authored source location - dev builds only, no release ROM cost (Outline Sec39).\n");
    fprintf(f, "struct EncounterTriggerSourceLocation { const char *file; u16 line; };\n");
    fprintf(f, "#endif\n\n");

    for (int i = 0; i < parsed->encounters_n; i++)
    {
        struct EncounterDef *enc = &parsed->encounters[i];
        char enc_const[64];
        constant_from_token("ENCOUNTER", &enc->name, enc_const, sizeof(enc_const));
        const char *enc_path = enc->name.source->path;

        fprintf(f, "#line %d \"%s\"\n", enc->name_line, enc_path);

        if (enc->vars_n > 0)
        {
            const char *bare = enc_const + strlen("ENCOUNTER_");
            for (int v = 0; v < enc->vars_n; v++)
            {
                fprintf(f, "#define ENC_VAR_%s_", bare);
                fprint_upper_token(f, &enc->vars[v].name);
                fprintf(f, " %d\n", enc->vars[v].index);
            }
            fprintf(f, "\n");
        }

        for (int t = 0; t < enc->triggers_n; t++)
        {
            struct Trigger *trig = &enc->triggers[t];
            if (!trig->has_conditions)
                continue;

            char name[80];
            cond_array_name(enc, t, name, sizeof(name));

            fprintf(f, "#line %d \"%s\"\n", trig->trigger_keyword_line, enc_path);
            fprintf(f, "static const struct EncounterCondition %s[] =\n{\n", name);
            for (int l = 0; l < trig->cond_lines_n; l++)
                fprintf(f, "    %s\n", trig->cond_lines[l]);
            fprintf(f, "};\n\n");
        }

        fprintf(f, "#line %d \"%s\"\n", enc->name_line, enc_path);
        fprintf(f, "static const struct EncounterTrigger sTriggers_%.*s[] =\n{\n",
                enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin]);
        for (int t = 0; t < enc->triggers_n; t++)
        {
            struct Trigger *trig = &enc->triggers[t];
            char cond_name[80] = "NULL";
            if (trig->has_conditions)
                cond_array_name(enc, t, cond_name, sizeof(cond_name));

            fprintf(f, "#line %d \"%s\"\n", trig->trigger_keyword_line, enc_path);
            fprintf(f, "    { .checkpoint = %s, .priority = %d, .flags = %s, .conditions = %s, .script = ",
                    trig->checkpoint_const, trig->priority, trig->flags_expr, cond_name);
            fprint_token(f, &trig->script);
            fprintf(f, " },\n");
        }
        fprintf(f, "};\n\n");

        fprintf(f, "#if TESTING\n");
        fprintf(f, "static const struct EncounterTriggerSourceLocation sTriggersDebug_%.*s[] =\n{\n",
                enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin]);
        for (int t = 0; t < enc->triggers_n; t++)
            fprintf(f, "    { \"%s\", %d },\n", enc_path, enc->triggers[t].trigger_keyword_line);
        fprintf(f, "};\n#endif\n\n");
    }

    fprintf(f, "const struct Encounter gEncounters[ENCOUNTER_COUNT] =\n{\n");
    for (int i = 0; i < parsed->encounters_n; i++)
    {
        struct EncounterDef *enc = &parsed->encounters[i];
        char enc_const[64];
        constant_from_token("ENCOUNTER", &enc->name, enc_const, sizeof(enc_const));
        fprintf(f, "#line %d \"%s\"\n", enc->name_line, enc->name.source->path);
        fprintf(f, "    [%s] =\n    {\n", enc_const);
        fprintf(f, "        .triggers = sTriggers_%.*s,\n",
                enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin]);
        fprintf(f, "        .triggerCount = ARRAY_COUNT(sTriggers_%.*s),\n",
                enc->name.end - enc->name.begin, &enc->name.source->buffer[enc->name.begin]);
        fprintf(f, "        .properties =\n        {\n");
        fprintf(f, "            .level = %s,\n", enc->properties.level);
        fprintf(f, "            .catchRate = %s,\n", enc->properties.catch_rate);
        fprintf(f, "            .ballPolicy = %s,\n", enc->properties.ball_policy);
        fprintf(f, "            .damageReduction = %s,\n", enc->properties.damage_reduction);
        fprintf(f, "            .immunities = %s,\n", enc->properties.immunities);
        fprintf(f, "            .capTypeEffectiveness = %s,\n", enc->properties.cap_type_effectiveness);
        fprintf(f, "            .flatToxicDamage = %s,\n", enc->properties.flat_toxic_damage);
        fprintf(f, "            .survive = %s,\n", enc->properties.survive);
        fprintf(f, "            .ability = %s,\n", enc->properties.ability);
        fprintf(f, "            .moves = %s,\n", enc->properties.moves);
        fprintf(f, "            .aiFlags = %s,\n", enc->properties.ai_flags);
        fprintf(f, "        },\n    },\n");
    }
    fprintf(f, "};\n");
}

static void usage(FILE *file, char *argv0)
{
    fprintf(file, "Usage: %s -o <output> <source>...\n", argv0);
}

// Reads the whole file into a malloc'd buffer. Tokens point into it for the rest of the run, so
// the buffer outlives parsing and is only freed at exit.
static bool read_source(const char *path, unsigned char **buffer_out, int *buffer_n_out)
{
    bool ok = false;
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "could not open '%s' for reading\n", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long buffer_n = ftell(file);
    if (buffer_n > INT_MAX)
    {
        fprintf(stderr, "could not read '%s': too big\n", path);
        goto exit;
    }

    unsigned char *buffer = malloc(buffer_n);
    if (!buffer)
    {
        fprintf(stderr, "could not allocate %ld bytes\n", buffer_n);
        goto exit;
    }

    rewind(file);
    if (fread(buffer, 1, buffer_n, file) < (size_t)buffer_n)
    {
        fprintf(stderr, "could not read '%s'\n", path);
        free(buffer);
        goto exit;
    }

    *buffer_out = buffer;
    *buffer_n_out = buffer_n;
    ok = true;

exit:
    fclose(file);
    return ok;
}

int main(int argc, char *argv[])
{
    int status = 1;
    FILE *output_file = NULL;
    struct Source *sources = NULL;
    int sources_n = 0;
    struct Parsed parsed = {};

    const char *output_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "o:")) != -1)
    {
        switch (opt)
        {
        case 'o':
            output_path = optarg;
            break;
        default:
            fprintf(stderr, "unknown option '%c'\n", opt);
            usage(stderr, argv[0]);
            goto exit;
        }
    }

    if (!output_path || optind == argc)
    {
        usage(stderr, argv[0]);
        goto exit;
    }

    // Every source is parsed into one gEncounters[], so an encounter can live in whichever file
    // suits it (see src/data/legendary_encounters/) without the runtime knowing about the split.
    sources = calloc(argc - optind, sizeof(*sources));
    assert(sources);
    for (; optind < argc; optind++)
    {
        const char *source_path = argv[optind];
        unsigned char *buffer;
        int buffer_n;
        if (!read_source(source_path, &buffer, &buffer_n))
            goto exit;

        struct Source *source = &sources[sources_n++];
        *source = (struct Source) {
            .path = source_path,
            .buffer = buffer,
            .buffer_n = buffer_n,
        };

        struct Parser parser = {
            .source = source,
            .location = { .line = 1, .column = 1 },
            .offset = 0,
        };
        parse(&parser, &parsed);
        if (parser.fatal_error)
            goto exit;
    }

    if (strcmp(output_path, "-") == 0)
    {
        output_file = stdout;
        output_path = "<stdout>";
    }
    else
    {
        output_file = fopen(output_path, "w");
        if (output_file == NULL)
        {
            fprintf(stderr, "could not open '%s' for writing\n", output_path);
            goto exit;
        }
    }
    fprint_encounters(output_file, &parsed, sources, sources_n);

    status = 0;

exit:
    if (output_file && output_file != stdout) fclose(output_file);
    if (parsed.encounters) free(parsed.encounters);
    for (int i = 0; i < sources_n; i++)
        free((unsigned char *)sources[i].buffer);
    free(sources);
    return status;
}
