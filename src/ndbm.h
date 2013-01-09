#ifndef __NDBM_H_
#define __NDBM_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define NDBM_CREATE 64
#define NDBM_RDONLY 0
#define NDBM_RDWR 1

//#include <sys/time.h>
//#include <stdint.h>
//#include <time.h>

//#define SET_NSHMBASE(ptr) if(ptr){_nshm_base=ptr->_base;}
//#define DEF_NSHMBASE(ptr) void*_nshm_base=ptr->_base;
//#define vos_assign(cast,ptr) ((cast)(ptr? (char*)ptr-(char*)_nshm_base: -1))
//#define vos_ptr(cast,offset) ((cast)(offset>0? (char*)_nshm_base+offset: 0))

//typedef struct{
//	int _fd;
//	size_t _size;
//	void* _base;
//	sigset_t _full_set;
//	sigset_t _backup_set;
//} NShm;

typedef struct{
	int _fd;
	size_t _size;
	void* _base;
	sigset_t _full_set;
	sigset_t _backup_set;
} NDBM;


#ifdef __cplusplus
extern "C" {
#endif

extern NDBM* ndbm_open(const char *path,size_t size,int flags,mode_t mode);
extern int ndbm_close(NDBM *dbm);
//extern NShm* nshm_attach(const char *path);
//extern void nshm_detach(NShm *nshm);
//extern NShm* nshm_reattach(const char *path,NShm *nshm);
//
//extern void* nshm_memalloc(NShm *nshm,int32_t size);
//extern void nshm_memfree(NShm *nshm,void *ptr);
//extern void* shmalloc(NShm *nshm,int32_t size);
//
//extern void* nshm_get(NShm *nshm,const char *key,int klen);
//extern int nshm_set(NShm *nshm,const char *key,int klen,const void *val);
//
//extern int nshm_get_replaced(NShm *nshm);
//extern time_t nshm_get_ctime(NShm *nshm);
//extern int64_t nshm_get_restbyte(NShm *nshm);
//
//extern int nshm_set_replaced(NShm *nshm);

#ifdef __cplusplus
}
#endif

#endif
