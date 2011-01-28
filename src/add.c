/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code used to check-out versions of the project
** from the local repository.
*/
#include "config.h"
#include "add.h"
#include <assert.h>
#include <dirent.h>

/*
** Set to true if files whose names begin with "." should be
** included when processing a recursive "add" command.
*/
static int includeDotFiles = 0;

/*
** This routine returns the names of files in a working checkout that
** are created by Fossil itself, and hence should not be added, deleted,
** or merge, and should be omitted from "clean" and "extra" lists.
**
** Return the N-th name.  The first name has N==0.  When all names have
** been used, return 0.
*/
const char *fossil_reserved_name(int N){
  /* Possible names of the local per-checkout database file and
  ** its associated journals
  */
  static const char *azName[] = {
     "_FOSSIL_",
     "_FOSSIL_-journal",
     "_FOSSIL_-wal",
     "_FOSSIL_-shm",
     ".fos",
     ".fos-journal",
     ".fos-wal",
     ".fos-shm",
  };

  /* Names of auxiliary files generated by SQLite when the "manifest"
  ** properity is enabled
  */
  static const char *azManifest[] = {
     "manifest",
     "manifest.uuid",
  };

  if( N>=0 && N<count(azName) ) return azName[N];
  if( N>=count(azName) && N<count(azName)+count(azManifest)
      && db_get_boolean("manifest",0) ){
    return azManifest[N-count(azName)];
  }
  return 0;
}

/*
** Return a list of all reserved filenames as an SQL list.
*/
const char *fossil_all_reserved_names(void){
  static char *zAll = 0;
  if( zAll==0 ){
    Blob x;
    int i;
    const char *z;
    blob_zero(&x);
    for(i=0; (z = fossil_reserved_name(i))!=0; i++){
      if( i>0 ) blob_append(&x, ",", 1);
      blob_appendf(&x, "'%s'", z);
    }
    zAll = blob_str(&x);
  }
  return zAll;
}


/*
** The pIgnore statement is query of the form:
**
**      SELECT (:x GLOB ... OR :x GLOB ... OR ...)
**
** In other words, it is a query that returns true if the :x value
** should be ignored.  Evaluate the query and return true to ignore
** and false to not ignore.
**
** If pIgnore is NULL, then do not ignore.
*/
static int shouldBeIgnored(Stmt *pIgnore, const char *zName){
  int rc = 0;
  if( pIgnore ){
    db_bind_text(pIgnore, ":x", zName);
    db_step(pIgnore);
    rc = db_column_int(pIgnore, 0);
    db_reset(pIgnore);
  }
  return rc;
}


/*
** Add a single file named zName to the VFILE table with vid.
**
** Omit any file whose name is pOmit.
*/
static void add_one_file(
  const char *zName,   /* Name of file to add */
  int vid,             /* Add to this VFILE */
  Blob *pOmit
){
  Blob pathname;
  const char *zPath;
  int i;
  const char *zReserved;

  file_tree_name(zName, &pathname, 1);
  zPath = blob_str(&pathname);
  for(i=0; (zReserved = fossil_reserved_name(i))!=0; i++){
    if( fossil_strcmp(zPath, zReserved)==0 ) break;
  }
  if( zReserved || (pOmit && blob_compare(&pathname, pOmit)==0) ){
    fossil_warning("cannot add %s", zPath);
  }else{
    if( !file_is_simple_pathname(zPath) ){
      fossil_fatal("filename contains illegal characters: %s", zPath);
    }
#if defined(_WIN32)
    if( db_exists("SELECT 1 FROM vfile"
                  " WHERE pathname=%Q COLLATE nocase", zPath) ){
      db_multi_exec("UPDATE vfile SET deleted=0"
                    " WHERE pathname=%Q COLLATE nocase", zPath);
    }
#else
    if( db_exists("SELECT 1 FROM vfile WHERE pathname=%Q", zPath) ){
      db_multi_exec("UPDATE vfile SET deleted=0 WHERE pathname=%Q", zPath);
    }
#endif
    else{
      db_multi_exec(
        "INSERT INTO vfile(vid,deleted,rid,mrid,pathname)"
        "VALUES(%d,0,0,0,%Q)", vid, zPath);
    }
    printf("ADDED  %s\n", zPath);
  }
  blob_reset(&pathname);
}

/*
** All content of the zDir directory to the SFILE table.
*/
void add_directory_content(const char *zDir, Stmt *pIgnore){
  DIR *d;
  int origSize;
  struct dirent *pEntry;
  Blob path;

  blob_zero(&path);
  blob_append(&path, zDir, -1);
  origSize = blob_size(&path);
  d = opendir(zDir);
  if( d ){
    while( (pEntry=readdir(d))!=0 ){
      char *zPath;
      if( pEntry->d_name[0]=='.' ){
        if( !includeDotFiles ) continue;
        if( pEntry->d_name[1]==0 ) continue;
        if( pEntry->d_name[1]=='.' && pEntry->d_name[2]==0 ) continue;
      }
      blob_appendf(&path, "/%s", pEntry->d_name);
      zPath = blob_str(&path);
      if( shouldBeIgnored(pIgnore, zPath) ){
        /* Noop */
      }else if( file_isdir(zPath)==1 ){
        add_directory_content(zPath, pIgnore);
      }else if( file_isfile_or_link(zPath) ){
        db_multi_exec("INSERT INTO sfile VALUES(%Q)", zPath);
      }
      blob_resize(&path, origSize);
    }
  }
  closedir(d);
  blob_reset(&path);
}

/*
** Add all content of a directory.
*/
void add_directory(const char *zDir, int vid, Blob *pOmit, Stmt *pIgnore){
  Stmt q;
  add_directory_content(zDir, pIgnore);
  db_prepare(&q, "SELECT x FROM sfile ORDER BY x");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zName = db_column_text(&q, 0);
    add_one_file(zName, vid, pOmit);
  }
  db_finalize(&q);
  db_multi_exec("DELETE FROM sfile");
}

/*
** COMMAND: add
**
** Usage: %fossil add FILE...
**
** Make arrangements to add one or more files to the current checkout
** at the next commit.
**
** When adding files recursively, filenames that begin with "." are
** excluded by default.  To include such files, add the "--dotfiles"
** option to the command-line.
**
** The --ignore option overrides the "ignore-glob" setting.  See
** documentation on the "setting" command for further information.
*/
void add_cmd(void){
  int i;
  int vid;
  const char *zIgnoreFlag;
  Blob repo;
  Stmt ignoreTest;     /* Test to see if a name should be ignored */
  Stmt *pIgnore;       /* Pointer to ignoreTest or to NULL */

  zIgnoreFlag = find_option("ignore",0,1);
  includeDotFiles = find_option("dotfiles",0,0)!=0;
  db_must_be_within_tree();
  if( zIgnoreFlag==0 ){
    zIgnoreFlag = db_get("ignore-glob", 0);
  }
  vid = db_lget_int("checkout",0);
  if( vid==0 ){
    fossil_panic("no checkout to add to");
  }
  db_begin_transaction();
  if( !file_tree_name(g.zRepositoryName, &repo, 0) ){
    blob_zero(&repo);
  }
  db_multi_exec("CREATE TEMP TABLE sfile(x TEXT PRIMARY KEY)");
#if defined(_WIN32)
  db_multi_exec(
     "CREATE INDEX IF NOT EXISTS vfile_pathname "
     "  ON vfile(pathname COLLATE nocase)"
  );
#endif
  if( zIgnoreFlag && zIgnoreFlag[0] ){
    db_prepare(&ignoreTest, "SELECT %s", glob_expr(":x", zIgnoreFlag));
    pIgnore = &ignoreTest;
  }else{
    pIgnore = 0;
  }
  for(i=2; i<g.argc; i++){
    char *zName;
    int isDir;

    zName = mprintf("%/", g.argv[i]);
    isDir = file_isdir(zName);
    if( isDir==1 ){
      add_directory(zName, vid, &repo, pIgnore);
    }else if( isDir==0 ){
      fossil_fatal("not found: %s", zName);
    }else if( access(zName, R_OK) ){
      fossil_fatal("cannot open %s", zName);
    }else{
      add_one_file(zName, vid, &repo);
    }
    free(zName);
  }
  if( pIgnore ) db_finalize(pIgnore);
  db_end_transaction(0);
}

/*
** Remove all contents of zDir
*/
void del_directory_content(const char *zDir){
  DIR *d;
  int origSize;
  struct dirent *pEntry;
  Blob path;

  blob_zero(&path);
  blob_append(&path, zDir, -1);
  origSize = blob_size(&path);
  d = opendir(zDir);
  if( d ){
    while( (pEntry=readdir(d))!=0 ){
      char *zPath;
      if( pEntry->d_name[0]=='.'){
        if( !includeDotFiles ) continue;
        if( pEntry->d_name[1]==0 ) continue;
        if( pEntry->d_name[1]=='.' && pEntry->d_name[2]==0 ) continue;
      }
      blob_appendf(&path, "/%s", pEntry->d_name);
      zPath = blob_str(&path);
      if( file_isdir(zPath)==1 ){
        del_directory_content(zPath);
      }else if( file_isfile_or_link(zPath) ){
        char *zFilePath;
        Blob pathname;
        file_tree_name(zPath, &pathname, 1);
        zFilePath = blob_str(&pathname);
        if( !db_exists(
            "SELECT 1 FROM vfile WHERE pathname=%Q AND NOT deleted", zFilePath)
        ){
          printf("SKIPPED  %s\n", zPath);
        }else{
          db_multi_exec("UPDATE vfile SET deleted=1 WHERE pathname=%Q", zPath);
          printf("DELETED  %s\n", zPath);
        }
        blob_reset(&pathname);
      }
      blob_resize(&path, origSize);
    }
  }
  closedir(d);
  blob_reset(&path);
}

/*
** COMMAND: rm
** COMMAND: delete
**
** Usage: %fossil rm FILE...
**    or: %fossil delete FILE...
**
** Remove one or more files from the tree.
**
** This command does not remove the files from disk.  It just marks the
** files as no longer being part of the project.  In other words, future
** changes to the named files will not be versioned.
*/
void delete_cmd(void){
  int i;
  int vid;

  db_must_be_within_tree();
  vid = db_lget_int("checkout", 0);
  if( vid==0 ){
    fossil_panic("no checkout to remove from");
  }
  db_begin_transaction();
  for(i=2; i<g.argc; i++){
    char *zName;

    zName = mprintf("%/", g.argv[i]);
    if( file_isdir(zName) == 1 ){
      del_directory_content(zName);
    } else {
      char *zPath;
      Blob pathname;
      file_tree_name(zName, &pathname, 1);
      zPath = blob_str(&pathname);
      if( !db_exists(
               "SELECT 1 FROM vfile WHERE pathname=%Q AND NOT deleted", zPath) ){
        fossil_fatal("not in the repository: %s", zName);
      }else{
        db_multi_exec("UPDATE vfile SET deleted=1 WHERE pathname=%Q", zPath);
        printf("DELETED  %s\n", zPath);
      }
      blob_reset(&pathname);
    }
    free(zName);
  }
  db_multi_exec("DELETE FROM vfile WHERE deleted AND rid=0");
  db_end_transaction(0);
}

/*
** COMMAND: addremove
**
** Usage: %fossil addremove ?--dotfiles? ?--ignore GLOBPATTERN? ?--test?
**
** Do all necessary "add" and "rm" commands to synchronize the repository
** with the content of the working checkout
**
**  *  All files in the checkout but not in the repository (that is,
**     all files displayed using the "extra" command) are added as
**     if by the "add" command.
**
**  *  All files in the repository but missing from the checkout (that is,
**     all files that show as MISSING with the "status" command) are
**     removed as if by the "rm" command.
**
** The command does not "commit".  You must run the "commit" separately
** as a separate step.
**
** Files and directories whose names begin with "." are ignored unless
** the --dotfiles option is used.
**
** The --ignore option overrides the "ignore-glob" setting.  See
** documentation on the "setting" command for further information.
**
** The --test option shows what would happen without actually doing anything.
**
** This command can be used to track third party software.
*/
void import_cmd(void){
  Blob path;
  const char *zIgnoreFlag = find_option("ignore",0,1);
  int allFlag = find_option("dotfiles",0,0)!=0;
  int isTest = find_option("test",0,0)!=0;
  int n;
  Stmt q;
  int vid;
  Blob repo;
  int nAdd = 0;
  int nDelete = 0;

  db_must_be_within_tree();
  if( zIgnoreFlag==0 ){
    zIgnoreFlag = db_get("ignore-glob", 0);
  }
  vid = db_lget_int("checkout",0);
  if( vid==0 ){
    fossil_panic("no checkout to add to");
  }
  db_begin_transaction();
  db_multi_exec("CREATE TEMP TABLE sfile(x TEXT PRIMARY KEY)");
  n = strlen(g.zLocalRoot);
  blob_init(&path, g.zLocalRoot, n-1);
  /* now we read the complete file structure into a temp table */
  vfile_scan(0, &path, blob_size(&path), allFlag);
  if( file_tree_name(g.zRepositoryName, &repo, 0) ){
    db_multi_exec("DELETE FROM sfile WHERE x=%B", &repo);
  }

  /* step 1: search for extra files */
  db_prepare(&q, 
      "SELECT x, %Q || x FROM sfile"
      " WHERE x NOT IN (%s)"
      "   AND NOT %s"
      " ORDER BY 1",
      g.zLocalRoot,
      fossil_all_reserved_names(),
      glob_expr("x", zIgnoreFlag)
  );
  while( db_step(&q)==SQLITE_ROW ){
    add_one_file(db_column_text(&q, 1), vid, 0);
    nAdd++;
  }
  db_finalize(&q);
  /* step 2: search for missing files */
  db_prepare(&q, 
      "SELECT pathname,%Q || pathname,deleted FROM vfile"
      " WHERE deleted!=1"
      " ORDER BY 1",
      g.zLocalRoot
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char * zFile;
    const char * zPath;

    zFile = db_column_text(&q, 0);
    zPath = db_column_text(&q, 1);
    if( !file_isfile_or_link(zPath) ){
      if( !isTest ){
        db_multi_exec("UPDATE vfile SET deleted=1 WHERE pathname=%Q", zFile);
      }
      printf("DELETED  %s\n", zFile);
      nDelete++;
    }
  }
  db_finalize(&q);
  /* show cmmand summary */
  printf("added %d files, deleted %d files\n", nAdd, nDelete);

  db_end_transaction(isTest);
}


/*
** Rename a single file.
**
** The original name of the file is zOrig.  The new filename is zNew.
*/
static void mv_one_file(int vid, const char *zOrig, const char *zNew){
  printf("RENAME %s %s\n", zOrig, zNew);
  db_multi_exec(
    "UPDATE vfile SET pathname='%s' WHERE pathname='%s' AND vid=%d",
    zNew, zOrig, vid
  );
}

/*
** COMMAND: mv
** COMMAND: rename
**
** Usage: %fossil mv|rename OLDNAME NEWNAME
**    or: %fossil mv|rename OLDNAME... DIR
**
** Move or rename one or more files within the tree
**
** This command does not rename the files on disk.  This command merely
** records the fact that filenames have changed so that appropriate notations
** can be made at the next commit/checkin.
*/
void mv_cmd(void){
  int i;
  int vid;
  char *zDest;
  Blob dest;
  Stmt q;

  db_must_be_within_tree();
  vid = db_lget_int("checkout", 0);
  if( vid==0 ){
    fossil_panic("no checkout rename files in");
  }
  if( g.argc<4 ){
    usage("OLDNAME NEWNAME");
  }
  zDest = g.argv[g.argc-1];
  db_begin_transaction();
  file_tree_name(zDest, &dest, 1);
  db_multi_exec(
    "UPDATE vfile SET origname=pathname WHERE origname IS NULL;"
  );
  db_multi_exec(
    "CREATE TEMP TABLE mv(f TEXT UNIQUE ON CONFLICT IGNORE, t TEXT);"
  );
  if( file_isdir(zDest)!=1 ){
    Blob orig;
    if( g.argc!=4 ){
      usage("OLDNAME NEWNAME");
    }
    file_tree_name(g.argv[2], &orig, 1);
    db_multi_exec(
      "INSERT INTO mv VALUES(%B,%B)", &orig, &dest
    );
  }else{
    if( blob_eq(&dest, ".") ){
      blob_reset(&dest);
    }else{
      blob_append(&dest, "/", 1);
    }
    for(i=2; i<g.argc-1; i++){
      Blob orig;
      char *zOrig;
      int nOrig;
      file_tree_name(g.argv[i], &orig, 1);
      zOrig = blob_str(&orig);
      nOrig = blob_size(&orig);
      db_prepare(&q,
         "SELECT pathname FROM vfile"
         " WHERE vid=%d"
         "   AND (pathname='%s' OR pathname GLOB '%s/*')"
         " ORDER BY 1",
         vid, zOrig, zOrig
      );
      while( db_step(&q)==SQLITE_ROW ){
        const char *zPath = db_column_text(&q, 0);
        int nPath = db_column_bytes(&q, 0);
        const char *zTail;
        if( nPath==nOrig ){
          zTail = file_tail(zPath);
        }else{
          zTail = &zPath[nOrig+1];
        }
        db_multi_exec(
          "INSERT INTO mv VALUES('%s','%s%s')",
          zPath, blob_str(&dest), zTail
        );
      }
      db_finalize(&q);
    }
  }
  db_prepare(&q, "SELECT f, t FROM mv ORDER BY f");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zFrom = db_column_text(&q, 0);
    const char *zTo = db_column_text(&q, 1);
    mv_one_file(vid, zFrom, zTo);
  }
  db_finalize(&q);
  db_end_transaction(0);
}
