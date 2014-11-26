/*
** Copyright (c) 2014 D. Richard Hipp
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
** This file contains code used to implement the "publish" and
** "unpublished" commands.
*/
#include "config.h"
#include "publish.h"
#include <assert.h>

/*
** COMMAND: unpublished
**
** Usage: %fossil unpublished ?OPTIONS?
**
** Show a list of unpublished or "private" artifacts.  Unpublished artifacts
** will never push and hence will not be shared with collaborators.
**
** By default, this command only shows unpublished checkins.  To show
** all unpublished artifacts, use the --all command-line option.
**
** OPTIONS:
**     --all                   Show all artifacts, not just checkins
**     --brief                 Show just the SHA1 hashes, not details
*/
void unpublished_cmd(void){
  int bAll = find_option("all",0,0)!=0;
  int bBrief = find_option("brief",0,0)!=0;
  const char *zCols;
  int n = 0;
  Stmt q;

  db_find_and_open_repository(0,0);
  verify_all_options();
  if( bBrief ){
    zCols = "(SELECT uuid FROM blob WHERE rid=private.rid)";
  }else{
    zCols = "private.rid";
  }
  if( bAll ){
    db_prepare(&q, "SELECT %s FROM private", zCols/*safe-for-%s*/);
  }else{
    db_prepare(&q, "SELECT %s FROM private, event"
                   " WHERE private.rid=event.objid"
                   "   AND event.type='ci';", zCols/*safe-for-%s*/);
  }
  while( db_step(&q)==SQLITE_ROW ){
    if( bBrief ){
      fossil_print("%s\n", db_column_text(&q,0));
    }else{
      if( n++ > 0 ) fossil_print("%.78c\n",'-');
      whatis_rid(db_column_int(&q,0),0);
    }
  }
  db_finalize(&q);
}

/*
** COMMAND: publish
**
** Usage: %fossil publish ?--only? TAGS...
**
** Cause artifacts identified by TAGS... to be published (made non-private).
** This can be used (for example) to convert a private branch into a public
** branch, or to publish a bundle that was imported privately.
**
** If any of TAGS names a branch, then all checkins on that most recent
** instance of that branch are included, not just the most recent checkin.
**
** If any of TAGS name checkins then all files and tags associated with
** those checkins are also published automatically.  Except if the --only
** option is used, then only the specific artifacts identified by TAGS
** are published.
**
** If a TAG is already public, this command is a harmless no-op.
*/
void publish_cmd(void){
  int bOnly = find_option("only",0,0)!=0;
  int bTest = find_option("test",0,0)!=0;  /* Undocumented --test option */
  int bExclusive = find_option("exclusive",0,0)!=0;  /* undocumented */
  int i;

  db_find_and_open_repository(0,0);
  verify_all_options();
  if( g.argc<3 ) usage("?--only? TAGS...");
  db_begin_transaction();
  db_multi_exec("CREATE TEMP TABLE ok(rid INTEGER PRIMARY KEY);");
  for(i=2; i<g.argc; i++){
    int rid = name_to_rid(g.argv[i]);
    if( db_exists("SELECT 1 FROM tagxref"
                  " WHERE rid=%d AND tagid=%d"
                  "   AND tagtype>0 AND value=%Q",
                  rid,TAG_BRANCH,g.argv[i]) ){
      rid = start_of_branch(rid, 1);
      compute_descendants(rid, 1000000000);
    }else{
      db_multi_exec("INSERT OR IGNORE INTO ok VALUES(%d)", rid);
    }
  }
  if( !bOnly ){
    find_checkin_associates("ok", bExclusive);
  }
  if( bTest ){
    /* If the --test option is used, then do not actually publish any
    ** artifacts.  Instead, just list the artifact information on standard
    ** output.  The --test option is useful for verifying correct operation
    ** of the logic that figures out which artifacts to publish, such as
    ** the find_checkin_associates() routine */
    Stmt q;
    int i = 0;
    db_prepare(&q, "SELECT rid FROM ok");
    while( db_step(&q)==SQLITE_ROW ){
      int rid = db_column_int(&q,0);
      if( i++ > 0 ) fossil_print("%.78c\n", '-');
      whatis_rid(rid, 0);
    }
    db_finalize(&q);
  }else{
    /* Standard behavior is simply to remove the published documents from
    ** the PRIVATE table */
    db_multi_exec("DELETE FROM private WHERE rid IN ok");
  }
  db_end_transaction(0);
}
