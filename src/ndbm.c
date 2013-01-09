#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "ndbm.h"

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

