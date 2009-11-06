/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License version 2 as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code used to implement the "diff" command
*/
#include "config.h"
#include "diffcmd.h"
#include <assert.h>

/*
** Shell-escape the given string.  Append the result to a blob.
*/
static void shell_escape(Blob *pBlob, const char *zIn){
  int n = blob_size(pBlob);
  int k = strlen(zIn);
  int i, c;
  char *z;
  for(i=0; (c = zIn[i])!=0; i++){
    if( isspace(c) || c=='"' || (c=='\\' && zIn[i+1]!=0) ){
      blob_appendf(pBlob, "\"%s\"", zIn);
      z = blob_buffer(pBlob);
      for(i=n+1; i<=n+k; i++){
        if( z[i]=='"' ) z[i] = '_';
      }
      return;
    }
  }
  blob_append(pBlob, zIn, -1);
}

/*
** This function implements a cross-platform "system()" interface.
*/
int portable_system(char *zOrigCmd){
  int rc;
#ifdef __MINGW32__
  /* On windows, we have to put double-quotes around the entire command.
  ** Who knows why - this is just the way windows works.
  */
  char *zNewCmd = mprintf("\"%s\"", zOrigCmd);
  rc = system(zNewCmd);
  free(zNewCmd);
#else
  /* On unix, evaluate the command directly.
  */
  rc = system(zOrigCmd);
#endif 
  return rc; 
}

/*
** Show the difference between two files, one in memory and one on disk.
**
** The difference is the set of edits needed to transform pFile1 into
** zFile2.  The content of pFile1 is in memory.  zFile2 exists on disk.
**
** Use the internal diff logic if zDiffCmd is NULL.  Otherwise call the
** command zDiffCmd to do the diffing.
*/
static void diff_file(
  Blob *pFile1,             /* In memory content to compare from */
  const char *zFile2,       /* On disk content to compare to */
  const char *zName,        /* Display name of the file */
  const char *zDiffCmd      /* Command for comparison */
){
  if( zDiffCmd==0 ){
    Blob out;      /* Diff output text */
    Blob file2;    /* Content of zFile2 */

    /* Read content of zFile2 into memory */
    blob_zero(&file2);
    blob_read_from_file(&file2, zFile2);

    /* Compute and output the differences */
    blob_zero(&out);
    text_diff(pFile1, &file2, &out, 5);
    printf("--- %s\n+++ %s\n", zName, zName);
    printf("%s\n", blob_str(&out));

    /* Release memory resources */
    blob_reset(&file2);
    blob_reset(&out);
  }else{
    int cnt = 0;
    Blob nameFile1;    /* Name of temporary file to old pFile1 content */
    Blob cmd;          /* Text of command to run */

    /* Construct a temporary file to hold pFile1 based on the name of
    ** zFile2 */
    blob_zero(&nameFile1);
    do{
      blob_reset(&nameFile1);
      blob_appendf(&nameFile1, "%s~%d", zFile2, cnt++);
    }while( access(blob_str(&nameFile1),0)==0 );
    blob_write_to_file(pFile1, blob_str(&nameFile1));

    /* Construct the external diff command */
    blob_zero(&cmd);
    blob_appendf(&cmd, "%s ", zDiffCmd);
    shell_escape(&cmd, blob_str(&nameFile1));
    blob_append(&cmd, " ", 1);
    shell_escape(&cmd, zFile2);

    /* Run the external diff command */
    portable_system(blob_str(&cmd));

    /* Delete the temporary file and clean up memory used */
    unlink(blob_str(&nameFile1));
    blob_reset(&nameFile1);
    blob_reset(&cmd);
  }
}

/*
** Do a diff against a single file named in g.argv[2] from version zFrom
** against the same file on disk.
*/
static void diff_one_against_disk(const char *zFrom, const char *zDiffCmd){
  Blob fname;
  Blob content;
  file_tree_name(g.argv[2], &fname, 1);
  historical_version_of_file(zFrom, blob_str(&fname), &content);
  diff_file(&content, g.argv[2], g.argv[2], zDiffCmd);
  blob_reset(&content);
  blob_reset(&fname);
}

/*
** Run a diff between the version zFrom and files on disk.  zFrom might
** be NULL which means to simply show the difference between the edited
** files on disk and the check-out on which they are based.
*/
static void diff_all_against_disk(const char *zFrom, const char *zDiffCmd){
  int vid;
  Blob sql;
  Stmt q;

  vid = db_lget_int("checkout", 0);
  blob_zero(&sql);
  db_begin_transaction();
  if( zFrom ){
    int rid = name_to_rid(zFrom);
    if( !is_a_version(rid) ){
      fossil_fatal("no such check-in: %s", zFrom);
    }
    load_vfile_from_rid(rid);
    blob_appendf(&sql,
      "SELECT v2.pathname, v2.deleted, v2.chnged, v2.rid==0, v1.rid"
      "  FROM vfile v1, vfile v2 "
      " WHERE v1.pathname=v2.pathname AND v1.vid=%d AND v2.vid=%d"
      "   AND (v2.deleted OR v2.chnged OR v2.rid==0)"
      "UNION "
      "SELECT pathname, 1, 0, 0, 0"
      "  FROM vfile v1"
      " WHERE v1.vid=%d"
      "   AND NOT EXISTS(SELECT 1 FROM vfile v2"
                        " WHERE v2.vid=%d AND v2.pathname=v1.pathname)"
      "UNION "
      "SELECT pathname, 0, 0, 1, 0"
      "  FROM vfile v2"
      " WHERE v2.vid=%d"
      "   AND NOT EXISTS(SELECT 1 FROM vfile v1"
                        " WHERE v1.vid=%d AND v1.pathname=v2.pathname)"
      " ORDER BY 1",
      rid, vid, rid, vid, vid, rid
    );
  }else{
    blob_appendf(&sql,
      "SELECT pathname, deleted, chnged , rid==0, rid"
      "  FROM vfile"
      " WHERE vid=%d"
      "   AND (deleted OR chnged OR rid==0)"
      " ORDER BY pathname",
      vid
    );
  }
  db_prepare(&q, blob_str(&sql));
  while( db_step(&q)==SQLITE_ROW ){
    const char *zPathname = db_column_text(&q,0);
    int isDeleted = db_column_int(&q, 1);
    int isChnged = db_column_int(&q,2);
    int isNew = db_column_int(&q,3);
    char *zFullName = mprintf("%s%s", g.zLocalRoot, zPathname);
    if( isDeleted ){
      printf("DELETED  %s\n", zPathname);
    }else if( access(zFullName, 0) ){
      printf("MISSING  %s\n", zPathname);
    }else if( isNew ){
      printf("ADDED    %s\n", zPathname);
    }else if( isDeleted ){
      printf("DELETED  %s\n", zPathname);
    }else if( isChnged==3 ){
      printf("ADDED_BY_MERGE %s\n", zPathname);
    }else{
      int srcid = db_column_int(&q, 4);
      Blob content;
      content_get(srcid, &content);
      printf("Index: %s\n======================================="
             "============================\n",
             zPathname
      );
      diff_file(&content, zFullName, zPathname, zDiffCmd);
      blob_reset(&content);
    }
    free(zFullName);
  }
  db_finalize(&q);
  db_end_transaction(1);
}



/*
** COMMAND: diff
** COMMAND: gdiff
**
** Usage: %fossil diff|gdiff ?options? ?FILE?
**
** Show the difference between the current version of FILE (as it
** exists on disk) and that same file as it was checked out.  Or
** if the FILE argument is omitted, show the unsaved changed currently
** in the working check-out.
**
** If the "--from VERSION" or "-r VERSION" option is used it specifies
** the source check-in for the diff operation.  If not specified, the 
** source check-in is the base check-in for the current check-out.
**
** If the "--to VERSION" option appears, it specifies the check-in from
** which the second version of the file or files is taken.  If there is
** no "--to" option then the (possibly edited) files in the current check-out
** are used.
**
** The "-i" command-line option forces the use of the internal diff logic
** rather than any external diff program that might be configured using
** the "setting" command.  If no external diff program is configured, then
** the "-i" option is a no-op.  The "-i" option converts "gdiff" into "diff".
*/
void diff_cmd(void){
  int isGDiff;               /* True for gdiff.  False for normal diff */
  int isInternDiff;          /* True for internal diff */
  const char *zFrom;         /* Source version number */
  const char *zTo;           /* Target version number */
  const char *zDiffCmd = 0;  /* External diff command. NULL for internal diff */

  isGDiff = g.argv[1][0]=='g';
  isInternDiff = find_option("internal","i",0)!=0;
  zFrom = find_option("from", "r", 1);
  zTo = find_option("to", 0, 1);

  if( zTo==0 ){
    db_must_be_within_tree();
    if( !isInternDiff ){
      zDiffCmd = db_get(isGDiff ? "gdiff-command" : "diff-command", 0);
    }
    verify_all_options();
    if( g.argc==3 ){
      diff_one_against_disk(zFrom, zDiffCmd);
    }else{
      diff_all_against_disk(zFrom, zDiffCmd);
    }
  }else if( zFrom==0 ){
    fossil_fatal("must use --from if --to is present");
  }else{
    db_find_and_open_repository(1);
    if( !isInternDiff ){
      zDiffCmd = db_get(isGDiff ? "gdiff-command" : "diff-command", 0);
    }
    verify_all_options();
    fossil_fatal("--to not yet implemented");
#if 0
    if( g.argc==3 ){
      diff_one_two_versions(zFrom, zTo, zDiffCmd);
    }else{
      diff_all_two_versions(zFrom, zTo, zDiffCmd);
    }
#endif
  }
}
