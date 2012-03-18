#ifndef __CORE_PLAYER_H
#define __CORE_PLAYER_H
#include <base/helper.h>
#include <base/intreg.h>
#include <base/cxform.h>
#include <base/matrix.h>

struct transform {
	struct matrix matrix;
	struct cxform cxform;
};

struct string {
	const char *str;
	size_t len;
};

struct player;
struct sprite;
struct parser;
struct muface;
struct memface;
struct logface;
struct errface;

struct player *player_create(struct muface *mux, struct memface *mem, struct logface *log, struct errface *err);
void player_delete(struct player *pl);

void player_load(struct player *pl, const struct string target, void *stream);

void player_advance(struct player *pl);

struct bufctx;
struct rectangle;

// rt is a value-result argument.
// As input, rt is the minimum region needed to be redrawn.
// As output, rt is the region needed to be refresh.
void player_render(struct player *pl, struct transform tsm, struct bufctx *bx, struct rectangle *rt);

enum place_flag {
	PlaceFlagMove			= 1 << 0,
	PlaceFlagHasCharacter		= 1 << 1,
	PlaceFlagHasMatrix		= 1 << 2,
	PlaceFlagHasCxform		= 1 << 3,
	PlaceFlagHasRatio		= 1 << 4,
	PlaceFlagHasName		= 1 << 5,
	PlaceFlagHasClipDepth		= 1 << 6,
	PlaceFlagHasClipActions		= 1 << 7,
	PlaceFlagHasFilterList		= 1 << 8,
	PlaceFlagHasBlendMode		= 1 << 9,
	PlaceFlagHasCacheAsBit		= 1 << 10,
	PlaceFlagHasClassName		= 1 << 11,
	PlaceFlagHasImage		= 1 << 12,
};

enum character_type {
	CharacterShape,
	CharacterSprite,
	CharacterMorphShape,
	CharacterButton,
	CharacterSource,
	CharacterFont,
	CharacterText,
	CharacterEditText,
	CharacterTypeNumber
};

struct place_info {
	uintreg_t flag;
	uintreg_t type;
	uintptr_t character;
	struct transform transform;
	uintreg_t clipdepth;
	uintreg_t chardepth;
	uintreg_t stepratio;
	struct string moviename;
};

void sprite_place_object(struct sprite *si, const struct place_info *pi);
void sprite_remove_object(struct sprite *si, uintreg_t);

#endif
