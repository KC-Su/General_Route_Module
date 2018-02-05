#ifndef __GLIST_H
#define __GLIST_H

/* This file is from Linux Kernel (include/linux/glist.h) 
 * and modified by simply removing hardware prefetching of glist items. 
 * Here by copyright, credits attributed to wherever they belong.
 * Kulesh Shanmugasundaram (kulesh [squiggly] isis.poly.edu)
 */

/*
 * Simple doubly linked glist implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole glists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#include <stdlib.h>
#define LIST_POISON1	NULL
#define LIST_POISON2	NULL

struct glist_head {
	struct glist_head *next, *prev;
};

#define GLIST_HEAD_INIT(name) { &(name), &(name) }

#define GLIST_HEAD(name) \
	struct glist_head name = GLIST_HEAD_INIT(name)

#define INIT_GLIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal glist manipulation where we know
 * the prev/next entries already!
 */
static inline void __glist_add(struct glist_head *new,
			      struct glist_head *prev,
			      struct glist_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * glist_add - add a new entry
 * @new: new entry to be added
 * @head: glist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void glist_add(struct glist_head *new, struct glist_head *head)
{
	__glist_add(new, head, head->next);
}

/**
 * glist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: glist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void glist_add_tail(struct glist_head *new, struct glist_head *head)
{
	__glist_add(new, head->prev, head);
}

/*
 * Delete a glist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal glist manipulation where we know
 * the prev/next entries already!
 */
static inline void __glist_del(struct glist_head *prev, struct glist_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * glist_del - deletes entry from glist.
 * @entry: the element to delete from the glist.
 * Note: glist_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void glist_del(struct glist_head *entry)
{
	__glist_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
}

/**
 * glist_del_init - deletes entry from glist and reinitialize it.
 * @entry: the element to delete from the glist.
 */
static inline void glist_del_init(struct glist_head *entry)
{
	__glist_del(entry->prev, entry->next);
	INIT_GLIST_HEAD(entry); 
}

/**
 * glist_move - delete from one glist and add as another's head
 * @glist: the entry to move
 * @head: the head that will precede our entry
 */
static inline void glist_move(struct glist_head *glist, struct glist_head *head)
{
        __glist_del(glist->prev, glist->next);
        glist_add(glist, head);
}

/**
 * glist_move_tail - delete from one glist and add as another's tail
 * @glist: the entry to move
 * @head: the head that will follow our entry
 */
static inline void glist_move_tail(struct glist_head *glist,
				  struct glist_head *head)
{
        __glist_del(glist->prev, glist->next);
        glist_add_tail(glist, head);
}

/**
 * glist_empty - tests whether a glist is empty
 * @head: the glist to test.
 */
static inline int glist_empty(struct glist_head *head)
{
	return head->next == head;
}

static inline void __glist_splice(struct glist_head *glist,
				 struct glist_head *head)
{
	struct glist_head *first = glist->next;
	struct glist_head *last = glist->prev;
	struct glist_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
 * glist_splice - join two glists
 * @glist: the new glist to add.
 * @head: the place to add it in the first glist.
 */
static inline void glist_splice(struct glist_head *glist, struct glist_head *head)
{
	if (!glist_empty(glist))
		__glist_splice(glist, head);
}

/**
 * glist_splice_init - join two glists and reinitialise the emptied glist.
 * @glist: the new glist to add.
 * @head: the place to add it in the first glist.
 *
 * The glist at @glist is reinitialised
 */
static inline void glist_splice_init(struct glist_head *glist,
				    struct glist_head *head)
{
	if (!glist_empty(glist)) {
		__glist_splice(glist, head);
		INIT_GLIST_HEAD(glist);
	}
}

/**
 * glist_entry - get the struct for this entry
 * @ptr:	the &struct glist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the glist_struct within the struct.
 */
#define glist_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * glist_for_each	-	iterate over a glist
 * @pos:	the &struct glist_head to use as a loop counter.
 * @head:	the head for your glist.
 */
#define glist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
/**
 * glist_for_each_prev	-	iterate over a glist backwards
 * @pos:	the &struct glist_head to use as a loop counter.
 * @head:	the head for your glist.
 */
#define glist_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)
        	
/**
 * glist_for_each_safe	-	iterate over a glist safe against removal of glist entry
 * @pos:	the &struct glist_head to use as a loop counter.
 * @n:		another &struct glist_head to use as temporary storage
 * @head:	the head for your glist.
 */
#define glist_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * glist_for_each_entry	-	iterate over glist of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your glist.
 * @member:	the name of the glist_struct within the struct.
 */
#define glist_for_each_entry(pos, head, member)				\
	for (pos = glist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = glist_entry(pos->member.next, typeof(*pos), member))

/**
 * glist_for_each_entry_safe - iterate over glist of given type safe against removal of glist entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your glist.
 * @member:	the name of the glist_struct within the struct.
 */
#define glist_for_each_entry_safe(pos, n, head, member)			\
	for (pos = glist_entry((head)->next, typeof(*pos), member),	\
		n = glist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = glist_entry(n->member.next, typeof(*n), member))


/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

typedef struct hlist_head {
	struct hlist_node *first;
} hlist_head_t;

typedef struct hlist_node {
	struct hlist_node *next, **pprev;
} hlist_node_t;

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) hlist_head_t name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static __inline void INIT_HLIST_NODE(hlist_node_t *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static __inline int hlist_unhashed(const hlist_node_t *h)
{
	return !h->pprev;
}

static __inline int hlist_empty(const hlist_head_t *h)
{
	return !h->first;
}

static __inline void __hlist_del(hlist_node_t *n)
{
	hlist_node_t *next = n->next;
	hlist_node_t **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static __inline void hlist_del(hlist_node_t *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

static __inline void hlist_del_init(hlist_node_t *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static __inline void hlist_add_head(hlist_node_t *n, hlist_head_t *h)
{
	hlist_node_t *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static __inline void hlist_add_before(hlist_node_t *n,
					hlist_node_t *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static __inline void hlist_add_after(hlist_node_t *n,
					hlist_node_t *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static __inline void hlist_move_list(hlist_head_t *old,
				   hlist_head_t *new)
{
	new->first = old->first;
	if (new->first)
		new->first->pprev = &new->first;
	old->first = NULL;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

//add by jeff
#define hlist_first_entry(hh, type, member) \
	 hlist_entry(hh->first, type, member);	

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos; \
	     pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first; \
         pos && ({ n = pos->next; 1; }); \
         pos = n)

#endif
