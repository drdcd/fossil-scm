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
** This file implements the undo/redo functionality.
*/
#include "config.h"
#include "undo.h"



/*
** Undo the change to the file zPathname.  zPathname is the pathname
** of the file relative to the root of the repository.  If redoFlag is
** true the redo a change.  If there is nothing to undo (or redo) then
** this routine is a noop.
*/
static void undo_one(const char *zPathname, int redoFlag){
  Stmt q;
  char *zFullname;
  db_prepare(&q,
    "SELECT content, exists FROM undo WHERE pathname=%Q AND redoflag=%d",
     zPathname, redoFlag
  );
  if( db_step(&q)==SQLITE_ROW ){
    int old_exists;
    int new_exists;
    Blob current;
    Blob new;
    zFullname = mprintf("%s/%s", g.zLocalRoot, zPathname);
    new_exists = file_size(zFullname)>=0;
    if( new_exists ){
      blob_read_from_file(&current, zFullname);
    }else{
      blob_zero(&current);
    }
    blob_zero(&new);
    old_exists = db_column_int(&q, 1);
    if( old_exists ){
      db_ephemeral_blob(&q, 0, &new);
    }
    printf("%sdo changes to %s\n", redoFlag ? "Re" : "Un", zPathname);
    if( old_exists ){
      if( new_exists ){
        printf("%s %s\n", redoFlag ? "REDO" : "UNDO", zPathname);
      }else{
        printf("NEW %s\n", zPathname);
      }
      blob_write_to_file(&new, zFullname);
    }else{
      printf("DELETE %s\n", zPathname);
      unlink(zFullname);
    }
    blob_reset(&new);
    free(zFullname);
    db_finalize(&q);
    db_prepare(&q, 
       "UPDATE undo SET content=:c, existsflag=%d, redoflag=NOT redoflag"
       " WHERE pathname=%Q",
       new_exists, zPathname
    );
    if( new_exists ){
      db_bind_blob(&q, ":c", &current);
    }
    db_step(&q);
    blob_reset(&current);
  }
  db_finalize(&q);
}

/*
** Undo or redo all undoable or redoable changes.
*/
static void undo_all(int redoFlag){
  Stmt q;
  db_prepare(&q, "SELECT pathname FROM undo WHERE redoflag=%d"
                 " ORDER BY +pathname", redoFlag);
  while( db_step(&q) ){
    const char *zPathname = db_column_text(&q, 0);
    undo_one(zPathname, redoFlag);
  }
  db_finalize(&q);
  db_multi_exec(
    "CREATE TEMP TABLE undo_vfile_2 AS SELECT * FROM vfile;"
    "DELETE FROM vfile;"
    "INSERT INTO vfile SELECT * FROM undo_vfile;"
    "DELETE FROM undo_vfile;"
    "INSERT INTO undo_vfile SELECT * FROM undo_vfile_2;"
    "DROP TABLE undo_vfile_2;"
    "CREATE TEMP TABLE undo_vmerge_2 AS SELECT * FROM vmerge;"
    "DELETE FROM vmerge;"
    "INSERT INTO vmerge SELECT * FROM undo_vmerge;"
    "DELETE FROM undo_vmerge;"
    "INSERT INTO undo_vmerge SELECT * FROM undo_vmerge_2;"
    "DROP TABLE undo_vmerge_2;"
  );
}

/*
** Reset the the undo memory.
*/
void undo_reset(void){
  static const char zSql[] =
    @ DROP TABLE IF EXISTS undo;
    @ DROP TABLE IF EXISTS undo_vfile;
    @ DROP TABLE IF EXISTS undo_vmerge;
    ;
  db_multi_exec(zSql);
  db_lset_int("undo_available", 0);
}

/*
** Begin capturing a snapshot that can be undone.
*/
void undo_begin(void){
  static const char zSql[] = 
    @ CREATE TABLE undo(
    @   pathname TEXT UNIQUE,             -- Name of the file
    @   redoflag BOOLEAN,                 -- 0 for undoable.  1 for redoable
    @   existsflag BOOLEAN,               -- True if the file exists
    @   content BLOB                      -- Saved content
    @ );
    @ CREATE TABLE undo_vfile AS SELECT * FROM vfile;
    @ CREATE TABLE undo_vfile AS SELECT * FROM vmerge;
  ;
  undo_reset();
  db_multi_exec(zSql);
  db_lset_int("undo_available", 1);
}

/*
** Save the current content of the file zPathname so that it
** will be undoable.  The name is relative to the root of the
** tree.
*/
void undo_save(const char *zPathname){
  char *zFullname;
  Blob content;
  int existsFlag;
  Stmt q;

  zFullname = mprintf("%s/%s", g.zLocalRoot, zPathname);
  existsFlag = file_size(zFullname)>=0;
  db_prepare(&q,
    "REPLACE INTO undo(pathname,redoflag,existsflag,content)"
    " VALUES(%Q,0,%d,:c)",
    zPathname, existsFlag
  );
  if( existsFlag ){
    blob_read_from_file(&content, zFullname);
    db_bind_blob(&q, ":c", &content);
  }
  free(zFullname);
  db_step(&q);
  db_finalize(&q);
  blob_reset(&content);
}

/*
** COMMAND: undo
**
** Usage: %fossil undo ?FILENAME...?
**
** Undo the most recent update or merge operation.  If FILENAME is
** specified then restore the content of the named file(s) but otherwise
** leave the update or merge in effect.
**
** A single level of undo/redo is supported.  The undo/redo stack
** is cleared by the commit and checkout commands.
*/
void undo_cmd(void){
  int undo_available;
  db_must_be_within_tree();
  db_begin_transaction();
  undo_available = db_lget_int("undo_available", 0);
  if( g.argc==2 ){
    if( undo_available!=1 ){
      fossil_fatal("no update or merge operation is available to undo");
    }
    undo_all(0);
    db_lset_int("undo_available", 2);
  }else if( g.argc>=3 ){
    int i;
    if( undo_available==0 ){
      fossil_fatal("no update or merge operation is available to undo");
    }
    for(i=2; i<g.argc; i++){
      const char *zFile = g.argv[i];
      Blob path;
      file_tree_name(zFile, &path);
      undo_one(blob_str(&path), 0);
      blob_reset(&path);
    }
  }
  db_end_transaction(0);
}

/*
** COMMAND: redo
**
** Usage: %fossil redo ?FILENAME...?
**
** Redo the an update or merge operation that has been undone by the
** undo command.  If FILENAME is specified then restore the changes
** associated with the named file(s) but otherwise leave the update
** or merge undone.
**
** A single level of undo/redo is supported.  The undo/redo stack
** is cleared by the commit and checkout commands.
*/
void redo_cmd(void){
  int undo_available;
  db_must_be_within_tree();
  db_begin_transaction();
  undo_available = db_lget_int("undo_available", 0);
  if( g.argc==2 ){
    if( undo_available!=2 ){
      fossil_fatal("no undone update or merge operation is available to redo");
    }
    undo_all(1);
    db_lset_int("undo_available", 1);
  }else if( g.argc>=3 ){
    int i;
    if( undo_available==0 ){
      fossil_fatal("no update or merge operation is available to redo");
    }
    for(i=2; i<g.argc; i++){
      const char *zFile = g.argv[i];
      Blob path;
      file_tree_name(zFile, &path);
      undo_one(blob_str(&path), 0);
      blob_reset(&path);
    }
  }
  db_end_transaction(0);
}
