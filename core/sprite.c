#include "player.h"
#include "muplex.h"
#include "define.h"
#include "common.h"
#include <base/compat.h>
#include <base/intreg.h>
#include <base/helper.h>
#include <base/slab.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

enum depth_class {
	DepthClassTimeline,
	DepthClassDynamic,
	DepthClassReserve,
	DepthClassInvalid,
};

typedef unsigned long depth_t;

static inline uintreg_t
depth_classify(depth_t dh) {
	if (dh < 0x4000) {
		return DepthClassTimeline;
	} else {
		return DepthClassDynamic;
	}
}

typedef uint16_t obtype_t;

#define ObjectFields				\
	uint16_t type;				\
	uint16_t slabname:1;			\
	uint16_t dycreate:1;			\
	uint16_t scripted:1;			\
	uint16_t forwarding:1;			\
	uint16_t issource:1;			\
	uint16_t dirty:1;			\
	uint16_t stopped:1;			\
	uint16_t clipdepth;			\
	uint16_t stepratio;			\
	depth_t depth;				\
	uintptr_t character;			\
	struct transform transform;		\
	struct object *above;			\
	struct object *parent

#define ObnameFields				\
	ObjectFields;				\
	struct string name

#define SpriteFields				\
	ObnameFields;				\
	struct thread thread;			\
	struct object *display;			\
	struct sprite_define define;		\
	uintptr_t tagpos;			\
	intreg_t cframe;			\
	struct sprite *sibling;			\
	struct sprite *children;		\
	struct source *source;			\
	struct source *scroot;			\
	bool fastforwarding

struct dictionary;

#define SourceFields				\
	SpriteFields;				\
	struct stream *stream

#define obj2obname(ob)		((struct obname *)(ob))
#define obj2sprite(ob)		((struct sprite *)(ob))
#define obj2source(ob)		((struct source *)(ob))
#define obj2object(ob)		((struct object *)(ob))

bool
obtype_scriptable(uintreg_t type) {
	return ((type == CharacterSprite) || (type == CharacterButton) || (type == CharacterEditText));
}

struct object {
	ObjectFields;
};

struct obname {
	ObnameFields;
};

struct thread {
	struct thread *tdnext;
	struct player *player;
};

struct sprite {
	SpriteFields;
};

struct source {
	SourceFields;
};

static struct sprite *
sprite_from_thread(struct thread *td) {
	return obj2sprite(((char *)td - offsetof(struct sprite, thread)));
}

// Layered-source: chained by 'above', 'depth' is level.
// player is _level0
struct player {
	SourceFields;
	struct thread *threads;
	size_t unnamed_instances;
	struct slab_pool *sapool;
	struct slab *object_slab[CharacterTypeNumber];
	struct slab *name_slab[3];
	struct muface *mux;
	struct memface *mem;
	struct logface *log;
	struct errface *err;
	// struct object *drag_object;
	// struct point drag_spoint;
};

struct shape {
	ObjectFields;
};

struct morphshape {
	ObjectFields;
};

struct button {
	ObjectFields;
};

/* Thread Status */
enum thread_status {
	ThreadStatusStop,
	ThreadStatusPlay
};

static inline struct object *
object_parent(const struct object *ob) {
	return ob->parent;
}

static inline depth_t
object_depth(const struct object *ob) {
	return ob->depth;
}

static uintreg_t
object_type(const struct object *ob) {
	return ob->type;
}

static void
object_replace_character(struct object *ob, uintptr_t ch, uintreg_t type) {
	// FIXME Find out replacement-condition ?
	uintreg_t oldt = object_type(ob);
	if (oldt == type && !obtype_scriptable(oldt)) {
		ob->character = ch;
	}
}

static bool
object_script_created(const struct object *ob) {
	return ob->dycreate;
}

static bool
object_script_removable(const struct object *ob) {
	depth_t dh = object_depth(ob);
	return depth_classify(dh) == DepthClassDynamic;
}

static bool
object_scriptable(const struct object *ob) {
	return obtype_scriptable(object_type(ob));
}

void
object_script_change(struct object *ob) {
	ob->scripted = 1;
}

bool
object_script_changed(const struct object *ob) {
	return ob->scripted;
}

void
object_timeline_change(struct object *ob) {
	ob->dirty = 1;
}

bool
object_timeline_changeable(const struct object *ob) {
	return !object_script_changed(ob);
}

static bool
object_compatible(struct object *ob, struct object *in) {
	assert(ob->depth == in->depth);
	// loadMovie(url, target) replaces sprite object with source object,
	// they have same types but different characters.
	//
	// So we use 'type(ob->character) == type(in->character)'
	// instead of 'ob->character == in->character'.
	return ob->stepratio == in->stepratio
	       && ob->clipdepth == in->clipdepth
	       && object_type(ob) == object_type(in);
}

static void
object_import_place(struct object *ob, const struct place_info *pi) {
	assert(object_type(ob) == pi->type);
	ob->type = (obtype_t)pi->type;
	ob->character = pi->character;
	ob->clipdepth = pi->clipdepth;
	ob->stepratio = pi->stepratio;
	ob->transform = pi->transform;

	if (object_type(ob) == CharacterSprite && (pi->flag & PlaceFlagHasName) != 0) {
		struct sprite *si = obj2sprite(ob);
		si->name = pi->moviename;
	}
}

// Export 'character' 'transform' 'clipdepth' fields of pi.
static inline void
object_export_place(const struct object *ob, struct place_info *pi) {
	pi->type = ob->type;
	pi->character = ob->character;
	pi->clipdepth = ob->clipdepth;
	pi->transform = ob->transform;
}

static void
object_change_place(struct object *ob, const struct place_info *pi) {
	if (object_timeline_changeable(ob)) {
		uintreg_t flag = pi->flag;
		if ((flag & PlaceFlagHasCharacter)) {
			object_replace_character(ob, pi->character, pi->type);
		}
		if ((flag & PlaceFlagHasMatrix)) {
			ob->transform.matrix = pi->transform.matrix;
		}
		if ((flag & PlaceFlagHasCxform)) {
			ob->transform.cxform = pi->transform.cxform;
		}
		if ((flag & PlaceFlagHasRatio)) {
			ob->stepratio = pi->stepratio;
		}
		object_timeline_change(ob);
	}
}

static inline struct object *
player_create_object(struct player *pl, uintreg_t type) {
	assert(type < CharacterTypeNumber);
	assert(pl->object_slab[type] != NULL);
	struct object *ob = slab_alloc(pl->object_slab[type]);
	ob->type = (obtype_t)type;
	return ob;
}

static inline void
player_delete_object(struct player *pl, struct object *ob) {
	uintreg_t type = object_type(ob);
	slab_dealloc(pl->object_slab[type], ob);
}

static void
player_attach_obname(struct player *pl, struct obname *on) {
	if (on->name.str != NULL) {
		return;
	}
	char buf[30];	// "instance" plus 20 characters of max uint64_t.
	size_t n = ++ pl->unnamed_instances;
	size_t len = snprintf(buf, sizeof(buf), "instance%zu", n);
	assert(len < sizeof(buf));
	size_t i = len/10;
	struct slab *sa = pl->name_slab[i];
	if (sa == NULL) {
		size_t isize[3] = {10, 20, 30};
		size_t nitem[3] = {9, 20, 40};
		sa = pl->name_slab[i] = slab_pool_alloc(pl->sapool, nitem[i], isize[i]);
	}
	char *str = slab_alloc(sa);
	strcpy(str, buf);
	on->slabname = 1;
	on->name.str = str;
	on->name.len = len;
	switch (i) {
	case 2: {
		struct slab *sa = pl->name_slab[1];
		if (sa != NULL && slab_idle(sa)) {
			pl->name_slab[1] = NULL;
			slab_pool_dealloc(pl->sapool, sa);
		}
	} // fallthrough
	case 1: {
		struct slab *sa = pl->name_slab[0];
		if (sa != NULL && slab_idle(sa)) {
			pl->name_slab[0] = NULL;
			slab_pool_dealloc(pl->sapool, sa);
		}
	} break;
	}
}

static void
player_detach_obname(struct player *pl, struct obname *on) {
	if (on->slabname) {
		size_t i = on->name.len/10;
		slab_dealloc(pl->name_slab[i], (void *)on->name.str);
	}
}

static inline void
player_attach_thread(struct player *pl, struct thread *td) {
	td->player = pl;
	td->tdnext = pl->threads;
	pl->threads = td;
}

static void
player_detach_thread(struct player *pl, struct thread *td) {
	struct thread *ti = pl->threads;
	struct thread **tp = &pl->threads;
	while (ti != td) {
		assert(ti != NULL);
		tp = &ti->tdnext;
		ti = ti->tdnext;
	}
	assert(ti == td);
	*tp = ti->tdnext;
}

void
player_detach_source(struct player *pl, struct source *sc) {
	player_detach_thread(pl, &sc->thread);
}

struct source *
player_umount_level(struct player *pl, uintreg_t lvl) {
	assert(lvl != 0);
	struct object **oo = &pl->above;
	while ((*oo)->depth < lvl) {
		oo = &(*oo)->above;
	}
	if ((*oo)->depth == lvl) {
		struct source *sc = obj2source(*oo);
		*oo = (*oo)->above;
		return sc;
	}
	return NULL;
}

void
player_mount_level(struct player *pl, struct source *sc) {
	uint32_t lvl = sc->depth;
	struct object **oo = &pl->above;
	while ((*oo)->depth < lvl) {
		oo = &(*oo)->above;
	}
	sc->above = *oo;
	*oo = obj2object(sc);
}

void
player_remove_level(struct player *pl, uintreg_t lvl) {
	if (lvl == 0) {
		// XXX _level0 can be remove ?
		return;
	}
	struct source *sc = player_umount_level(pl, lvl);
	player_detach_source(pl, sc);
}

struct source *
player_search_level(const struct player *pl, uintreg_t lvl) {
	for (struct source *sc = obj2source(pl); sc != NULL; sc = obj2source(sc->above)) {
		if (sc->depth == lvl) {
			return sc;
		}
	}
	return NULL;
}

static struct player *
player_from_thread(const struct thread *td) {
	return td->player;
}

static void sprite_advance(struct sprite *si);


static bool
sprite_is_player(const struct sprite *si) {
	return si == obj2sprite(player_from_thread(&si->thread));
}

static inline bool
sprite_is_source(const struct sprite *si) {
	return si == obj2sprite(si->source);
}

static void
sprite_mount_object(struct sprite *si, depth_t dh, struct object *in) {
	struct object *ob = si->display;
	struct object **oo = &si->display;
	for (; ob != NULL && ob->depth < dh; oo = &ob->above, ob = ob->above) {
	}
	assert(ob == NULL || ob->depth > dh);
	in->above = ob;
	*oo = in;
}

static struct object *
sprite_umount_object(struct sprite *si, depth_t dh) {
	struct object *ob = si->display;
	struct object **oo = &si->display;
	for (; ob != NULL && ob->depth < dh; oo = &ob->above, ob = ob->above) {
	}
	if (ob != NULL && ob->depth == dh) {
		*oo = ob->above;
		return ob;
	}
	return NULL;
}

static struct object *
sprite_search_object(struct sprite *si, depth_t depth) {
	struct object *ob;
	for (ob = si->display; ob != NULL && ob->depth < depth; ob = ob->above) {
	}
	if (ob != NULL && ob->depth == depth) {
		return ob;
	}
	return NULL;
}

static inline void
sprite_add_child(struct sprite *si, struct sprite *child) {
	child->sibling = si->children;
	si->children = child;
}

static inline void
sprite_del_child(struct sprite *si, struct sprite *child) {
	struct sprite **ss = &si->children;
	for (; *ss != NULL && *ss != child; ss = &(*ss)->sibling) {
	}
	assert(*ss != NULL);
	*ss = child->sibling;
}

static inline void
sprite_initz(struct sprite *si, struct source *sc, struct player *pl) {
	si->tagpos = si->define.tagbeg;
	si->source = si->scroot = sc;
	player_attach_thread(pl, &si->thread);
	player_attach_obname(pl, obj2obname(si));
}

static inline void
sprite_finiz(struct sprite *si) {
	struct player *pl = player_from_thread(&si->thread);
	player_detach_thread(pl, &si->thread);
	player_detach_obname(pl, obj2obname(si));
}

void
sprite_attach_object(struct sprite *si, struct object *ob) {
	switch (object_type(ob)) {
	case CharacterSprite: {
		struct sprite *so = obj2sprite(ob);
		sprite_add_child(si, so);
		struct source *sc = si->source;
		struct stream *stm = sc->stream;
		stream_struct_sprite(stm, so->character, &so->define);
		sprite_initz(so, sc, player_from_thread(&si->thread));
		sprite_advance(so);
	} break;
	default:
		break;
	}
}

static void
sprite_detach_object(struct sprite *si, struct object *ob) {
	switch (object_type(ob)) {
	case CharacterSprite: {
		struct sprite *so = obj2sprite(ob);
		sprite_del_child(si, so);
		sprite_finiz(so);
	} break;
	default:
		break;
	}
}

static inline void
sprite_change_object(struct sprite *si, const struct place_info *pi) {
	struct object *ob = sprite_search_object(si, pi->chardepth);
	if (ob) {
		object_change_place(ob, pi);
	}
}

static struct object *
sprite_create_object(struct sprite *si, const struct place_info *pi) {
	struct player *pl = player_from_thread(&si->thread);
	struct object *ob = player_create_object(pl, pi->type);
	if (ob) {
		object_import_place(ob, pi);
		sprite_attach_object(si, ob);
		sprite_mount_object(si, ob->depth, ob);
	}
	return ob;
}

static void
sprite_delete_object(struct sprite *si, struct object *ob) {
	struct player *pl = player_from_thread(&si->thread);
	sprite_detach_object(si, ob);
	player_delete_object(pl, ob);
}

void
sprite_place_object(struct sprite *si, const struct place_info *pi) {
	if ((pi->flag & PlaceFlagMove)) {
		sprite_change_object(si, pi);
	} else {
		sprite_create_object(si, pi);
	}
}

void
sprite_remove_object(struct sprite *si, depth_t dh) {
	struct object *ob = sprite_umount_object(si, dh);
	if (ob != NULL) {
		sprite_delete_object(si, ob);
	}
}

static inline bool
sprite_clonable(const struct sprite *si) {
	return !sprite_is_source(si);
}

static void
sprite_clone(struct sprite *si, uintreg_t depth, const char *name, size_t len) {
	if (sprite_clonable(si)) {
		struct place_info pi;
		object_export_place((struct object *)si, &pi);
		pi.flag = PlaceFlagHasName;
		pi.chardepth = depth;
		// Cloned sprite may be placed with depth fallen into timeline zone.
		// Special stepratio value do discriminate it from timeline sprites.
		//
		// See: object_compatible().
		pi.stepratio = (uintreg_t)-1;
		pi.moviename.str = name;
		pi.moviename.len = len;

		struct sprite *parent = obj2sprite(si->parent);
		sprite_remove_object(parent, depth);
		struct object *ob = sprite_create_object(parent, &pi);
		obj2sprite(ob)->dycreate = 1;
	}
}

static void
sprite_remove(struct sprite *si) {
	struct sprite *parent = obj2sprite(si->parent);
	if (parent == NULL) {
		struct player *pl = player_from_thread(&si->thread);
		player_remove_level(pl, si->depth);
	} else {
		sprite_remove_object(parent, object_depth((struct object *)si));
	}
}

static inline bool
sprite_script_removalbe(const struct sprite *si) {
	return object_script_removable((struct object *)si);
}

void
sprite_script_clone(struct sprite *si, depth_t dh, const char *name, size_t len) {
	sprite_clone(si, dh, name, len);
}

void
sprite_script_remove(struct sprite *si) {
	if (sprite_script_removalbe(si)) {
		sprite_remove(si);
	}
}

// TODO Better to use old-objects ? Old-objects may catched by outside actions.
// Objects in si's displaylist are all timeline.
static void
sprite_merge_old(struct sprite *si, struct object *old) {
	struct object *ob = si->display;
	struct object **oblink = &si->display;
	while (ob != NULL && old != NULL) {
		assert(depth_classify(ob->depth) == DepthClassTimeline);
		if (ob->depth > old->depth) {
			if (depth_classify(old->depth) == DepthClassTimeline) {
				struct object *tmp = old->above;
				sprite_delete_object(si, old);
				old = tmp;
			} else {
				struct object *tmp = old->above;
				old->above = ob;
				*oblink = old;
				oblink = &old->above;
				old = tmp;
			}
		} else if (ob->depth == old->depth) {
			if (object_compatible(ob, old) && object_type(ob) == CharacterSprite) {
				if (!object_script_changed(old)) {
					old->transform = ob->transform;
				}
				// Use old instead of new.
				struct object *tmp = old->above;
				struct object *del = ob;
				ob = old->above = ob->above;
				*oblink = old;
				oblink = &old->above;
				old = tmp;
				sprite_delete_object(si, del);
			} else {
				struct object *tmp = old->above;
				sprite_delete_object(si, old);
				old = tmp;
				oblink = &ob->above;
				ob = ob->above;
			}
		} else {
			oblink = &ob->above;
			ob = ob->above;
		}
	}
	if (ob == NULL) {
		while (old != NULL) {
			if (depth_classify(old->depth) == DepthClassTimeline) {
				struct object *tmp = old->above;
				sprite_delete_object(si, old);
				old = tmp;
			} else {
				*oblink = old;
				break;
			}
		}
	}
}

static inline void
sprite_start_fastforward(struct sprite *si) {
	si->fastforwarding = 1;
}

static inline void
sprite_stop_fastforward(struct sprite *si) {
	si->fastforwarding = 0;
}

static void
sprite_progress_frames(struct sprite *si, uintreg_t n) {
	uintptr_t tagpos = si->tagpos;
	struct stream *stm = si->source->stream;
	if (n > 1) {
		sprite_start_fastforward(si);
		do {
			tagpos = stream_progress_frame(stm, si, tagpos);
		} while(--n > 1);
		sprite_stop_fastforward(si);
	}
	assert(n == 1);
	si->tagpos = stream_progress_frame(stm, si, tagpos);
}

static void
sprite_goto_frame(struct sprite *si, intreg_t frame) {
	if (frame < si->cframe) {
		struct object *old = si->display;
		si->display = NULL;
		si->cframe = -1;
		si->tagpos = si->define.tagbeg;
		sprite_progress_frames(si, frame+1);
		sprite_merge_old(si, old);
	} else if (frame > si->cframe) {
		sprite_progress_frames(si, frame-si->cframe);
	}
	si->cframe = frame;
}

static void
sprite_advance(struct sprite *si) {
	if (!si->stopped) {
		intreg_t to = si->cframe+1;
		if (to == si->define.nframe) {
			to = 0;
		}
		sprite_goto_frame(si, to);
	}
}

#define string_build(str, len)		&(struct string){.str=str, .len=len}

static bool
string_equalto(const struct string *s0, const struct string *s1) {
	if (s0->len == s1->len && memcmp(s0->str, s1->str, s0->len) == 0) {
		return true;
	}
	return false;
}

int
string_compare(const struct string *a, const struct string *b) {
	return strcmp(a->str, b->str);
}

static intreg_t
memcompare(const void *s1, const void *s2, size_t n) {
	intreg_t d=0;
	for (size_t i=0; i<n && d==0; i++) {
		d = (intreg_t)*((unsigned char *)s1) - (intreg_t)*((unsigned char *)s2);
	}
	return d;
}

// Locate first occurence of c in ptr.
// If found, return its location, otherwise return end.
static void *
memlocate(const void *ptr, const void *end, intreg_t c) {
	const unsigned char *ret=ptr;
	for (; ret<(unsigned char *)end && *ret != c; ret++) {
	}
	return (void *)ret;
}

// Convert string to level number.
// If success, return first invalid character, otherwise return NULL.
#define char_is_digit(ch)	((ch) >= '0' || (ch) <= '9')
#define char_to_digit(ch)	((ch) - '0')
static char *
mem2level(const char *ptr, const char *end, uintreg_t *level) {
	const char *ret = NULL;
	if (ptr < end && char_is_digit(*ptr)) {
		uintreg_t l = 0;
		do {
			l = l*10 + char_to_digit(*ptr);
			ptr++;
		} while (ptr < end && char_is_digit(*ptr));
		ret = ptr;
		*level = l;
	}
	return (char *)ret;
}
#undef char_to_digit
#undef char_is_digit

struct source *
sprite_search_level(const struct sprite *si, uintreg_t lvl) {
	return player_search_level(player_from_thread(&si->thread), lvl);
}

static struct sprite *
sprite_search_child(const struct sprite *si, const struct string *name) {
	for (si = si->children; si != NULL; si = si->sibling) {
		if (string_equalto(&si->name, name)) {
			break;
		}
	}
	return (void *)si;
}

static inline struct sprite *
sprite_search_name(const struct sprite *si, const char *str, size_t len) {
	assert(si != NULL);
	if (len == 2 && str[0] == '.' && str[1] == '.') {
		si = obj2sprite(si->parent);
	} else {
		const struct string *name = string_build(str, len);
		si = sprite_search_child(si, name);
	}
	return (void *)si;
}

struct sprite *
sprite_search_target(const struct sprite *si, const char *path, const char *pend) {
	if (path < pend) {
		uintreg_t level;
		const char *end;
		if (path[0] == '/') {
			path++;
			si = obj2sprite(si->scroot);
		} else if (pend-path > 6 && memcompare(path, "_level", 6) == 0 && (end = mem2level(path+6, pend, &level))) {
			path = end;
			si = obj2sprite(sprite_search_level(si, level));
		}
	}
	while (path < pend && si != NULL) {
		const char *sepa = memlocate(path, pend, '/');
		si = sprite_search_name(si, path, sepa-path);
		path = sepa+1;
	}
	return (void *)si;
}

void
player_advance(struct player *pl) {
	for (struct thread *td = pl->threads; td != NULL; td = td->tdnext) {
		sprite_advance(sprite_from_thread(td));
	}
	// TODO Execute script
}

static inline void
player_inita(struct player *pl, struct muface *mux, struct memface *mem, struct logface *log, struct errface *err) {
	memset(pl, 0, sizeof(*pl));
	pl->mux = mux;
	pl->mem = mem;
	pl->log = log;
	pl->err = err;
	pl->unnamed_instances = 0;
	pl->sapool = NULL;
	pl->threads = NULL;
}

static inline void
player_initz(struct player * restrict pl) {
	sprite_initz(obj2sprite(pl), obj2source(pl), pl);
	pl->sapool = slab_pool_create(pl->mem, 10);
	pl->object_slab[CharacterShape] = slab_pool_alloc(pl->sapool, 20, sizeof(struct shape));
	pl->object_slab[CharacterSprite] = slab_pool_alloc(pl->sapool, 10, sizeof(struct sprite));
}

void
player_load0(struct player *pl, const void *ud, enum stream_type ut) {
	// XXX How to reflect rate/size to outside ?
	struct stream_define def;
	pl->stream = pl->mux->create_stream(pl->mux->muplex, ud, ut, &def);
	pl->define.nframe = def.nframe;
	pl->define.tagbeg = def.tagbeg;
	player_initz(pl);
}

struct player *
player_create(struct muface *mux, struct memface *mem, struct logface *log, struct errface *err) {
	struct player *pl = mem->alloc(mem->ctx, sizeof(*pl), __FILE__, __LINE__);
	player_inita(pl, mux, mem, log, err);
	return pl;
}

void
player_delete(struct player *pl) {
	// TODO delete all streams associated with sources.
	if (pl->stream) {
		pl->mux->delete_stream(pl->mux->muplex, pl->stream);
	}
	if (pl->sapool) {
		slab_pool_delete(pl->sapool);
	}
	pl->mem->dealloc(pl->mem->ctx, pl, __FILE__, __LINE__);
}
