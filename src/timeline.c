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
** This file contains code to implement the timeline web page
**
*/
#include <string.h>
#include <time.h>
#include "config.h"
#include "timeline.h"

/*
** Shorten a UUID so that is the minimum length needed to contain
** at least one digit in the range 'a'..'f'.  The minimum length is 10.
*/
static void shorten_uuid(char *zDest, const char *zSrc){
  int i;
  for(i=0; i<10 && zSrc[i]<='9'; i++){}
  memcpy(zDest, zSrc, 10);
  if( i==10 && zSrc[i] ){
    do{
      zDest[i] = zSrc[i];
      i++;
    }while( zSrc[i-1]<='9' );
  }else{
    i = 10;
  }
  zDest[i] = 0;
}


/*
** Generate a hyperlink to a version.
*/
void hyperlink_to_uuid(const char *zUuid){
  char zShortUuid[UUID_SIZE+1];
  shorten_uuid(zShortUuid, zUuid);
  if( g.okHistory ){
    @ <a href="%s(g.zBaseURL)/info/%s(zShortUuid)">[%s(zShortUuid)]</a>
  }else{
    @ <b>[%s(zShortUuid)]</b>
  }
}

/*
** Generate a hyperlink that invokes javascript to highlight
** a version on mouseover.
*/
void hyperlink_to_uuid_with_mouseover(
  const char *zUuid,   /* The UUID to display */
  const char *zIn,     /* Javascript proc for mouseover */
  const char *zOut,    /* Javascript proc for mouseout */
  int id               /* Argument to javascript procs */
){
  char zShortUuid[UUID_SIZE+1];
  shorten_uuid(zShortUuid, zUuid);
  if( g.okHistory ){
    @ <a onmouseover='%s(zIn)("m%d(id)")' onmouseout='%s(zOut)("m%d(id)")'
    @    href="%s(g.zBaseURL)/vinfo/%s(zShortUuid)">[%s(zShortUuid)]</a>
  }else{
    @ <b onmouseover='%s(zIn)("m%d(id)")' onmouseout='%s(zOut)("m%d(id)")'>
    @ [%s(zShortUuid)]</b>
  }
}

/*
** Generate a hyperlink to a diff between two versions.
*/
void hyperlink_to_diff(const char *zV1, const char *zV2){
  if( g.okHistory ){
    if( zV2==0 ){
      @ <a href="%s(g.zBaseURL)/diff?v2=%s(zV1)">[diff]</a>
    }else{
      @ <a href="%s(g.zBaseURL)/diff?v1=%s(zV1)&v2=%s(zV2)">[diff]</a>
    }
  }
}

/*
** Generate a hyperlink to a date & time.
*/
void hyperlink_to_date(const char *zDate, const char *zSuffix){
  if( zSuffix==0 ) zSuffix = "";
  if( g.okHistory ){
    @ <a href="%s(g.zTop)/timeline?c=%T(zDate)">%s(zDate)</a>%s(zSuffix)
  }else{
    @ %s(zDate)%s(zSuffix)
  }
}

/*
** Generate a hyperlink to a user.  This will link to a timeline showing
** events by that user.  If the date+time is specified, then the timeline
** is centered on that date+time.
*/
void hyperlink_to_user(const char *zU, const char *zD, const char *zSuf){
  if( zSuf==0 ) zSuf = "";
  if( g.okHistory ){
    if( zD && zD[0] ){
      @ <a href="%s(g.zTop)/timeline?c=%T(zD)&u=%T(zU)">%h(zU)</a>%s(zSuf)
    }else{
      @ <a href="%s(g.zTop)/timeline?u=%T(zU)">%h(zU)</a>%s(zSuf)
    }
  }else{
    @ %s(zU)
  }
}

/*
** Count the number of primary non-branch children for the given check-in.
**
** A primary child is one where the parent is the primary parent, not
** a merge parent.
**
** A non-branch child is one which is on the same branch as the parent.
*/
int count_nonbranch_children(int pid){
  int nNonBranch;
  static const char zSql[] = 
    @ SELECT count(*) FROM plink
    @  WHERE pid=%d AND isprim
    @    AND coalesce((SELECT value FROM tagxref
    @                   WHERE tagid=%d AND rid=plink.pid), 'trunk')
    @       =coalesce((SELECT value FROM tagxref
    @                   WHERE tagid=%d AND rid=plink.cid), 'trunk')
  ;
  nNonBranch = db_int(0, zSql, pid, TAG_BRANCH, TAG_BRANCH);
  return nNonBranch;
}

/*
** Allowed flags for the tmFlags argument to www_print_timeline
*/
#if INTERFACE
#define TIMELINE_ARTID    0x0001  /* Show artifact IDs on non-check-in lines */
#define TIMELINE_LEAFONLY 0x0002  /* Show "Leaf", but not "Merge", "Fork" etc */
#define TIMELINE_BRIEF    0x0004  /* Combine adjacent elements of same object */
#endif

/*
** Output a timeline in the web format given a query.  The query
** should return these columns:
**
**    0.  rid
**    1.  UUID
**    2.  Date/Time
**    3.  Comment string
**    4.  User
**    5.  Number of non-merge children
**    6.  Number of parents
**    7.  True if is a leaf
**    8.  background color
**    9.  type ("ci", "w", "t")
**   10.  list of symbolic tags.
**   11.  tagid for ticket or wiki
**   12.  Short comment to user for repeated tickets and wiki
*/
void www_print_timeline(
  Stmt *pQuery,          /* Query to implement the timeline */
  int tmFlags,           /* Flags controlling display behavior */
  void (*xExtra)(int)    /* Routine to call on each line of display */
){
  int wikiFlags;
  int mxWikiLen;
  Blob comment;
  int prevTagid = 0;
  int suppressCnt = 0;
  char zPrevDate[20];

  zPrevDate[0] = 0;
  mxWikiLen = db_get_int("timeline-max-comment", 0);
  if( db_get_boolean("timeline-block-markup", 0) ){
    wikiFlags = WIKI_INLINE;
  }else{
    wikiFlags = WIKI_INLINE | WIKI_NOBLOCK;
  }

  db_multi_exec(
     "CREATE TEMP TABLE IF NOT EXISTS seen(rid INTEGER PRIMARY KEY);"
     "DELETE FROM seen;"
  );
  @ <table cellspacing=0 border=0 cellpadding=0>
  blob_zero(&comment);
  while( db_step(pQuery)==SQLITE_ROW ){
    int rid = db_column_int(pQuery, 0);
    const char *zUuid = db_column_text(pQuery, 1);
    int nPChild = db_column_int(pQuery, 5);
    int nParent = db_column_int(pQuery, 6);
    int isLeaf = db_column_int(pQuery, 7);
    const char *zBgClr = db_column_text(pQuery, 8);
    const char *zDate = db_column_text(pQuery, 2);
    const char *zType = db_column_text(pQuery, 9);
    const char *zUser = db_column_text(pQuery, 4);
    const char *zTagList = db_column_text(pQuery, 10);
    int tagid = db_column_int(pQuery, 11);
    int commentColumn = 3;    /* Column containing comment text */
    if( tagid ){
      if( tagid==prevTagid ){
        if( tmFlags & TIMELINE_BRIEF ){
          suppressCnt++;
          continue;
        }else{
          commentColumn = 12;
        }
      }
    }
    prevTagid = tagid;
    if( suppressCnt ){
      @ <tr><td><td><td>
      @ <small><i>... %d(suppressCnt) similar
      @ event%s(suppressCnt>1?"s":"") omitted.</i></small></tr>
      suppressCnt = 0;
    }
    if( strcmp(zType,"div")==0 ){
      @ <tr><td colspan=3><hr></td></tr>
      continue;
    }
    db_multi_exec("INSERT OR IGNORE INTO seen VALUES(%d)", rid);
    if( memcmp(zDate, zPrevDate, 10) ){
      sprintf(zPrevDate, "%.10s", zDate);
      @ <tr><td colspan=3>
      @   <div class="divider">%s(zPrevDate)</div>
      @ </td></tr>
    }
    @ <tr>
    @ <td valign="top">%s(&zDate[11])</td>
    @ <td width="20" align="center" valign="top">
    @ <font id="m%d(rid)" size="+1" color="white">*</font></td>
    if( zBgClr && zBgClr[0] ){
      @ <td valign="top" align="left" bgcolor="%h(zBgClr)">
    }else{
      @ <td valign="top" align="left">
    }
    if( zType[0]=='c' ){
      const char *azTag[5];
      int nTag = 0;
      hyperlink_to_uuid_with_mouseover(zUuid, "xin", "xout", rid);
      if( (tmFlags & TIMELINE_LEAFONLY)==0 ){
        if( nParent>1 ){
          azTag[nTag++] = "Merge";
        }
        if( nPChild>1 ){
          if( count_nonbranch_children(rid)>1 ){
            azTag[nTag++] = "Fork";
          }else{
            azTag[nTag++] = "Branch-Point";
          }
        }
      }
      if( isLeaf ){
        if( db_exists("SELECT 1 FROM tagxref"
                      " WHERE rid=%d AND tagid=%d AND tagtype>0",
                      rid, TAG_CLOSED) ){
          azTag[nTag++] = "Closed-Leaf";
        }else{
          azTag[nTag++] = "Leaf";
        }
      }
      if( nTag>0 ){
        int i;
        for(i=0; i<nTag; i++){
          @ <b>%s(azTag[i])%s(i==nTag-1?"":",")</b>
        }
      }
    }else if( (tmFlags & TIMELINE_ARTID)!=0 ){
      hyperlink_to_uuid(zUuid);
    }
    db_column_blob(pQuery, commentColumn, &comment);
    if( mxWikiLen>0 && blob_size(&comment)>mxWikiLen ){
      Blob truncated;
      blob_zero(&truncated);
      blob_append(&truncated, blob_buffer(&comment), mxWikiLen);
      blob_append(&truncated, "...", 3);
      wiki_convert(&truncated, 0, wikiFlags);
      blob_reset(&truncated);
    }else{
      wiki_convert(&comment, 0, wikiFlags);
    }
    blob_reset(&comment);
    if( zTagList && zTagList[0] ){
      @ (user: %h(zUser), tags: %h(zTagList))
    }else{
      @ (user: %h(zUser))
    }
    if( xExtra ){
      xExtra(rid);
    }
    @ </td></tr>
  }
  if( suppressCnt ){
    @ <tr><td><td><td>
    @ <small><i>... %d(suppressCnt) similar
    @ event%s(suppressCnt>1?"s":"") omitted.</i></small></tr>
    suppressCnt = 0;
  }
  @ </table>
}

/*
** Create a temporary table suitable for storing timeline data.
*/
static void timeline_temp_table(void){
  static const char zSql[] = 
    @ CREATE TEMP TABLE IF NOT EXISTS timeline(
    @   rid INTEGER PRIMARY KEY,
    @   uuid TEXT,
    @   timestamp TEXT,
    @   comment TEXT,
    @   user TEXT,
    @   nchild INTEGER,
    @   nparent INTEGER,
    @   isleaf BOOLEAN,
    @   bgcolor TEXT,
    @   etype TEXT,
    @   taglist TEXT,
    @   tagid INTEGER,
    @   short TEXT
    @ )
  ;
  db_multi_exec(zSql);
}

/*
** Return a pointer to a constant string that forms the basis
** for a timeline query for the WWW interface.
*/
const char *timeline_query_for_www(void){
  static char *zBase = 0;
  static const char zBaseSql[] =
    @ SELECT
    @   blob.rid,
    @   uuid,
    @   datetime(event.mtime,'localtime') AS timestamp,
    @   coalesce(ecomment, comment),
    @   coalesce(euser, user),
    @   (SELECT count(*) FROM plink WHERE pid=blob.rid AND isprim=1),
    @   (SELECT count(*) FROM plink WHERE cid=blob.rid),
    @   NOT EXISTS(SELECT 1 FROM plink
    @               WHERE pid=blob.rid
    @                AND coalesce((SELECT value FROM tagxref
    @                              WHERE tagid=%d AND rid=plink.pid), 'trunk')
    @                  = coalesce((SELECT value FROM tagxref
    @                              WHERE tagid=%d AND rid=plink.cid), 'trunk')),
    @   bgcolor,
    @   event.type,
    @   (SELECT group_concat(substr(tagname,5), ', ') FROM tag, tagxref
    @     WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid
    @       AND tagxref.rid=blob.rid AND tagxref.tagtype>0),
    @   tagid,
    @   brief
    @  FROM event JOIN blob 
    @ WHERE blob.rid=event.objid
  ;
  if( zBase==0 ){
    zBase = mprintf(zBaseSql, TAG_BRANCH, TAG_BRANCH);
  }
  return zBase;
}

/*
** Generate a submenu element with a single parameter change.
*/
static void timeline_submenu(
  HQuery *pUrl,            /* Base URL */
  const char *zMenuName,   /* Submenu name */
  const char *zParam,      /* Parameter value to add or change */
  const char *zValue,      /* Value of the new parameter */
  const char *zRemove      /* Parameter to omit */
){
  style_submenu_element(zMenuName, zMenuName, "%s",
                        url_render(pUrl, zParam, zValue, zRemove, 0));
}


/*
** zDate is a localtime date.  Insert records into the
** "timeline" table to cause <hr> to be inserted before and after
** entries of that date.
*/
static void timeline_add_dividers(const char *zDate){
  db_multi_exec(
    "INSERT INTO timeline(rid,timestamp,etype)"
    "VALUES(-1,datetime(%Q,'-1 second') || '.9','div')",
    zDate
  );
  db_multi_exec(
    "INSERT INTO timeline(rid,timestamp,etype)"
    "VALUES(-2,datetime(%Q) || '.1','div')",
     zDate
  );
}


/*
** WEBPAGE: timeline
**
** Query parameters:
**
**    a=TIMESTAMP    after this date
**    b=TIMESTAMP    before this date.
**    c=TIMESTAMP    "circa" this date.
**    n=COUNT        number of events in output
**    p=RID          artifact RID and up to COUNT parents and ancestors
**    d=RID          artifact RID and up to COUNT descendants
**    t=TAGID        show only check-ins with the given tagid
**    u=USER         only if belonging to this user
**    y=TYPE         'ci', 'w', 't'
**    s=TEXT         string search (comment and brief)
**
** p= and d= can appear individually or together.  If either p= or d=
** appear, then u=, y=, a=, and b= are ignored.
**
** If a= and b= appear, only a= is used.  If neither appear, the most
** recent events are choosen.
**
** If n= is missing, the default count is 20.
*/
void page_timeline(void){
  Stmt q;                            /* Query used to generate the timeline */
  Blob sql;                          /* text of SQL used to generate timeline */
  Blob desc;                         /* Description of the timeline */
  int nEntry = atoi(PD("n","20"));   /* Max number of entries on timeline */
  int p_rid = atoi(PD("p","0"));     /* artifact p and its parents */
  int d_rid = atoi(PD("d","0"));     /* artifact d and its descendants */
  const char *zUser = P("u");        /* All entries by this user if not NULL */
  const char *zType = PD("y","all"); /* Type of events.  All if NULL */
  const char *zAfter = P("a");       /* Events after this time */
  const char *zBefore = P("b");      /* Events before this time */
  const char *zCirca = P("c");       /* Events near this time */
  const char *zTagName = P("t");     /* Show events with this tag */
  const char *zSearch = P("s");      /* Search string */
  HQuery url;                        /* URL for various branch links */
  int tagid;                         /* Tag ID */
  int tmFlags;                       /* Timeline flags */

  /* To view the timeline, must have permission to read project data.
  */
  login_check_credentials();
  if( !g.okRead && !g.okRdTkt && !g.okRdWiki ){ login_needed(); return; }
  if( zTagName && g.okRead ){
    tagid = db_int(0, "SELECT tagid FROM tag WHERE tagname='sym-%q'", zTagName);
  }else{
    tagid = 0;
  }
  if( zType[0]=='a' ){
    tmFlags = TIMELINE_BRIEF;
  }else{
    tmFlags = 0;
  }

  style_header("Timeline");
  login_anonymous_available();
  timeline_temp_table();
  blob_zero(&sql);
  blob_zero(&desc);
  blob_append(&sql, "INSERT OR IGNORE INTO timeline ", -1);
  blob_append(&sql, timeline_query_for_www(), -1);
  if( (p_rid || d_rid) && g.okRead ){
    /* If p= or d= is present, ignore all other parameters other than n= */
    char *zUuid;
    int np, nd;

    if( p_rid && d_rid ){
      if( p_rid!=d_rid ) p_rid = d_rid;
      if( P("n")==0 ) nEntry = 10;
    }
    db_multi_exec(
       "CREATE TEMP TABLE IF NOT EXISTS ok(rid INTEGER PRIMARY KEY)"
    );
    zUuid = db_text("", "SELECT uuid FROM blob WHERE rid=%d",
                         p_rid ? p_rid : d_rid);
    blob_appendf(&sql, " AND event.objid IN ok");
    nd = 0;
    if( d_rid ){
      compute_descendants(d_rid, nEntry+1);
      nd = db_int(0, "SELECT count(*)-1 FROM ok");
      if( nd>=0 ){
        db_multi_exec("%s", blob_str(&sql));
        blob_appendf(&desc, "%d descendant%s", nd,(1==nd)?"":"s");
      }
      timeline_add_dividers(
        db_text("1","SELECT datetime(mtime,'localtime') FROM event"
                    " WHERE objid=%d", d_rid)
      );
      db_multi_exec("DELETE FROM ok");
    }
    if( p_rid ){
      compute_ancestors(p_rid, nEntry+1);
      np = db_int(0, "SELECT count(*)-1 FROM ok");
      if( np>0 ){
        if( nd>0 ) blob_appendf(&desc, " and ");
        blob_appendf(&desc, "%d ancestors", np);
        db_multi_exec("%s", blob_str(&sql));
      }
      if( d_rid==0 ){
        timeline_add_dividers(  
          db_text("1","SELECT datetime(mtime,'localtime') FROM event"
                      " WHERE objid=%d", p_rid)
        );
      }
    }
    if( g.okHistory ){
      blob_appendf(&desc, " of <a href='%s/info/%s'>[%.10s]</a>",
                   g.zBaseURL, zUuid, zUuid);
    }else{
      blob_appendf(&desc, " of check-in [%.10s]", zUuid);
    }
  }else{
    int n;
    const char *zEType = "event";
    char *zDate;
    char *zNEntry = mprintf("%d", nEntry);
    url_initialize(&url, "timeline");
    url_add_parameter(&url, "n", zNEntry);
    if( tagid>0 ){
      zType = "ci";
      url_add_parameter(&url, "t", zTagName);
      blob_appendf(&sql, " AND EXISTS (SELECT 1 FROM tagxref WHERE tagid=%d"
                                        " AND tagtype>0 AND rid=blob.rid)",
                   tagid);
    }
    if( (zType[0]=='w' && !g.okRdWiki)
     || (zType[0]=='t' && !g.okRdTkt)
     || (zType[0]=='c' && !g.okRead)
    ){
      zType = "all";
    }
    if( zType[0]=='a' ){
      if( !g.okRead || !g.okRdWiki || !g.okRdTkt ){
        char cSep = '(';
        blob_appendf(&sql, " AND event.type IN ");
        if( g.okRead ){
          blob_appendf(&sql, "%c'ci'", cSep);
          cSep = ',';
        }
        if( g.okRdWiki ){
          blob_appendf(&sql, "%c'w'", cSep);
          cSep = ',';
        }
        if( g.okRdTkt ){
          blob_appendf(&sql, "%c't'", cSep);
          cSep = ',';
        }
        blob_appendf(&sql, ")");
      }
    }else{ /* zType!="all" */
      blob_appendf(&sql, " AND event.type=%Q", zType);
      url_add_parameter(&url, "y", zType);
      if( zType[0]=='c' ){
        zEType = "checkin";
      }else if( zType[0]=='w' ){
        zEType = "wiki edit";
      }else if( zType[0]=='t' ){
        zEType = "ticket change";
      }
    }
    if( zUser ){
      blob_appendf(&sql, " AND event.user=%Q", zUser);
      url_add_parameter(&url, "u", zUser);
    }
    if ( zSearch ){
      blob_appendf(&sql,
        " AND (event.comment LIKE '%%%q%%' OR event.brief LIKE '%%%q%%')",
        zSearch, zSearch);
      url_add_parameter(&url, "s", zSearch);
    }
    if( zAfter ){
      while( isspace(zAfter[0]) ){ zAfter++; }
      if( zAfter[0] ){
        blob_appendf(&sql, 
           " AND event.mtime>=(SELECT julianday(%Q, 'utc'))"
           " ORDER BY event.mtime ASC", zAfter);
        url_add_parameter(&url, "a", zAfter);
        zBefore = 0;
      }else{
        zAfter = 0;
      }
    }else if( zBefore ){
      while( isspace(zBefore[0]) ){ zBefore++; }
      if( zBefore[0] ){
        blob_appendf(&sql, 
           " AND event.mtime<=(SELECT julianday(%Q, 'utc'))"
           " ORDER BY event.mtime DESC", zBefore);
        url_add_parameter(&url, "b", zBefore);
       }else{
        zBefore = 0;
      }
    }else if( zCirca ){
      while( isspace(zCirca[0]) ){ zCirca++; }
      if( zCirca[0] ){
        double rCirca = db_double(0.0, "SELECT julianday(%Q, 'utc')", zCirca);
        Blob sql2;
        blob_init(&sql2, blob_str(&sql), -1);
        blob_appendf(&sql2,
            " AND event.mtime<=%f ORDER BY event.mtime DESC LIMIT %d",
            rCirca, (nEntry+1)/2
        );
        db_multi_exec("%s", blob_str(&sql2));
        blob_reset(&sql2);
        blob_appendf(&sql,
            " AND event.mtime>=%f ORDER BY event.mtime ASC",
            rCirca
        );
        nEntry -= (nEntry+1)/2;
        timeline_add_dividers(zCirca);
        url_add_parameter(&url, "c", zCirca);
      }else{
        zCirca = 0;
      }
    }else{
      blob_appendf(&sql, " ORDER BY event.mtime DESC");
    }
    blob_appendf(&sql, " LIMIT %d", nEntry);
    db_multi_exec("%s", blob_str(&sql));

    n = db_int(0, "SELECT count(*) FROM timeline");
    if( n<nEntry && zAfter ){
      cgi_redirect(url_render(&url, "a", 0, "b", 0));
    }
    if( zAfter==0 && zBefore==0 && zCirca==0 ){
      blob_appendf(&desc, "%d most recent %ss", n, zEType);
    }else{
      blob_appendf(&desc, "%d %ss", n, zEType);
    }
    if( zUser ){
      blob_appendf(&desc, " by user %h", zUser);
    }
    if( tagid>0 ){
      blob_appendf(&desc, " tagged with \"%h\"", zTagName);
    }
    if( zAfter ){
      blob_appendf(&desc, " occurring on or after %h.<br>", zAfter);
    }else if( zBefore ){
      blob_appendf(&desc, " occurring on or before %h.<br>", zBefore);
    }else if( zCirca ){
      blob_appendf(&desc, " occurring around %h.<br>", zCirca);
    }
    if( g.okHistory ){
      if( zAfter || n==nEntry ){
        zDate = db_text(0, "SELECT min(timestamp) FROM timeline");
        timeline_submenu(&url, "Older", "b", zDate, "a");
        free(zDate);
      }
      if( zBefore || (zAfter && n==nEntry) ){
        zDate = db_text(0, "SELECT max(timestamp) FROM timeline");
        timeline_submenu(&url, "Newer", "a", zDate, "b");
        free(zDate);
      }else if( tagid==0 ){
        if( zType[0]!='a' ){
          timeline_submenu(&url, "All Types", "y", "all", 0);
        }
        if( zType[0]!='w' && g.okRdWiki ){
          timeline_submenu(&url, "Wiki Only", "y", "w", 0);
        }
        if( zType[0]!='c' && g.okRead ){
          timeline_submenu(&url, "Checkins Only", "y", "ci", 0);
        }
        if( zType[0]!='t' && g.okRdTkt ){
          timeline_submenu(&url, "Tickets Only", "y", "t", 0);
        }
      }
      if( nEntry>20 ){
        timeline_submenu(&url, "20 Events", "n", "20", 0);
      }
      if( nEntry<200 ){
        timeline_submenu(&url, "200 Events", "n", "200", 0);
      }
    }
  }
  blob_zero(&sql);
  db_prepare(&q, "SELECT * FROM timeline ORDER BY timestamp DESC");
  @ <h2>%b(&desc)</h2>
  blob_reset(&desc);
  www_print_timeline(&q, tmFlags, 0);
  db_finalize(&q);

  @ <script>
  @ var parentof = new Object();
  @ var childof = new Object();
  db_prepare(&q, "SELECT rid FROM timeline");
  while( db_step(&q)==SQLITE_ROW ){
    int rid = db_column_int(&q, 0);
    Stmt q2;
    const char *zSep;
    Blob *pOut = cgi_output_blob();

    db_prepare(&q2, "SELECT pid FROM plink WHERE cid=%d", rid);
    zSep = "";
    blob_appendf(pOut, "parentof[\"m%d\"] = [", rid);
    while( db_step(&q2)==SQLITE_ROW ){
      int pid = db_column_int(&q2, 0);
      blob_appendf(pOut, "%s\"m%d\"", zSep, pid);
      zSep = ",";
    }
    db_finalize(&q2);
    blob_appendf(pOut, "];\n");
    db_prepare(&q2, "SELECT cid FROM plink WHERE pid=%d", rid);
    zSep = "";
    blob_appendf(pOut, "childof[\"m%d\"] = [", rid);
    while( db_step(&q2)==SQLITE_ROW ){
      int pid = db_column_int(&q2, 0);
      blob_appendf(pOut, "%s\"m%d\"", zSep, pid);
      zSep = ",";
    }
    db_finalize(&q2);
    blob_appendf(pOut, "];\n");
  }
  db_finalize(&q);
  @ function setall(value){
  @   for(var x in parentof){
  @     setone(x,value);
  @   }
  @ }
  @ setall("#ffffff");
  @ function setone(id, clr){
  @   if( parentof[id]==null ) return 0;
  @   var w = document.getElementById(id);
  @   if( w.style.color==clr ){
  @     return 0
  @   }else{
  @     w.style.color = clr
  @     return 1
  @   }
  @ }
  @ function xin(id) {
  @   setall("#ffffff");
  @   setone(id,"#ff0000");
  @   set_children(id, "#b0b0b0");
  @   set_parents(id, "#b0b0b0");
  @   for(var x in parentof[id]){
  @     var pid = parentof[id][x]
  @     var w = document.getElementById(pid);
  @     if( w!=null ){
  @       w.style.color = "#000000";
  @     }
  @   }
  @   for(var x in childof[id]){
  @     var cid = childof[id][x]
  @     var w = document.getElementById(cid);
  @     if( w!=null ){
  @       w.style.color = "#000000";
  @     }
  @   }
  @ }
  @ function xout(id) {
  @   /* setall("#000000"); */
  @ }
  @ function set_parents(id, clr){
  @   var plist = parentof[id];
  @   if( plist==null ) return;
  @   for(var x in plist){
  @     var pid = plist[x];
  @     if( setone(pid,clr)==1 ){
  @       set_parents(pid,clr);
  @     }
  @   }
  @ }
  @ function set_children(id,clr){
  @   var clist = childof[id];
  @   if( clist==null ) return;
  @   for(var x in clist){
  @     var cid = clist[x];
  @     if( setone(cid,clr)==1 ){
  @       set_children(cid,clr);
  @     }
  @   }
  @ }
  @ </script>
  style_footer();
}

/*
** The input query q selects various records.  Print a human-readable
** summary of those records.
**
** Limit the number of entries printed to nLine.
** 
** The query should return these columns:
**
**    0.  rid
**    1.  uuid
**    2.  Date/Time
**    3.  Comment string and user
**    4.  Number of non-merge children
**    5.  Number of parents
*/
void print_timeline(Stmt *q, int mxLine){
  int nLine = 0;
  char zPrevDate[20];
  const char *zCurrentUuid=0;
  zPrevDate[0] = 0;

  if( g.localOpen ){
    int rid = db_lget_int("checkout", 0);
    zCurrentUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  }

  while( db_step(q)==SQLITE_ROW && nLine<=mxLine ){
    int rid = db_column_int(q, 0);
    const char *zId = db_column_text(q, 1);
    const char *zDate = db_column_text(q, 2);
    const char *zCom = db_column_text(q, 3);
    int nChild = db_column_int(q, 4);
    int nParent = db_column_int(q, 5);
    char *zFree = 0;
    int n = 0;
    char zPrefix[80];
    char zUuid[UUID_SIZE+1];
    
    sprintf(zUuid, "%.10s", zId);
    if( memcmp(zDate, zPrevDate, 10) ){
      printf("=== %.10s ===\n", zDate);
      memcpy(zPrevDate, zDate, 10);
      nLine++;
    }
    if( zCom==0 ) zCom = "";
    printf("%.8s ", &zDate[11]);
    zPrefix[0] = 0;
    if( nParent>1 ){
      sqlite3_snprintf(sizeof(zPrefix), zPrefix, "*MERGE* ");
      n = strlen(zPrefix);
    }
    if( nChild>1 ){
      const char *zBrType;
      if( count_nonbranch_children(rid)>1 ){
        zBrType = "*FORK* ";
      }else{
        zBrType = "*BRANCH* ";
      }
      sqlite3_snprintf(sizeof(zPrefix)-n, &zPrefix[n], zBrType);
      n = strlen(zPrefix);
    }
    if( zCurrentUuid && strcmp(zCurrentUuid,zId)==0 ){
      sqlite3_snprintf(sizeof(zPrefix)-n, &zPrefix[n], "*CURRENT* ");
      n += strlen(zPrefix);
    }
    zFree = sqlite3_mprintf("[%.10s] %s%s", zUuid, zPrefix, zCom);
    nLine += comment_print(zFree, 9, 79);
    sqlite3_free(zFree);
  }
}

/*
** Return a pointer to a static string that forms the basis for
** a timeline query for display on a TTY.
*/
const char *timeline_query_for_tty(void){
  static const char zBaseSql[] = 
    @ SELECT
    @   blob.rid,
    @   uuid,
    @   datetime(event.mtime,'localtime'),
    @   coalesce(ecomment,comment)
    @     || ' (user: ' || coalesce(euser,user,'?')
    @     || (SELECT case when length(x)>0 then ' tags: ' || x else '' end
    @           FROM (SELECT group_concat(substr(tagname,5), ', ') AS x
    @                   FROM tag, tagxref
    @                  WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid
    @                    AND tagxref.rid=blob.rid AND tagxref.tagtype>0))
    @     || ')',
    @   (SELECT count(*) FROM plink WHERE pid=blob.rid AND isprim),
    @   (SELECT count(*) FROM plink WHERE cid=blob.rid)
    @ FROM event, blob
    @ WHERE blob.rid=event.objid
  ;
  return zBaseSql;
}

/*
** Return true if the input string is a date in the ISO 8601 format:
** YYYY-MM-DD.
*/
static int isIsoDate(const char *z){
  return strlen(z)==10
      && z[4]=='-'
      && z[7]=='-'
      && isdigit(z[0])
      && isdigit(z[5]);
}

/*
** COMMAND: timeline
**
** Usage: %fossil timeline ?WHEN? ?BASELINE|DATETIME? ?-n N? ?-t TYPE?
**
** Print a summary of activity going backwards in date and time
** specified or from the current date and time if no arguments
** are given.  Show as many as N (default 20) check-ins.  The
** WHEN argument can be any unique abbreviation of one of these
** keywords:
**
**     before
**     after
**     descendants | children
**     ancestors | parents
**
** The BASELINE can be any unique prefix of 4 characters or more.
** The DATETIME should be in the ISO8601 format.  For
** examples: "2007-08-18 07:21:21".  You can also say "current"
** for the current version or "now" for the current time.
**
** The optional TYPE argument may any types supported by the /timeline
** page. For example:
**
**     w  = wiki commits only
**     ci = file commits only
**     t  = tickets only
*/
void timeline_cmd(void){
  Stmt q;
  int n, k;
  const char *zCount;
  const char *zType;
  char *zOrigin;
  char *zDate;
  Blob sql;
  int objid = 0;
  Blob uuid;
  int mode = 0 ;       /* 0:none  1: before  2:after  3:children  4:parents */
  db_find_and_open_repository(1);
  zCount = find_option("count","n",1);
  zType = find_option("type","t",1);
  if( zCount ){
    n = atoi(zCount);
  }else{
    n = 20;
  }
  if( g.argc>=4 ){
    k = strlen(g.argv[2]);
    if( strncmp(g.argv[2],"before",k)==0 ){
      mode = 1;
    }else if( strncmp(g.argv[2],"after",k)==0 && k>1 ){
      mode = 2;
    }else if( strncmp(g.argv[2],"descendants",k)==0 ){
      mode = 3;
    }else if( strncmp(g.argv[2],"children",k)==0 ){
      mode = 3;
    }else if( strncmp(g.argv[2],"ancestors",k)==0 && k>1 ){
      mode = 4;
    }else if( strncmp(g.argv[2],"parents",k)==0 ){
      mode = 4;
    }else if(!zType && !zCount){
      usage("?WHEN? ?BASELINE|DATETIME? ?-n|--count N? ?-t TYPE?");
    }
    if( '-' != *g.argv[3] ){
	zOrigin = g.argv[3];
    }else{
	zOrigin = "now";
    }
  }else if( g.argc==3 ){
    zOrigin = g.argv[2];
  }else{
    zOrigin = "now";
  }
  k = strlen(zOrigin);
  blob_zero(&uuid);
  blob_append(&uuid, zOrigin, -1);
  if( strcmp(zOrigin, "now")==0 ){
    if( mode==3 || mode==4 ){
      fossil_fatal("cannot compute descendants or ancestors of a date");
    }
    zDate = mprintf("(SELECT datetime('now'))");
  }else if( strncmp(zOrigin, "current", k)==0 ){
    if( !g.localOpen ){
      fossil_fatal("must be within a local checkout to use 'current'");
    }
    objid = db_lget_int("checkout",0);
    zDate = mprintf("(SELECT mtime FROM plink WHERE cid=%d)", objid);
  }else if( name_to_uuid(&uuid, 0)==0 ){
    objid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &uuid);
    zDate = mprintf("(SELECT mtime FROM plink WHERE cid=%d)", objid);
  }else{
    const char *zShift = "";
    if( mode==3 || mode==4 ){
      fossil_fatal("cannot compute descendants or ancestors of a date");
    }
    if( mode==0 ){
      if( isIsoDate(zOrigin) ) zShift = ",'+1 day'";
    }
    zDate = mprintf("(SELECT julianday(%Q%s, 'utc'))", zOrigin, zShift);
  }
  if( mode==0 ) mode = 1;
  blob_zero(&sql);
  blob_append(&sql, timeline_query_for_tty(), -1);
  blob_appendf(&sql, "  AND event.mtime %s %s",
     (mode==1 || mode==4) ? "<=" : ">=",
     zDate
  );

  if( mode==3 || mode==4 ){
    db_multi_exec("CREATE TEMP TABLE ok(rid INTEGER PRIMARY KEY)");
    if( mode==3 ){
      compute_descendants(objid, n);
    }else{
      compute_ancestors(objid, n);
    }
    blob_appendf(&sql, " AND blob.rid IN ok");
  }
  if( zType && (zType[0]!='a') ){
    blob_appendf(&sql, " AND event.type=%Q ", zType);
  }

  blob_appendf(&sql, " ORDER BY event.mtime DESC");
  db_prepare(&q, blob_str(&sql));
  blob_reset(&sql);
  print_timeline(&q, n);
  db_finalize(&q);
}

/*
** This is a version of the "localtime()" function from the standard
** C library.  It converts a unix timestamp (seconds since 1970) into
** a broken-out local time structure.
**
** This modified version of localtime() works like the library localtime()
** by default.  Except if the timeline-utc property is set, this routine
** uses gmttime() instead.  Thus by setting the timeline-utc property, we
** can get all localtimes to be displayed at UTC time.
*/
struct tm *fossil_localtime(const time_t *clock){
  if( g.fTimeFormat==0 ){
    if( db_get_int("timeline-utc", 1) ){
      g.fTimeFormat = 1;
    }else{
      g.fTimeFormat = 2;
    }
  }
  if( g.fTimeFormat==1 ){
    return gmtime(clock);
  }else{
    return localtime(clock);
  }
}
