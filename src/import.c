/*
** Copyright (c) 2010 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@sqlite.org
**
*******************************************************************************
**
** This file contains code used to import the content of a Git
** repository in the git-fast-import format as a new Fossil
** repository.
*/
#include "config.h"
#include "import.h"
#include <assert.h>

#if INTERFACE
/*
** A single file change record.
*/
struct ImportFile {
  char *zName;           /* Name of a file */
  char *zUuid;           /* UUID of the file */
  char *zPrior;          /* Prior name if the name was changed */
  char isFrom;           /* True if obtained from the parent */
  char isExe;            /* True if executable */
  char isLink;           /* True if symlink */
};
#endif


/*
** State information about an on-going fast-import parse.
*/
static struct {
  void (*xFinish)(void);      /* Function to finish a prior record */
  int nData;                  /* Bytes of data */
  char *zTag;                 /* Name of a tag */
  char *zBranch;              /* Name of a branch for a commit */
  char *zPrevBranch;          /* The branch of the previous check-in */
  char *aData;                /* Data content */
  char *zMark;                /* The current mark */
  char *zDate;                /* Date/time stamp */
  char *zUser;                /* User name */
  char *zComment;             /* Comment of a commit */
  char *zFrom;                /* from value as a UUID */
  char *zPrevCheckin;         /* Name of the previous check-in */
  char *zFromMark;            /* The mark of the "from" field */
  int nMerge;                 /* Number of merge values */
  int nMergeAlloc;            /* Number of slots in azMerge[] */
  char **azMerge;             /* Merge values */
  int nFile;                  /* Number of aFile values */
  int nFileAlloc;             /* Number of slots in aFile[] */
  ImportFile *aFile;          /* Information about files in a commit */
  int fromLoaded;             /* True zFrom content loaded into aFile[] */
  int hasLinks;               /* True if git repository contains symlinks */
  int tagCommit;              /* True if the commit adds a tag */
} gg;

/*
** Duplicate a string.
*/
char *fossil_strdup(const char *zOrig){
  char *z = 0;
  if( zOrig ){
    int n = strlen(zOrig);
    z = fossil_malloc( n+1 );
    memcpy(z, zOrig, n+1);
  }
  return z;
}

/*
** A no-op "xFinish" method
*/
static void finish_noop(void){}

/*
** Deallocate the state information.
**
** The azMerge[] and aFile[] arrays are zeroed by allocated space is
** retained unless the freeAll flag is set.
*/
static void import_reset(int freeAll){
  int i;
  gg.xFinish = 0;
  fossil_free(gg.zTag); gg.zTag = 0;
  fossil_free(gg.zBranch); gg.zBranch = 0;
  fossil_free(gg.aData); gg.aData = 0;
  fossil_free(gg.zMark); gg.zMark = 0;
  fossil_free(gg.zDate); gg.zDate = 0;
  fossil_free(gg.zUser); gg.zUser = 0;
  fossil_free(gg.zComment); gg.zComment = 0;
  fossil_free(gg.zFrom); gg.zFrom = 0;
  fossil_free(gg.zFromMark); gg.zFromMark = 0;
  for(i=0; i<gg.nMerge; i++){
    fossil_free(gg.azMerge[i]); gg.azMerge[i] = 0;
  }
  gg.nMerge = 0;
  for(i=0; i<gg.nFile; i++){
    fossil_free(gg.aFile[i].zName);
    fossil_free(gg.aFile[i].zUuid);
    fossil_free(gg.aFile[i].zPrior);
  }
  memset(gg.aFile, 0, gg.nFile*sizeof(gg.aFile[0]));
  gg.nFile = 0;
  if( freeAll ){
    fossil_free(gg.zPrevBranch);
    fossil_free(gg.zPrevCheckin);
    fossil_free(gg.azMerge);
    fossil_free(gg.aFile);
    memset(&gg, 0, sizeof(gg));
  }
  gg.xFinish = finish_noop;
}

/*
** Insert an artifact into the BLOB table if it isn't there already.
** If zMark is not zero, create a cross-reference from that mark back
** to the newly inserted artifact.
**
** If saveUuid is true, then pContent is a commit record.  Record its
** UUID in gg.zPrevCheckin.
*/
static int fast_insert_content(Blob *pContent, const char *zMark, int saveUuid){
  Blob hash;
  Blob cmpr;
  int rid;

  sha1sum_blob(pContent, &hash);
  rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &hash);
  if( rid==0 ){
    static Stmt ins;
    db_static_prepare(&ins,
        "INSERT INTO blob(uuid, size, content) VALUES(:uuid, :size, :content)"
    );
    db_bind_text(&ins, ":uuid", blob_str(&hash));
    db_bind_int(&ins, ":size", gg.nData);
    blob_compress(pContent, &cmpr);
    db_bind_blob(&ins, ":content", &cmpr);
    db_step(&ins);
    db_reset(&ins);
    blob_reset(&cmpr);
    rid = db_last_insert_rowid();
  }
  if( zMark ){
    db_multi_exec(
        "INSERT OR IGNORE INTO xmark(tname, trid, tuuid)"
        "VALUES(%Q,%d,%B)",
        zMark, rid, &hash
    );
    db_multi_exec(
        "INSERT OR IGNORE INTO xmark(tname, trid, tuuid)"
        "VALUES(%B,%d,%B)",
        &hash, rid, &hash
    );
  }
  if( saveUuid ){
    fossil_free(gg.zPrevCheckin);
    gg.zPrevCheckin = fossil_strdup(blob_str(&hash));
  }
  blob_reset(&hash);
  return rid;
}

/*
** Use data accumulated in gg from a "blob" record to add a new file
** to the BLOB table.
*/
static void finish_blob(void){
  Blob content;
  blob_init(&content, gg.aData, gg.nData);
  fast_insert_content(&content, gg.zMark, 0);
  blob_reset(&content);
  import_reset(0);
}

/*
** Use data accumulated in gg from a "tag" record to add a new
** control artifact to the BLOB table.
*/
static void finish_tag(void){
  Blob record, cksum;
  if( gg.zDate && gg.zTag && gg.zFrom && gg.zUser ){
    blob_zero(&record);
    blob_appendf(&record, "D %s\n", gg.zDate);
    blob_appendf(&record, "T +%F %s\n", gg.zTag, gg.zFrom);
    blob_appendf(&record, "U %F\n", gg.zUser);
    md5sum_blob(&record, &cksum);
    blob_appendf(&record, "Z %b\n", &cksum);
    fast_insert_content(&record, 0, 0);
    blob_reset(&record);
    blob_reset(&cksum);
  }
  import_reset(0);
}

/*
** Compare two ImportFile objects for sorting
*/
static int mfile_cmp(const void *pLeft, const void *pRight){
  const ImportFile *pA = (const ImportFile*)pLeft;
  const ImportFile *pB = (const ImportFile*)pRight;
  return fossil_strcmp(pA->zName, pB->zName);
}

/*
** Compare two strings for sorting.
*/
static int string_cmp(const void *pLeft, const void *pRight){
  const char *zLeft = *(const char **)pLeft;
  const char *zRight = *(const char **)pRight;
  return fossil_strcmp(zLeft, zRight);
}

/* Forward reference */
static void import_prior_files(void);

/*
** Use data accumulated in gg from a "commit" record to add a new
** manifest artifact to the BLOB table.
*/
static void finish_commit(void){
  int i;
  char *zFromBranch;
  char *aTCard[4];                /* Array of T cards for manifest */
  int nTCard = 0;                 /* Entries used in aTCard[] */
  Blob record, cksum;

  import_prior_files();
  qsort(gg.aFile, gg.nFile, sizeof(gg.aFile[0]), mfile_cmp);
  blob_zero(&record);
  blob_appendf(&record, "C %F\n", gg.zComment);
  blob_appendf(&record, "D %s\n", gg.zDate);
  for(i=0; i<gg.nFile; i++){
    const char *zUuid = gg.aFile[i].zUuid;
    if( zUuid==0 ) continue;
    blob_appendf(&record, "F %F %s", gg.aFile[i].zName, zUuid);
    if( gg.aFile[i].isExe ){
      blob_append(&record, " x\n", 3);
    }else if( gg.aFile[i].isLink ){
      blob_append(&record, " l\n", 3);
      gg.hasLinks = 1;
    }else{
      blob_append(&record, "\n", 1);
    }
  }
  if( gg.zFrom ){
    blob_appendf(&record, "P %s", gg.zFrom);
    for(i=0; i<gg.nMerge; i++){
      blob_appendf(&record, " %s", gg.azMerge[i]);
    }
    blob_append(&record, "\n", 1);
    zFromBranch = db_text(0, "SELECT brnm FROM xbranch WHERE tname=%Q",
                              gg.zFromMark);
  }else{
    zFromBranch = 0;
  }

  /* Add the required "T" cards to the manifest. Make sure they are added
  ** in sorted order and without any duplicates. Otherwise, fossil will not
  ** recognize the document as a valid manifest. */
  if( !gg.tagCommit && fossil_strcmp(zFromBranch, gg.zBranch)!=0 ){
    aTCard[nTCard++] = mprintf("T *branch * %F\n", gg.zBranch);
    aTCard[nTCard++] = mprintf("T *sym-%F *\n", gg.zBranch);
    if( zFromBranch ){
      aTCard[nTCard++] = mprintf("T -sym-%F *\n", zFromBranch);
    }
  }
  if( gg.zFrom==0 ){
    aTCard[nTCard++] = mprintf("T *sym-trunk *\n");
  }
  qsort(aTCard, nTCard, sizeof(char *), string_cmp);
  for(i=0; i<nTCard; i++){
    if( i==0 || fossil_strcmp(aTCard[i-1], aTCard[i]) ){
      blob_appendf(&record, "%s", aTCard[i]);
    }
  }
  for(i=0; i<nTCard; i++) free(aTCard[i]);

  free(zFromBranch);
  db_multi_exec("INSERT INTO xbranch(tname, brnm) VALUES(%Q,%Q)",
                gg.zMark, gg.zBranch);
  blob_appendf(&record, "U %F\n", gg.zUser);
  md5sum_blob(&record, &cksum);
  blob_appendf(&record, "Z %b\n", &cksum);
  fast_insert_content(&record, gg.zMark, 1);
  blob_reset(&record);
  blob_reset(&cksum);

  /* The "git fast-export" command might output multiple "commit" lines
  ** that reference a tag using "refs/tags/TAGNAME".  The tag should only
  ** be applied to the last commit that is output.  The problem is we do not
  ** know at this time if the current commit is the last one to hold this
  ** tag or not.  So make an entry in the XTAG table to record this tag
  ** but overwrite that entry if a later instance of the same tag appears.
  **
  ** This behavior seems like a bug in git-fast-export, but it is easier
  ** to work around the problem than to fix git-fast-export.
  */
  if( gg.tagCommit && gg.zDate && gg.zUser && gg.zFrom ){
    blob_appendf(&record, "D %s\n", gg.zDate);
    blob_appendf(&record, "T +sym-%F %s\n", gg.zBranch, gg.zPrevCheckin);
    blob_appendf(&record, "U %F\n", gg.zUser);
    md5sum_blob(&record, &cksum);
    blob_appendf(&record, "Z %b\n", &cksum);
    db_multi_exec(
       "INSERT OR REPLACE INTO xtag(tname, tcontent)"
       " VALUES(%Q,%Q)", gg.zBranch, blob_str(&record)
    );
    blob_reset(&record);
    blob_reset(&cksum);
  }

  fossil_free(gg.zPrevBranch);
  gg.zPrevBranch = gg.zBranch;
  gg.zBranch = 0;
  import_reset(0);
}

/*
** Turn the first \n in the input string into a \000
*/
static void trim_newline(char *z){
  while( z[0] && z[0]!='\n' ){ z++; }
  z[0] = 0;
}

/*
** Get a token from a line of text.  Return a pointer to the first
** character of the token and zero-terminate the token.  Make
** *pzIn point to the first character past the end of the zero
** terminator, or at the zero-terminator at EOL.
*/
static char *next_token(char **pzIn){
  char *z = *pzIn;
  int i;
  if( z[0]==0 ) return z;
  for(i=0; z[i] && z[i]!=' ' && z[i]!='\n'; i++){}
  if( z[i] ){
    z[i] = 0;
    *pzIn = &z[i+1];
  }else{
    *pzIn = &z[i];
  }
  return z;
}

/*
** Return a token that is all text up to (but omitting) the next \n
** or \r\n.
*/
static char *rest_of_line(char **pzIn){
  char *z = *pzIn;
  int i;
  if( z[0]==0 ) return z;
  for(i=0; z[i] && z[i]!='\r' && z[i]!='\n'; i++){}
  if( z[i] ){
    if( z[i]=='\r' && z[i+1]=='\n' ){
      z[i] = 0;
      i++;
    }else{
      z[i] = 0;
    }
    *pzIn = &z[i+1];
  }else{
    *pzIn = &z[i];
  }
  return z;
}

/*
** Convert a "mark" or "committish" into the UUID.
*/
static char *resolve_committish(const char *zCommittish){
  char *zRes;

  zRes = db_text(0, "SELECT tuuid FROM xmark WHERE tname=%Q", zCommittish);
  return zRes;
}

/*
** Create a new entry in the gg.aFile[] array
*/
static ImportFile *import_add_file(void){
  ImportFile *pFile;
  if( gg.nFile>=gg.nFileAlloc ){
    gg.nFileAlloc = gg.nFileAlloc*2 + 100;
    gg.aFile = fossil_realloc(gg.aFile, gg.nFileAlloc*sizeof(gg.aFile[0]));
  }
  pFile = &gg.aFile[gg.nFile++];
  memset(pFile, 0, sizeof(*pFile));
  return pFile;
}


/*
** Load all file information out of the gg.zFrom check-in
*/
static void import_prior_files(void){
  Manifest *p;
  int rid;
  ManifestFile *pOld;
  ImportFile *pNew;
  if( gg.fromLoaded ) return;
  gg.fromLoaded = 1;
  if( gg.zFrom==0 && gg.zPrevCheckin!=0
   && fossil_strcmp(gg.zBranch, gg.zPrevBranch)==0
  ){
     gg.zFrom = gg.zPrevCheckin;
     gg.zPrevCheckin = 0;
  }
  if( gg.zFrom==0 ) return;
  rid = fast_uuid_to_rid(gg.zFrom);
  if( rid==0 ) return;
  p = manifest_get(rid, CFTYPE_MANIFEST, 0);
  if( p==0 ) return;
  manifest_file_rewind(p);
  while( (pOld = manifest_file_next(p, 0))!=0 ){
    pNew = import_add_file();
    pNew->zName = fossil_strdup(pOld->zName);
    pNew->isExe = pOld->zPerm && strstr(pOld->zPerm, "x")!=0;
    pNew->isLink = pOld->zPerm && strstr(pOld->zPerm, "l")!=0;
    pNew->zUuid = fossil_strdup(pOld->zUuid);
    pNew->isFrom = 1;
  }
  manifest_destroy(p);
}

/*
** Locate a file in the gg.aFile[] array by its name.  Begin the search
** with the *pI-th file.  Update *pI to be one past the file found.
** Do not search past the mx-th file.
*/
static ImportFile *import_find_file(const char *zName, int *pI, int mx){
  int i = *pI;
  int nName = strlen(zName);
  while( i<mx ){
    const char *z = gg.aFile[i].zName;
    if( strncmp(zName, z, nName)==0 && (z[nName]==0 || z[nName]=='/') ){
      *pI = i+1;
      return &gg.aFile[i];
    }
    i++;
  }
  return 0;
}

/*
** Dequote a fast-export filename.  Filenames are normally unquoted.  But
** if the contain some obscure special characters, quotes might be added.
*/
static void dequote_git_filename(char *zName){
  int n, i, j;
  if( zName==0 || zName[0]!='"' ) return;
  n = (int)strlen(zName);
  if( zName[n-1]!='"' ) return;
  for(i=0, j=1; j<n-1; j++){
    char c = zName[j];
    if( c=='\\' ) c = zName[++j];
    zName[i++] = c;
  }
  zName[i] = 0;
}


/*
** Read the git-fast-import format from pIn and insert the corresponding
** content into the database.
*/
static void git_fast_import(FILE *pIn){
  ImportFile *pFile, *pNew;
  int i, mx;
  char *z;
  char *zUuid;
  char *zName;
  char *zPerm;
  char *zFrom;
  char *zTo;
  char zLine[1000];

  gg.xFinish = finish_noop;
  while( fgets(zLine, sizeof(zLine), pIn) ){
    if( zLine[0]=='\n' || zLine[0]=='#' ) continue;
    if( strncmp(zLine, "blob", 4)==0 ){
      gg.xFinish();
      gg.xFinish = finish_blob;
    }else
    if( strncmp(zLine, "commit ", 7)==0 ){
      gg.xFinish();
      gg.xFinish = finish_commit;
      trim_newline(&zLine[7]);
      z = &zLine[7];

      /* The argument to the "commit" line might match either of these
      ** patterns:
      **
      **   (A)  refs/heads/BRANCHNAME
      **   (B)  refs/tags/TAGNAME
      **
      ** If pattern A is used, then the branchname used is as shown.
      ** Except, the "master" branch which is the default branch name in
      ** Git is changed to "trunk" which is the default name in Fossil.
      ** If the pattern is B, then the new commit should be on the same
      ** branch as its parent.  And, we might need to add the TAGNAME
      ** tag to the new commit.  However, if there are multiple instances
      ** of pattern B with the same TAGNAME, then only put the tag on the
      ** last commit that holds that tag.
      **
      ** None of the above is explained in the git-fast-export
      ** documentation.  We had to figure it out via trial and error.
      */
      for(i=strlen(z)-1; i>=0 && z[i]!='/'; i--){}
      gg.tagCommit = strncmp(&z[i-4], "tags", 4)==0;  /* True for pattern B */
      if( z[i+1]!=0 ) z += i+1;
      if( fossil_strcmp(z, "master")==0 ) z = "trunk";
      gg.zBranch = fossil_strdup(z);
      gg.fromLoaded = 0;
    }else
    if( strncmp(zLine, "tag ", 4)==0 ){
      gg.xFinish();
      gg.xFinish = finish_tag;
      trim_newline(&zLine[4]);
      gg.zTag = fossil_strdup(&zLine[4]);
    }else
    if( strncmp(zLine, "reset ", 4)==0 ){
      gg.xFinish();
    }else
    if( strncmp(zLine, "checkpoint", 10)==0 ){
      gg.xFinish();
    }else
    if( strncmp(zLine, "feature", 7)==0 ){
      gg.xFinish();
    }else
    if( strncmp(zLine, "option", 6)==0 ){
      gg.xFinish();
    }else
    if( strncmp(zLine, "progress ", 9)==0 ){
      gg.xFinish();
      trim_newline(&zLine[9]);
      fossil_print("%s\n", &zLine[9]);
      fflush(stdout);
    }else
    if( strncmp(zLine, "data ", 5)==0 ){
      fossil_free(gg.aData); gg.aData = 0;
      gg.nData = atoi(&zLine[5]);
      if( gg.nData ){
        int got;
        gg.aData = fossil_malloc( gg.nData+1 );
        got = fread(gg.aData, 1, gg.nData, pIn);
        if( got!=gg.nData ){
          fossil_fatal("short read: got %d of %d bytes", got, gg.nData);
        }
        gg.aData[got] = 0;
        if( gg.zComment==0 && gg.xFinish==finish_commit ){
          gg.zComment = gg.aData;
          gg.aData = 0;
          gg.nData = 0;
        }
      }
    }else
    if( strncmp(zLine, "author ", 7)==0 ){
      /* No-op */
    }else
    if( strncmp(zLine, "mark ", 5)==0 ){
      trim_newline(&zLine[5]);
      fossil_free(gg.zMark);
      gg.zMark = fossil_strdup(&zLine[5]);
    }else
    if( strncmp(zLine, "tagger ", 7)==0 || strncmp(zLine, "committer ",10)==0 ){
      sqlite3_int64 secSince1970;
      for(i=0; zLine[i] && zLine[i]!='<'; i++){}
      if( zLine[i]==0 ) goto malformed_line;
      z = &zLine[i+1];
      for(i=i+1; zLine[i] && zLine[i]!='>'; i++){}
      if( zLine[i]==0 ) goto malformed_line;
      zLine[i] = 0;
      fossil_free(gg.zUser);
      gg.zUser = fossil_strdup(z);
      secSince1970 = 0;
      for(i=i+2; fossil_isdigit(zLine[i]); i++){
        secSince1970 = secSince1970*10 + zLine[i] - '0';
      }
      fossil_free(gg.zDate);
      gg.zDate = db_text(0, "SELECT datetime(%lld, 'unixepoch')", secSince1970);
      gg.zDate[10] = 'T';
    }else
    if( strncmp(zLine, "from ", 5)==0 ){
      trim_newline(&zLine[5]);
      fossil_free(gg.zFromMark);
      gg.zFromMark = fossil_strdup(&zLine[5]);
      fossil_free(gg.zFrom);
      gg.zFrom = resolve_committish(&zLine[5]);
    }else
    if( strncmp(zLine, "merge ", 6)==0 ){
      trim_newline(&zLine[6]);
      if( gg.nMerge>=gg.nMergeAlloc ){
        gg.nMergeAlloc = gg.nMergeAlloc*2 + 10;
        gg.azMerge = fossil_realloc(gg.azMerge, gg.nMergeAlloc*sizeof(char*));
      }
      gg.azMerge[gg.nMerge] = resolve_committish(&zLine[6]);
      if( gg.azMerge[gg.nMerge] ) gg.nMerge++;
    }else
    if( strncmp(zLine, "M ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zPerm = next_token(&z);
      zUuid = next_token(&z);
      zName = rest_of_line(&z);
      dequote_git_filename(zName);
      i = 0;
      pFile = import_find_file(zName, &i, gg.nFile);
      if( pFile==0 ){
        pFile = import_add_file();
        pFile->zName = fossil_strdup(zName);
      }
      pFile->isExe = (fossil_strcmp(zPerm, "100755")==0);
      pFile->isLink = (fossil_strcmp(zPerm, "120000")==0);
      fossil_free(pFile->zUuid);
      pFile->zUuid = resolve_committish(zUuid);
      pFile->isFrom = 0;
    }else
    if( strncmp(zLine, "D ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zName = rest_of_line(&z);
      dequote_git_filename(zName);
      i = 0;
      while( (pFile = import_find_file(zName, &i, gg.nFile))!=0 ){
        if( pFile->isFrom==0 ) continue;
        fossil_free(pFile->zName);
        fossil_free(pFile->zPrior);
        fossil_free(pFile->zUuid);
        *pFile = gg.aFile[--gg.nFile];
        i--;
      }
    }else
    if( strncmp(zLine, "C ", 2)==0 ){
      int nFrom;
      import_prior_files();
      z = &zLine[2];
      zFrom = next_token(&z);
      zTo = rest_of_line(&z);
      i = 0;
      mx = gg.nFile;
      nFrom = strlen(zFrom);
      while( (pFile = import_find_file(zFrom, &i, mx))!=0 ){
        if( pFile->isFrom==0 ) continue;
        pNew = import_add_file();
        pFile = &gg.aFile[i-1];
        if( strlen(pFile->zName)>nFrom ){
          pNew->zName = mprintf("%s%s", zTo, pFile->zName[nFrom]);
        }else{
          pNew->zName = fossil_strdup(pFile->zName);
        }
        pNew->isExe = pFile->isExe;
        pNew->isLink = pFile->isLink;
        pNew->zUuid = fossil_strdup(pFile->zUuid);
        pNew->isFrom = 0;
      }
    }else
    if( strncmp(zLine, "R ", 2)==0 ){
      int nFrom;
      import_prior_files();
      z = &zLine[2];
      zFrom = next_token(&z);
      zTo = rest_of_line(&z);
      i = 0;
      nFrom = strlen(zFrom);
      while( (pFile = import_find_file(zFrom, &i, gg.nFile))!=0 ){
        if( pFile->isFrom==0 ) continue;
        pNew = import_add_file();
        pFile = &gg.aFile[i-1];
        if( strlen(pFile->zName)>nFrom ){
          pNew->zName = mprintf("%s%s", zTo, pFile->zName[nFrom]);
        }else{
          pNew->zName = fossil_strdup(pFile->zName);
        }
        pNew->zPrior = pFile->zName;
        pNew->isExe = pFile->isExe;
        pNew->isLink = pFile->isLink;
        pNew->zUuid = pFile->zUuid;
        pNew->isFrom = 0;
        gg.nFile--;
        *pFile = *pNew;
        memset(pNew, 0, sizeof(*pNew));
      }
      fossil_fatal("cannot handle R records, use --full-tree");
    }else
    if( strncmp(zLine, "deleteall", 9)==0 ){
      gg.fromLoaded = 1;
    }else
    if( strncmp(zLine, "N ", 2)==0 ){
      /* No-op */
    }else

    {
      goto malformed_line;
    }
  }
  gg.xFinish();
  if( gg.hasLinks ){
    db_set_int("allow-symlinks", 1, 0);
  }
  import_reset(1);
  return;

malformed_line:
  trim_newline(zLine);
  fossil_fatal("bad fast-import line: [%s]", zLine);
  return;
}

typedef struct {
  const char *zKey;
  const char *zVal;
} KeyVal;
typedef struct {
  KeyVal *aHeaders;
  int nHeaders;
  char *pRawProps;
  KeyVal *aProps;
  int nProps;
  Blob content;
  int nContent;
} SvnRecord;

#define svn_find_header(rec, zHeader) \
  svn_find_keyval((rec).aHeaders, (rec).nHeaders, (zHeader))
#define svn_find_prop(rec, zProp) \
  svn_find_keyval((rec).aProps, (rec).nProps, (zProp))
static const char *svn_find_keyval(
  KeyVal *aKeyVal,
  int nKeyVal,
  const char *zKey
){
  int i;
  for(i=0; i<nKeyVal; i++){
    if( fossil_strcmp(aKeyVal[i].zKey, zKey)==0 ){
      return aKeyVal[i].zVal;
    }
  }
  return 0;
}

static void svn_free_rec(SvnRecord *rec){
  int i;
  for(i=0; i<rec->nHeaders; i++){
    fossil_free(rec->aHeaders[i].zKey);
  }
  fossil_free(rec->aHeaders);
  fossil_free(rec->aProps);
  fossil_free(rec->pRawProps);
  blob_reset(&rec->content);  
}

static int svn_read_headers(FILE *pIn, SvnRecord *rec){
  char zLine[1000];

  rec->aHeaders = 0;
  rec->nHeaders = 0;
  while( fgets(zLine, sizeof(zLine), pIn) ){
    if( zLine[0]!='\n' ) break;
  }
  if( feof(pIn) ) return 0;
  do{
    char *sep;
    if( zLine[0]=='\n' ) break;
    rec->nHeaders += 1;
    rec->aHeaders = fossil_realloc(rec->aHeaders,
      sizeof(rec->aHeaders[0])*rec->nHeaders);
    rec->aHeaders[rec->nHeaders-1].zKey = mprintf("%s", zLine);
    sep = strchr(rec->aHeaders[rec->nHeaders-1].zKey, ':');
    if( !sep ){
      trim_newline(zLine);
      fossil_fatal("bad header line: [%s]", zLine);
    }
    *sep = 0;
    rec->aHeaders[rec->nHeaders-1].zVal = sep+1;
    sep = strchr(rec->aHeaders[rec->nHeaders-1].zVal, '\n');
    *sep = 0;
    while(rec->aHeaders[rec->nHeaders-1].zVal
       && fossil_isspace(*(rec->aHeaders[rec->nHeaders-1].zVal)) )
    {
      rec->aHeaders[rec->nHeaders-1].zVal++;
    }
  }while( fgets(zLine, sizeof(zLine), pIn) );
  if( zLine[0]!='\n' ){
      trim_newline(zLine);
      fossil_fatal("svn-dump data ended unexpectedly");
  }
  return 1;
}

static void svn_read_props(FILE *pIn, SvnRecord *rec){
  int nRawProps = 0;
  char *pRawProps;
  const char *zLen;

  rec->pRawProps = 0;
  rec->aProps = 0;
  rec->nProps = 0;
  zLen = svn_find_header(*rec, "Prop-content-length");
  if( zLen ){
    nRawProps = atoi(zLen);
  }
  if( nRawProps ){
    int got;
    char *zLine;
    rec->pRawProps = pRawProps = fossil_malloc( nRawProps );
    got = fread(rec->pRawProps, 1, nRawProps, pIn);
    if( got!=nRawProps ){
      fossil_fatal("short read: got %d of %d bytes", got, nRawProps);
    }
    if( memcmp(&pRawProps[got-10], "PROPS-END\n", 10)!=0 ){
      fossil_fatal("svn-dump data ended unexpectedly");
    }
    zLine = pRawProps;
    while( zLine<(pRawProps+nRawProps-10) ){
      char *eol;
      int propLen;
      if( zLine[0]!='K' ){
        fossil_fatal("svn-dump data format broken");
      }
      propLen = atoi(&zLine[2]);
      eol = strchr(zLine, '\n');
      zLine = eol+1;
      eol = zLine+propLen;
      if( *eol!='\n' ){
        fossil_fatal("svn-dump data format broken");
      }
      *eol = 0;
      rec->nProps += 1;
      rec->aProps = fossil_realloc(rec->aProps,
        sizeof(rec->aProps[0])*rec->nProps);
      rec->aProps[rec->nProps-1].zKey = zLine;
      zLine = eol+1;
      if( zLine[0]!='V' ){
        fossil_fatal("svn-dump data format broken");
      }
      propLen = atoi(&zLine[2]);
      eol = strchr(zLine, '\n');
      zLine = eol+1;
      eol = zLine+propLen;
      if( *eol!='\n' ){
        fossil_fatal("svn-dump data format broken");
      }
      *eol = 0;
      rec->aProps[rec->nProps-1].zVal = zLine;
      zLine = eol+1;
    }
  }
}

static int svn_read_rec(FILE *pIn, SvnRecord *rec){
  const char *zLen;
  int nLen = 0;
  if( svn_read_headers(pIn, rec)==0 ) return 0;
  svn_read_props(pIn, rec);
  blob_zero(&rec->content);
  zLen = svn_find_header(*rec, "Text-content-length");
  if( zLen ){
    nLen = atoi(zLen);
  }
  blob_read_from_channel(&rec->content, pIn, nLen);
  if( blob_size(&rec->content)!=nLen ){
    fossil_fatal("short read: got %d of %d bytes",
      blob_size(&rec->content), nLen
    );
  }
  return 1;
}

static void svn_create_manifests(int flatFlag){
  Blob manifest;
  Stmt qRev;
  Stmt qParent;
  Stmt qFiles;
  Stmt qTags;
  Stmt setUuid;

  if( !flatFlag ){
    db_multi_exec("DELETE FROM xrevisions WHERE tbranch ISNULL;"
                  "UPDATE xrevisions SET tparent=("
                  "  SELECT ifnull(max(trev),-1) FROM xrevisions t"
                  "  WHERE t.trev<xrevisions.trev"
                  "    AND t.tbranch=xrevisions.tbranch"
                  " )"
                  " WHERE tparent ISNULL;"
                  "WITH xprefix AS ( "
                  "  SELECT trev, CASE tbranch WHEN 'trunk' THEN 'trunk/'"
                  "  ELSE 'branches/'||tbranch||'/' END xpref"
                  "  FROM xrevisions"
                  " ), "
                  " x AS (SELECT trev xrev, xpref, length(xpref) xlen "
                  " FROM xprefix) "
                  " UPDATE xfiles SET tpath= "
                  " CASE substr(tpath,1,(SELECT xlen FROM x WHERE xrev=trev))"
                  " WHEN (SELECT xpref FROM x WHERE xrev=trev)"
                  "  THEN substr(tpath,(SELECT xlen FROM x WHERE xrev=trev)+1)"
                  " END;"
                  "DELETE FROM xfiles WHERE tpath ISNULL");
  }else{
    db_multi_exec("UPDATE xrevisions SET tparent=trev-1");
  }
  db_prepare(&qRev, "SELECT trev, tuser, tmsg, ttime, tparent, tbranch"
                    " FROM xrevisions "
                    " ORDER BY trev");
  db_prepare(&qParent, "SELECT tuuid, tbranch FROM xrevisions WHERE trev=:rev");
  db_prepare(&qFiles, "SELECT tpath, uuid, tperm"
                      " FROM xfiles JOIN blob ON xfiles.trid=blob.rid"
                      " WHERE trev=:rev ORDER BY tpath");
  db_prepare(&qTags, "SELECT ttag FROM xtags WHERE trev=:rev");
  db_prepare(&setUuid, "UPDATE xrevisions"
                       " SET tuuid=(SELECT uuid FROM blob WHERE rid=:rid)"
                       " WHERE trev=:rev");
  blob_zero(&manifest);
  while( db_step(&qRev)==SQLITE_ROW ){
    int rev = db_column_int(&qRev, 0);
    const char *zUser = db_column_text(&qRev, 1);
    const char *zMsg = db_column_text(&qRev, 2);
    const char *zTime = db_column_text(&qRev, 3);
    int parentRev = db_column_int(&qRev, 4);
    const char *zBranch = db_column_text(&qRev, 5);
    int rid;
    const char *zParentBranch = 0;
    Blob mcksum;
    blob_reset(&manifest);
    if( zMsg ){
      blob_appendf(&manifest, "C %F\n", zMsg);
    }else{
      blob_append(&manifest, "C (no\\scomment)\n", 16);
    }
    blob_appendf(&manifest, "D %s\n", zTime);
    db_bind_int(&qFiles, ":rev", rev);
    while( db_step(&qFiles)==SQLITE_ROW ){
      const char *zFile = db_column_text(&qFiles, 0);
      const char *zUuid = db_column_text(&qFiles, 1);
      const char *zPerm = db_column_text(&qFiles, 2);
      blob_appendf(&manifest, "F %F %s %s\n", zFile, zUuid, zPerm);
    }
    db_reset(&qFiles);
    if( parentRev>=0 ){
      const char *zParentUuid;
      db_bind_int(&qParent, ":rev", parentRev);
      db_step(&qParent);
      zParentUuid = db_column_text(&qParent, 0);
      blob_appendf(&manifest, "P %s\n", zParentUuid);
      if( !flatFlag ){
        zParentBranch = db_column_text(&qParent, 1);
        if( strcmp(zBranch, zParentBranch)!=0 ){
          blob_appendf(&manifest, "T *branch * %s\n", zBranch);
          blob_appendf(&manifest, "T *sym-%s *\n", zBranch);
          zParentBranch = mprintf("%s", zParentBranch);
        }else{
          zParentBranch = 0;
        }
      }
      db_reset(&qParent);
    }else{
      blob_appendf(&manifest, "T *branch * trunk\n");
      blob_appendf(&manifest, "T *sym-trunk *\n");
    }
    db_bind_int(&qTags, ":rev", rev);
    while( db_step(&qTags)==SQLITE_ROW ){
      const char *zTag = db_column_text(&qTags, 0);
      blob_appendf(&manifest, "T +sym-%s *\n", zTag);
    }
    db_reset(&qTags);
    blob_appendf(&manifest, "T +sym-svn-rev-%d *\n", rev);
    if( zParentBranch ) {
      blob_appendf(&manifest, "T -sym-%s *\n", zParentBranch);
    }
    if( zUser ){
      blob_appendf(&manifest, "U %F\n", zUser);
    }else{
      const char *zUserOvrd = find_option("user-override",0,1);
      blob_appendf(&manifest, "U %F\n",
        zUserOvrd ? zUserOvrd : login_name());
    }
    md5sum_blob(&manifest, &mcksum);
    blob_appendf(&manifest, "Z %b\n", &mcksum);
    blob_reset(&mcksum);

    rid = content_put(&manifest);
    db_bind_int(&setUuid, ":rid", rid);
    db_bind_int(&setUuid, ":rev", rev);
    db_step(&setUuid);
    db_reset(&setUuid);
  }
  db_finalize(&qRev);
  db_finalize(&qParent);
  db_finalize(&qFiles);
  db_finalize(&qTags);
  db_finalize(&setUuid);
}
/*
** Read the svn-dump format from pIn and insert the corresponding
** content into the database.
*/
static void svn_dump_import(FILE *pIn, int flatFlag){
  SvnRecord rec;
  int ver;
  const char *zTemp;
  const char *zUuid;
  char zBranch[200] = {0};
  Stmt insRev;
  Stmt insFile;
  Stmt insTag;
  Stmt delFile;
  Stmt setBranch;
  Stmt setParent;
  int rev = 0;

  /* version */
  if( svn_read_rec(pIn, &rec)
   && (zTemp = svn_find_header(rec, "SVN-fs-dump-format-version")) ){
    ver = atoi(zTemp);
    if( ver!=2 ){
      fossil_fatal("Unknown svn-dump format version: %d", ver);
    }
  }else{
    fossil_fatal("Input is not an svn-dump!");
  }
  svn_free_rec(&rec);
  /* UUID */
  if( !svn_read_rec(pIn, &rec) || !(zUuid = svn_find_header(rec, "UUID")) ){
    fossil_fatal("Missing UUID!");
  }
  svn_free_rec(&rec);
  /* content */
  db_prepare(&insRev,
      "INSERT INTO xrevisions (trev, tuser, tmsg, ttime) "
      "VALUES(:rev, :user, :msg, :time)"
  );
  db_prepare(&insFile,
      "INSERT INTO xfiles (trev, tpath, trid, tperm) "
      "VALUES(:rev, :path, :rid, :perm)"
  );
  db_prepare(&insTag, "INSERT INTO xtags (trev, ttag) VALUES(:rev, :tag)");
  db_prepare(&delFile,
      "DELETE FROM xfiles "
      "WHERE trev=:rev "
      "  AND (tpath=:path OR (tpath>:path||'/' AND tpath<:path||'0'))"
  );
  db_prepare(&setBranch,
      "UPDATE xrevisions SET tbranch=:branch "
      "WHERE trev=:rev"
  );
  db_prepare(&setParent,
      "UPDATE xrevisions SET tparent=:parent "
      "WHERE trev=:rev"
  );
  while( svn_read_rec(pIn, &rec) ){
    if( zTemp = svn_find_header(rec, "Revision-number") ){
      const char *zUser = svn_find_prop(rec, "svn:author");
      const char *zLog = svn_find_prop(rec, "svn:log");
      const char *zDate = svn_find_prop(rec, "svn:date");
      if( !flatFlag ){
        if( *zBranch ){
          db_bind_text(&setBranch, ":branch", zBranch);
          db_bind_int(&setBranch, ":rev", rev);
          db_step(&setBranch);
          db_reset(&setBranch);
          zBranch[0] = 0;
        }
      }
      zDate = date_in_standard_format(zDate);
      rev = atoi(zTemp);
      db_bind_int(&insRev, ":rev", rev);
      db_bind_text(&insRev, ":user", zUser);
      db_bind_text(&insRev, ":msg", zLog);
      db_bind_text(&insRev, ":time", zDate);
      db_step(&insRev);
      db_reset(&insRev);
      fossil_free(zDate);
      if( rev>0 ){
        db_multi_exec("INSERT INTO xfiles (trev, tpath, trid, tperm) "
                      "SELECT %d, tpath, trid, tperm FROM xfiles "
                      "WHERE trev=%d", rev, rev-1);
      }
    }else
    if( zTemp = svn_find_header(rec, "Node-path") ){
      const char *zPath = zTemp;
      const char *zAction = svn_find_header(rec, "Node-action");
      const char *zKind = svn_find_header(rec, "Node-kind");
      const char *zSrcPath = svn_find_header(rec, "Node-copyfrom-path");
      const char *zPerm = svn_find_prop(rec, "svn:executable") ? "x" : 0;
      int srcRev = -1;
      int rid = 0;
      if( zSrcPath ){
        zTemp = svn_find_header(rec, "Node-copyfrom-rev");
        if( zTemp ){
          srcRev = atoi(zTemp);
        }else{
          fossil_fatal("Missing copyfrom-rev");
        }
      }
      if( !flatFlag ){
        if( strncmp(zPath, "branches/", 9)==0 ){
          int i;
          sqlite3_snprintf(sizeof(zBranch), zBranch, "%s", zPath+9);
          for( i=0; zBranch[i]; i++ ){
            if( zBranch[i]=='/' ) zBranch[i]=0;
          }
        }else
        if( strncmp(zPath, "trunk/", 6)==0 ){
          memcpy(zBranch, "trunk", 6);
        }
      }
      if( strncmp(zAction, "delete", 6)==0
       || strncmp(zAction, "replace", 7)==0 )
      {
        db_bind_int(&delFile, ":rev", rev);
        db_bind_text(&delFile, ":path", zPath);
        db_step(&delFile);
        db_reset(&delFile);
      } /* no 'else' here since 'replace' does both a 'delete' and an 'add' */
      if( strncmp(zAction, "add", 3)==0
       || strncmp(zAction, "replace", 7)==0 )
      {
        if( zKind==0 ){
          fossil_fatal("Missing Node-kind");
        }else if( strncmp(zKind, "dir", 3)==0 ){
          if( zSrcPath ){
            db_multi_exec(
              "INSERT INTO xfiles (trev, tpath, trid, tperm) "
              " SELECT %d, %Q||substr(tpath, length(%Q)+1), trid, tperm "
              " FROM xfiles "
              " WHERE trev=%d AND tpath GLOB '%q/*'",
              rev, zPath, zSrcPath, srcRev, zSrcPath
            );
            if( !flatFlag ){
              if( strncmp(zPath, "branches/", 9)==0 ){
                zTemp = zPath+9;
                while( *zTemp && *zTemp!='/' ){ zTemp++; }
                if( *zTemp==0 ){
                  db_bind_int(&setParent, ":parent", srcRev);
                  db_bind_int(&setParent, ":rev", rev);
                  db_step(&setParent);
                  db_reset(&setParent);
                }
              }else if( strncmp(zPath, "tags/", 5)==0 ){
                zTemp = zPath+5;
                db_bind_int(&insTag, ":rev", srcRev);
                db_bind_text(&insTag, ":tag", zTemp);
                db_step(&insTag);
                db_reset(&insTag);
              }
            }
          }
        }else{
          if( blob_size(&rec.content)==0 && zSrcPath ){
            rid = db_int(0,
                         "SELECT trid FROM xfiles WHERE trev=%d AND tpath=%Q",
                         srcRev, zSrcPath);
          }else{
            rid = content_put(&rec.content);
          }
          db_bind_int(&insFile, ":rev", rev);
          db_bind_int(&insFile, ":rid", rid);
          db_bind_text(&insFile, ":path", zPath);
          db_bind_text(&insFile, ":perm", zPerm);
          db_step(&insFile);
          db_reset(&insFile);
        }
      }else
      if( strncmp(zAction, "change", 6)==0 ){
        rid = content_put(&rec.content);
        db_bind_int(&insFile, ":rev", rev);
        db_bind_int(&insFile, ":rid", rid);
        db_bind_text(&insFile, ":path", zPath);
        db_bind_text(&insFile, ":perm", zPerm);
        db_step(&insFile);
        db_reset(&insFile);
      }else
      if( strncmp(zAction, "delete", 6)==0){ /* already did this above */
      }else{
        fossil_fatal("Unknown Node-action");
      }
    }else{
      fossil_fatal("Unknown record type");
    }
    svn_free_rec(&rec);
  }
  if( !flatFlag ){
    if( *zBranch ){
      db_bind_text(&setBranch, ":branch", zBranch);
      db_bind_int(&setBranch, ":rev", rev);
      db_step(&setBranch);
    }
  }
  db_finalize(&insRev);
  db_finalize(&insFile);
  db_finalize(&insTag);
  db_finalize(&delFile);
  db_finalize(&setBranch);
  db_finalize(&setParent);
  svn_create_manifests(flatFlag);
}

/*
** COMMAND: import
**
** Usage: %fossil import FORMAT ?OPTIONS? NEW-REPOSITORY ?INPUT-FILE?
**
** Read interchange format generated by another VCS and use it to
** construct a new Fossil repository named by the NEW-REPOSITORY
** argument.  If no input file is supplied the interchange format
** data is read from standard input.
**
** The following formats are currently understood by this command
**
**   git          Import from the git-fast-export file format
**
**   svn          Import from the svnadmin-dump file format
**                Options:
**                  --flat  The whole dump is a single branch. The default
**                          is to use the trunk/branches/tags svn layout
**
** The --incremental option allows an existing repository to be extended
** with new content.
**
** Options:
**   --incremental  allow importing into an existing repository
**
** See also: export
*/
void import_cmd(void){
  char *zPassword;
  FILE *pIn;
  Stmt q;
  int forceFlag = find_option("force", "f", 0)!=0;
  int incrFlag = find_option("incremental", "i", 0)!=0;
  int flatFlag = find_option("flat", 0, 0)!=0;

  verify_all_options();
  if( g.argc!=4  && g.argc!=5 ){
    usage("FORMAT REPOSITORY-NAME");
  }
  if( g.argc==5 ){
    pIn = fossil_fopen(g.argv[4], "rb");
  }else{
    pIn = stdin;
    fossil_binary_mode(pIn);
  }
  if( !incrFlag ){
    if( forceFlag ) file_delete(g.argv[3]);
    db_create_repository(g.argv[3]);
  }
  db_open_repository(g.argv[3]);
  db_open_config(0);

  db_begin_transaction();
  if( !incrFlag ) db_initial_setup(0, 0, 0, 1);

  if( strncmp(g.argv[2], "git", 3)==0 ){
    /* The following temp-tables are used to hold information needed for
    ** the import.
    **
    ** The XMARK table provides a mapping from fast-import "marks" and symbols
    ** into artifact ids (UUIDs - the 40-byte hex SHA1 hash of artifacts).
    ** Given any valid fast-import symbol, the corresponding fossil rid and
    ** uuid can found by searching against the xmark.tname field.
    **
    ** The XBRANCH table maps commit marks and symbols into the branch those
    ** commits belong to.  If xbranch.tname is a fast-import symbol for a
    ** checkin then xbranch.brnm is the branch that checkin is part of.
    **
    ** The XTAG table records information about tags that need to be applied
    ** to various branches after the import finishes.  The xtag.tcontent field
    ** contains the text of an artifact that will add a tag to a check-in.
    ** The git-fast-export file format might specify the same tag multiple
    ** times but only the last tag should be used.  And we do not know which
    ** occurrence of the tag is the last until the import finishes.
    */
    db_multi_exec(
       "CREATE TEMP TABLE xmark(tname TEXT UNIQUE, trid INT, tuuid TEXT);"
       "CREATE TEMP TABLE xbranch(tname TEXT UNIQUE, brnm TEXT);"
       "CREATE TEMP TABLE xtag(tname TEXT UNIQUE, tcontent TEXT);"
    );

    git_fast_import(pIn);
    db_prepare(&q, "SELECT tcontent FROM xtag");
    while( db_step(&q)==SQLITE_ROW ){
      Blob record;
      db_ephemeral_blob(&q, 0, &record);
      fast_insert_content(&record, 0, 0);
      import_reset(0);
    }
    db_finalize(&q);
  }else
  if( strncmp(g.argv[2], "svn", 3)==0 ){
    db_multi_exec(
       "CREATE TEMP TABLE xrevisions("
       " trev INTEGER PRIMARY KEY, tuser TEXT, tmsg TEXT, ttime DATETIME,"
       " tparent INT, tbranch TEXT, tuuid TEXT"
       ");"
       "CREATE TEMP TABLE xfiles("
       " trev INT, tpath TEXT, trid TEXT, tperm TEXT,"
       " UNIQUE (trev, tpath) ON CONFLICT REPLACE"
       ");"
       "CREATE TEMP TABLE xtags("
       " trev INT, ttag TEXT"
       ");"
    );
    svn_dump_import(pIn, flatFlag);
  }

  db_end_transaction(0);
  db_begin_transaction();
  fossil_print("Rebuilding repository meta-data...\n");
  rebuild_db(0, 1, !incrFlag);
  verify_cancel();
  db_end_transaction(0);
  fossil_print("Vacuuming..."); fflush(stdout);
  db_multi_exec("VACUUM");
  fossil_print(" ok\n");
  if( !incrFlag ){
    fossil_print("project-id: %s\n", db_get("project-code", 0));
    fossil_print("server-id:  %s\n", db_get("server-code", 0));
    zPassword = db_text(0, "SELECT pw FROM user WHERE login=%Q", g.zLogin);
    fossil_print("admin-user: %s (password is \"%s\")\n", g.zLogin, zPassword);
  }
}
