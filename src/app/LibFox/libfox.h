/*
$Id:  $

Description: Use fault oblivious key-value store for communication

$Log: $
*/

#ifndef _LIBFOX_H
#define _LIBFOX_H

typedef struct fox_st *fox_ptr;

int fox_new(fox_ptr *fhand_ptr, int nprocs, int procid);

int fox_free(fox_ptr fhand);

int fox_flush(fox_ptr fhand, int term);

int fox_server_add(fox_ptr fhand, const char *hostlist);

int fox_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_get(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	char **pvalue, size_t *pvalue_sz);

int fox_delete(
	fox_ptr fhand,
	const char *key, size_t key_sz);

int fox_sync_set(
	fox_ptr fhand,
	unsigned delta,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_sync_get(
	fox_ptr fhand,
	unsigned delta,
	const char *key, size_t key_sz,
	char **pvalue, size_t *pvalue_sz);

int fox_broad_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_broad_get(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	char **pvalue, size_t *pvalue_sz);

int fox_queue_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_queue_get(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	char **pvalue, size_t *pvalue_sz);

int fox_broad_queue_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_dist_queue_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_dist_queue_get(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	char **pvalue, size_t *pvalue_sz);

int fox_reduce_set(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	const char *value, size_t value_sz);

int fox_reduce_get(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	char *pvalue, size_t pvalue_sz,
	void (*reduce)(void *out, void *in));

/* Experimental, may change radically */

enum {C_RENDEZVOUS=1, C_DISPERSE=2, C_BARRIER=3};

int fox_collect(
	fox_ptr fhand,
	const char *key, size_t key_sz,
	int root, int opt);

#endif /* _LIBFOX_H */
