#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ndbm.h"

#define _DFL_DIRSHIFT 5
#define _DFL_SEGSHIFT 12
#define _DFL_DIRSIZE (1<<_DFL_DIRSHIFT)
#define _DFL_SEGSIZE (1<<_DFL_SEGSHIFT)
#define _DFL_MAX_BUCKETSIZE (_DFL_DIRSIZE * _DFL_SEGSIZE)
#define _DFL_FFACTOR 4

#define _NOW_VERSION 1
#define _ALIGNMENT_BYTE 8
#define _MAGIC "_ndbm_"
#define _MAGIC_SIZ ((size_t)8)
#define _SMALL_MSEG_NUM 64
#define _SMALL_MSEG_NMEMB 8
#define _BIG_MSEG_NUM 20
#define _BIG_MSEG_NMEMB 4
#define _SEGSIZE_THRESHOLD 1024
#define _MAX_ALLOCSIZE (1024*1024*1024)

typedef struct{
	char magic[_MAGIC_SIZ];
	pthread_spinlock_t lock_point;
	int64_t free_offset;
	int64_t small_msegs[_SMALL_MSEG_NUM];
	int64_t big_msegs[_BIG_MSEG_NUM];
	int32_t replaced;
	int32_t version;
	time_t ctime;
//	int64_t dir[_DATA_HASH_SIZ];
	int32_t dir_size;
	int32_t seg_size;
	int32_t dir_num;
	int32_t seg_num;
	int32_t bucket_num;
	int64_t dir[];
} _ndbm_header_t;

static NDBM* _ndbm_create(const char *path,size_t size,int flags,mode_t mode);
static NDBM* _ndbm_open(const char *path,int flags);
static void _init(NDBM *ndbm);

NDBM* ndbm_open(const char *path,size_t size,int flags,mode_t mode)
{
	if( flags & NDBM_CREATE ){
		return( _ndbm_create(path,size,flags,mode) );
	}
	else{
		return( _ndbm_open(path,flags) );
	}
}

int ndbm_close(NDBM *dbm)
{
	if( NULL == dbm ){
		return(1);
	}

	if( dbm->_base != MAP_FAILED ){
		munmap(dbm->_base,dbm->_size);
		dbm->_base = MAP_FAILED;
	}

	if( dbm->_fd >= 0 ){
		close(dbm->_fd); dbm->_fd = -1;
	}

	_init(dbm); free(dbm);
	return(0);
}

static NDBM* _ndbm_create(const char *path,size_t size,int flags,mode_t mode)
{
	NDBM *dbm = NULL;
	int prot = PROT_READ;
	_ndbm_header_t *header;
	int ix;

	if( NULL == path ){
		goto error;
	}

	dbm = (NDBM*)malloc(sizeof(*dbm));
	if( NULL == dbm ){
		goto error;
	}

	_init(dbm);
	dbm->_fd = open(path,flags,mode);
	if( dbm->_fd < 0 ){
		goto error;
	}

	if( ftruncate(dbm->_fd,(off_t)size) != 0 ){
		goto error;
	}
	dbm->_size = (size_t)size;

	if( flags & NDBM_RDWR ){
		prot |= PROT_WRITE;
	}

	dbm->_base = mmap(0,dbm->_size,prot,MAP_SHARED,dbm->_fd,(off_t)0);
	if( MAP_FAILED == dbm->_base ){
		goto error;
	}

	header = (_ndbm_header_t*)dbm->_base;
	strncpy(header->magic,_MAGIC,_MAGIC_SIZ);
	header->version = _NOW_VERSION;
	header->replaced = 0;
	header->ctime = time(NULL);
	header->dir_size = _DFL_DIRSIZE;
	header->dir_num = 0;
	header->seg_num = 0;
	header->bucket_num = 0;
	if( pthread_spin_init(&header->lock_point,PTHREAD_PROCESS_SHARED) != 0 ){
		goto error;
	}
	header->free_offset = (int64_t)(sizeof(_ndbm_header_t) + sizeof(int64_t) * header->dir_size);

	for(ix = 0; ix < _SMALL_MSEG_NUM; ix++ ){
		header->small_msegs[ix] = (int64_t)-1;
	}

	for(ix = 0; ix < _BIG_MSEG_NUM; ix++ ){
		header->big_msegs[ix] = (int64_t)-1;
	}

	for(ix = 0; ix < header->dir_size; ix++ ){
		header->dir[ix] = (int64_t)-1;
	}

	return(dbm);

 error:
	ndbm_close(dbm); dbm = NULL;
	return(NULL);
}

static NDBM* _ndbm_open(const char *path,int flags)
{
	NDBM *dbm = NULL;
	int prot = PROT_READ;
	struct stat st;

	if( NULL == path ){
		goto error;
	}

	dbm = (NDBM*)malloc(sizeof(*dbm));
	if( NULL == dbm ){
		goto error;
	}

	_init(dbm);
	dbm->_fd = open(path,flags);
	if( dbm->_fd < 0 ){
		goto error;
	}

	if( fstat(dbm->_fd,&st) < 0 ){
		goto error;
	}
	dbm->_size = (size_t)st.st_size;

	if( flags & NDBM_RDWR ){
		prot |= PROT_WRITE;
	}

	dbm->_base = mmap(0,dbm->_size,prot,MAP_SHARED,dbm->_fd,(off_t)0);
	if( MAP_FAILED == dbm->_base ){
		goto error;
	}

//	if( strncmp(base->magic,_MAGIC,strlen(_MAGIC)) != 0 ){
//		goto error;
//	}
//
//	if( base->version != _NOW_VERSION ){
//		goto error;
//	}

	return(dbm);

 error:
	ndbm_close(dbm); dbm = NULL;
	return(NULL);
}

static void _init(NDBM *dbm)
{
	dbm->_fd = -1;
	dbm->_size = 0;
	dbm->_base = MAP_FAILED;
	sigfillset(&dbm->_full_set);
	sigemptyset(&dbm->_backup_set);
	return;
}

