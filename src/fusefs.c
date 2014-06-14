/*
** Copyright (c) 2014 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@sqlite.org
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This module implements the userspace side of a Fuse Filesystem that
** contains all check-ins for a fossil repository.  
**
** This module is a mostly a no-op unless compiled with -DFOSSIL_HAVE_FUSEFS.
** The FOSSIL_HAVE_FUSEFS should be omitted on systems that lack support for
** the Fuse Filesystem, of course.
*/
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "fusefs.h"
#ifdef FOSSIL_HAVE_FUSEFS

#define FUSE_USE_VERSION 26
#include <fuse.h>

/*
** Global state information about the archive
*/
static struct sGlobal {
  /* A cache of a single check-in manifest */
  int rid;                  /* rid for the cached manifest */
  char *zSymName;           /* Symbolic name corresponding to rid */
  Manifest *pMan;           /* The cached manifest */
  /* A cache of a single file within a single check-in */
  int iFileRid;             /* Check-in ID for the cached file */
  ManifestFile *pFile;      /* Name of a cached file */
  Blob content;             /* Content of the cached file */
  /* Parsed path */
  char *az[3];              /* 0=type, 1=id, 2=path */
} fusefs;

/*
** Split of the input path into 0, 1, 2, or 3 elements in fusefs.az[].
** Return the number of elements.
**
** Any prior path parse is deleted.
*/
static int fusefs_parse_path(const char *zPath){
  int i, j;
  for(i=0; i<count(fusefs.az); i++){
    fossil_free(fusefs.az[i]);
    fusefs.az[i] = 0;
  }
  if( strcmp(zPath,"/")==0 ) return 0;
  for(i=0, j=1; i<2 && zPath[j]; i++){
    int jStart = j;
    while( zPath[j] && zPath[j]!='/' ){ j++; }
    fusefs.az[i] = mprintf("%.*s", j-jStart, &zPath[jStart]);
    if( zPath[j] ) j++;
  }
  if( zPath[j] ) fusefs.az[i++] = fossil_strdup(&zPath[j]);
  return i;
}

/*
** Load manifest rid into the cache.
*/
static void fusefs_load_rid(int rid, const char *zSymName){
  if( fusefs.rid==rid && fusefs.pMan!=0 ) return;
  blob_reset(&fusefs.content);
  manifest_destroy(fusefs.pMan);
  fossil_free(fusefs.zSymName);
  fusefs.zSymName = fossil_strdup(zSymName);
  fusefs.pFile = 0;
  fusefs.pMan = manifest_get(rid, CFTYPE_MANIFEST, 0);
  fusefs.rid = rid;
}

/*
** Locate the rid corresponding to a symbolic name
*/
static int fusefs_name_to_rid(const char *zSymName){
  if( fusefs.rid>0 && strcmp(zSymName, fusefs.zSymName)==0 ){
    return fusefs.rid;
  }else{
    return symbolic_name_to_rid(zSymName, "ci");
  }
}


/*
** Implementation of stat()
*/
static int fusefs_getattr(const char *zPath, struct stat *stbuf){
  int n, rid;
  ManifestFile *pFile;
  stbuf->st_uid = getuid();
  stbuf->st_gid = getgid();
  n = fusefs_parse_path(zPath);
  if( n==0 ){
    stbuf->st_mode = S_IFDIR | 0555;
    stbuf->st_nlink = 2;
    return 0;
  }
  if( strcmp(fusefs.az[0],"checkins")!=0 ) return -ENOENT;
  if( n==1 ){
    stbuf->st_mode = S_IFDIR | 0111;
    stbuf->st_nlink = 2;
    return 0;
  }
  rid = fusefs_name_to_rid(fusefs.az[1]);
  if( rid<=0 ) return -ENOENT;
  if( n==2 ){
    stbuf->st_mode = S_IFDIR | 0555;
    stbuf->st_nlink = 2;
    return 0;
  }
  fusefs_load_rid(rid, fusefs.az[1]);
  if( fusefs.pMan==0 ) return -ENOENT;
  pFile = manifest_file_seek(fusefs.pMan, fusefs.az[2], 1);
  if( pFile==0 ) return -ENOENT;
  stbuf->st_mtime = (fusefs.pMan->rDate - 2440587.5)*86400.0;
  if( strcmp(fusefs.az[2], pFile->zName)==0 ){
    stbuf->st_mode = S_IFREG |
              (manifest_file_mperm(pFile)==PERM_EXE ? 0555 : 0444);
    stbuf->st_nlink = 1;
    stbuf->st_size = db_int(0, "SELECT size FROM blob WHERE uuid='%s'", 
                               pFile->zUuid);
    return 0;
  }
  n = (int)strlen(fusefs.az[2]);
  if( strncmp(fusefs.az[2], pFile->zName, n)!=0 ) return -ENOENT;
  if( pFile->zName[n]!='/' ) return -ENOENT;
  stbuf->st_mode = S_IFDIR | 0555;
  stbuf->st_nlink = 2;
  return 0;
}

/*
** Implementation of readdir()
*/
static int fusefs_readdir(
  const char *zPath,
  void *buf,
  fuse_fill_dir_t filler,
  off_t offset,
  struct fuse_file_info *fi
){
  int n, rid;
  ManifestFile *pFile;
  const char *zPrev = "";
  int nPrev = 0;
  char *z;
  int cnt = 0;
  n = fusefs_parse_path(zPath);
  if( n==0 ){
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "checkins", NULL, 0);    
    return 0;
  }
  if( strcmp(fusefs.az[0],"checkins")!=0 ) return -ENOENT;
  if( n==1 ) return -ENOENT;
  rid = fusefs_name_to_rid(fusefs.az[1]);
  if( rid<=0 ) return -ENOENT;
  fusefs_load_rid(rid, fusefs.az[1]);
  if( fusefs.pMan==0 ) return -ENOENT;
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  manifest_file_rewind(fusefs.pMan);
  if( n==2 ){
    while( (pFile = manifest_file_next(fusefs.pMan, 0))!=0 ){
      if( nPrev>0 && strncmp(pFile->zName, zPrev, nPrev)==0 ) continue;
      zPrev = pFile->zName;
      for(nPrev=0; zPrev[nPrev] && zPrev[nPrev]!='/'; nPrev++){}
      z = mprintf("%.*s", nPrev, zPrev);
      filler(buf, z, NULL, 0);
      fossil_free(z);
      cnt++;
    }
  }else{
    char *zBase = mprintf("%s/", fusefs.az[2]);
    int nBase = (int)strlen(zBase);
    while( (pFile = manifest_file_next(fusefs.pMan, 0))!=0 ){
      if( strcmp(pFile->zName, zBase)>=0 ) break;
    }
    while( pFile && strncmp(zBase, pFile->zName, nBase)==0 ){
      if( nPrev==0 || strncmp(pFile->zName+nBase, zPrev, nPrev)!=0 ){
        zPrev = pFile->zName+nBase;
        for(nPrev=0; zPrev[nPrev] && zPrev[nPrev]!='/'; nPrev++){}
        if( zPrev[nPrev]=='/' ){
          z = mprintf("%.*s", nPrev, zPrev);
          filler(buf, z, NULL, 0);
          fossil_free(z);
        }else{
          filler(buf, zPrev, NULL, 0);
          nPrev = 0;
        }
        cnt++;
      }
      pFile = manifest_file_next(fusefs.pMan, 0);
    }
  }
  return cnt>0 ? 0 : -ENOENT;
}


/*
** Implementation of read()
*/
static int fusefs_read(
  const char *zPath,
  char *buf,
  size_t size,
  off_t offset,
  struct fuse_file_info *fi
){
  int n, rid;
  n = fusefs_parse_path(zPath);
  if( n<3 ) return -ENOENT;
  if( strcmp(fusefs.az[0], "checkins")!=0 ) return -ENOENT;
  rid = fusefs_name_to_rid(fusefs.az[1]);
  if( rid<=0 ) return -ENOENT;
  fusefs_load_rid(rid, fusefs.az[1]);
  if( fusefs.pFile!=0 && strcmp(fusefs.az[2], fusefs.pFile->zName)!=0 ){
    fusefs.pFile = 0;
    blob_reset(&fusefs.content);
  }
  fusefs.pFile = manifest_file_seek(fusefs.pMan, fusefs.az[2], 0);
  if( fusefs.pFile==0 ) return -ENOENT;
  rid = uuid_to_rid(fusefs.pFile->zUuid, 0);
  content_get(rid, &fusefs.content);
  if( offset>blob_size(&fusefs.content) ) return 0;
  if( offset+size>blob_size(&fusefs.content) ){
    size = blob_size(&fusefs.content) - offset;
  }
  memcpy(buf, blob_buffer(&fusefs.content)+offset, size);
  return size;
}  

static struct fuse_operations fusefs_methods = {
  .getattr = fusefs_getattr,
  .readdir = fusefs_readdir,
  .read    = fusefs_read,
};
#endif /* FOSSIL_HAVE_FUSEFS */

/*
** COMMAND: fusefs
**
** Usage: %fossil fusefs DIRECTORY
**
** This command uses the Fuse Filesystem to mount a directory at
** DIRECTORY that contains the content of all check-ins in the
** repository.
*/
void fusefs_cmd(void){
#ifndef FOSSIL_HAVE_FUSEFS
  fossil_fatal("this build of fossil does not support the fuse filesystem");
#else
  char *zMountPoint;
  char *azNewArgv[5];
  int doDebug = find_option("debug","d",0)!=0;
  db_find_and_open_repository(0,0);
  verify_all_options();
  blob_init(&fusefs.content, 0, 0);
  if( g.argc!=3 ) usage("DIRECTORY");
  zMountPoint = g.argv[2];
  if( file_mkdir(zMountPoint, 0) ){
    fossil_fatal("cannot make directory [%s]", zMountPoint);
  }
  azNewArgv[0] = g.argv[0];
  azNewArgv[1] = doDebug ? "-d" : "-f";
  azNewArgv[2] = "-s";
  azNewArgv[3] = zMountPoint;
  azNewArgv[4] = 0;
  fuse_main(4, azNewArgv, &fusefs_methods, NULL);
#endif
}
