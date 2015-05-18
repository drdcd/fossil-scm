/*
** Copyright (c) 2013 Stephen Beal
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
** This file contains code to implement the /reports web page.
**
*/
#include "config.h"
#include <string.h>
#include <time.h>
#include "statrep.h"


/*
** Used by stats_report_xxxxx() to remember which type of events
** to show. Populated by stats_report_init_view() and holds the
** return value of that function.
*/
static int statsReportType = 0;

/*
** Set by stats_report_init_view() to one of the y=XXXX values
** accepted by /timeline?y=XXXX.
*/
static const char *statsReportTimelineYFlag = NULL;


/*
** Generate a submenu element with a single parameter change.
*/
static void statrep_submenu(
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
** Creates a TEMP VIEW named v_reports which is a wrapper around the
** EVENT table filtered on event.type. It looks for the request
** parameter 'type' (reminder: we "should" use 'y' for consistency
** with /timeline, but /reports uses 'y' for the year) and expects it
** to contain one of the conventional values from event.type or the
** value "all", which is treated as equivalent to "*".  By default (if
** no 'y' is specified), "*" is assumed (that is also the default for
** invalid/unknown filter values). That 'y' filter is the one used for
** the event list. Note that a filter of "*" or "all" is equivalent to
** querying against the full event table. The view, however, adds an
** abstraction level to simplify the implementation code for the
** various /reports pages.
**
** Returns one of: 'c', 'w', 'g', 't', 'e', representing the type of
** filter it applies, or '*' if no filter is applied (i.e. if "all" is
** used).
*/
static int stats_report_init_view(){
  const char *zType = PD("type","*");  /* analog to /timeline?y=... */
  const char *zRealType = NULL;        /* normalized form of zType */
  int rc = 0;                          /* result code */
  assert( !statsReportType && "Must not be called more than once." );
  switch( (zType && *zType) ? *zType : 0 ){
    case 'c':
    case 'C':
      zRealType = "ci";
      rc = *zRealType;
      break;
    case 'e':
    case 'E':
      zRealType = "e";
      rc = *zRealType;
      break;
    case 'g':
    case 'G':
      zRealType = "g";
      rc = *zRealType;
      break;
    case 't':
    case 'T':
      zRealType = "t";
      rc = *zRealType;
      break;
    case 'w':
    case 'W':
      zRealType = "w";
      rc = *zRealType;
      break;
    default:
      rc = '*';
      break;
  }
  assert(0 != rc);
  if(zRealType){
    statsReportTimelineYFlag = zRealType;
    db_multi_exec("CREATE TEMP VIEW v_reports AS "
                  "SELECT * FROM event WHERE type GLOB %Q",
                  zRealType);
  }else{
    statsReportTimelineYFlag = "a";
    db_multi_exec("CREATE TEMP VIEW v_reports AS "
                  "SELECT * FROM event");
  }
  return statsReportType = rc;
}

/*
** Returns a string suitable (for a given value of suitable) for
** use in a label with the header of the /reports pages, dependent
** on the 'type' flag. See stats_report_init_view().
** The returned bytes are static.
*/
static const char *stats_report_label_for_type(){
  assert( statsReportType && "Must call stats_report_init_view() first." );
  switch( statsReportType ){
    case 'c':
      return "check-ins";
    case 'e':
      return "technotes";
    case 'w':
      return "wiki changes";
    case 't':
      return "ticket changes";
    case 'g':
      return "tag changes";
    default:
      return "all types";
  }
}

/*
** A helper for the /reports family of pages which prints out a menu
** of links for the various type=XXX flags. zCurrentViewName must be
** the name/value of the 'view' parameter which is in effect at the
** time this is called. e.g. if called from the 'byuser' view then
** zCurrentViewName must be "byuser". Any URL parameters which need to
** be added to the generated URLs should be passed in zParam. The
** caller is expected to have already encoded any zParam in the %T or
** %t encoding.  */
static void stats_report_event_types_menu(const char *zCurrentViewName,
                                          const char *zParam){
  char *zTop;
  if(zParam && !*zParam){
    zParam = NULL;
  }
  zTop = mprintf("%s/reports?view=%s%s%s", g.zTop, zCurrentViewName,
                 zParam&&*zParam ? "&" : "", zParam);
  cgi_printf("<div>");
  cgi_printf("<span>Types:</span> ");
  if('*' == statsReportType){
    cgi_printf(" <strong>all</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s'>all</a>", zTop);
  }
  if('c' == statsReportType){
    cgi_printf(" <strong>check-ins</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=ci'>check-ins</a>", zTop);
  }
  if('e' == statsReportType){
    cgi_printf(" <strong>technotes</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=e'>technotes</a>", zTop);
  }
  if( 't' == statsReportType ){
    cgi_printf(" <strong>tickets</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=t'>tickets</a>", zTop);
  }
  if( 'g' == statsReportType ){
    cgi_printf(" <strong>tags</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=g'>tags</a>", zTop);
  }
  if( 'w' == statsReportType ){
    cgi_printf(" <strong>wiki</strong>", zTop);
  }else{
    cgi_printf(" <a href='%s&type=w'>wiki</a>", zTop);
  }
  fossil_free(zTop);
  cgi_printf("</div>");
}


/*
** Helper for stats_report_by_month_year(), which generates a list of
** week numbers. zTimeframe should be either a timeframe in the form YYYY
** or YYYY-MM.
*/
static void stats_report_output_week_links(const char *zTimeframe){
  Stmt stWeek = empty_Stmt;
  char yearPart[5] = {0,0,0,0,0};
  memcpy(yearPart, zTimeframe, 4);
  db_prepare(&stWeek,
             "SELECT DISTINCT strftime('%%W',mtime) AS wk, "
             "count(*) AS n, "
             "substr(date(mtime),1,%d) AS ym "
             "FROM v_reports "
             "WHERE ym=%Q AND mtime < current_timestamp "
             "GROUP BY wk ORDER BY wk",
             strlen(zTimeframe),
             zTimeframe);
  while( SQLITE_ROW == db_step(&stWeek) ){
    const char *zWeek = db_column_text(&stWeek,0);
    const int nCount = db_column_int(&stWeek,1);
    cgi_printf("<a href='%R/timeline?"
               "yw=%t-%t&n=%d&y=%s'>%s</a>",
               yearPart, zWeek,
               nCount, statsReportTimelineYFlag, zWeek);
  }
  db_finalize(&stWeek);
}

/*
** Implements the "byyear" and "bymonth" reports for /reports.
** If includeMonth is true then it generates the "bymonth" report,
** else the "byyear" report. If zUserName is not NULL then the report is
** restricted to events created by the named user account.
*/
static void stats_report_by_month_year(char includeMonth,
                                       char includeWeeks,
                                       const char *zUserName){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  const char *zTimeLabel = includeMonth ? "Year/Month" : "Year";
  char zPrevYear[5] = {0};           /* For keeping track of when
                                        we change years while looping */
  int nEventsPerYear = 0;            /* Total event count for the
                                        current year */
  char showYearTotal = 0;            /* Flag telling us when to show
                                        the per-year event totals */
  Blob header = empty_blob;          /* Page header text */
  int nMaxEvents  = 1;               /* for calculating length of graph
                                        bars. */
  int iterations = 0;                /* number of weeks/months we iterate
                                        over */
  Blob userFilter = empty_blob;      /* Optional user=johndoe query string */
  stats_report_init_view();
  if( zUserName ){
    blob_appendf(&userFilter, "user=%s", zUserName);
  }
  stats_report_event_types_menu( includeMonth ? "bymonth" : "byyear",
               blob_str(&userFilter) );
  blob_reset(&userFilter);
  db_prepare(&query,
             "SELECT substr(date(mtime),1,%d) AS timeframe,"
             "       count(*) AS eventCount"
             "  FROM v_reports"
             " WHERE ifnull(coalesce(euser,user,'')=%Q,1)"
             " GROUP BY timeframe"
             " ORDER BY timeframe DESC",
             includeMonth ? 7 : 4, zUserName);
  @ <h1>Timeline Events (%s(stats_report_label_for_type()))
  @ by year%s(includeMonth ? "/month" : "")
  if( zUserName ){
    @ for user %h(zUserName)
  }
  @ </h1>
  @ <table class='statistics-report-table-events' border='0' cellpadding='2'
  @  cellspacing='0' id='statsTable'>
  @ <thead>
  @ <th>%s(zTimeLabel)</th>
  @ <th>Events</th>
  @ <th width='90%%'><!-- relative commits graph --></th>
  @ </thead><tbody>
  /*
     Run the query twice. The first time we calculate the maximum
     number of events for a given row. Maybe someone with better SQL
     Fu can re-implement this with a single query.
  */
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
    ++iterations;
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const char *zTimeframe = db_column_text(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 1;
    showYearTotal = 0;
    if(!nSize) nSize = 1;
    if(includeMonth){
      /* For Month/year view, add a separator for each distinct year. */
      if(!*zPrevYear ||
         (0!=fossil_strncmp(zPrevYear,zTimeframe,4))){
        showYearTotal = *zPrevYear;
        if(showYearTotal){
          rowClass = ++nRowNumber % 2;
          @ <tr class='row%d(rowClass)'>
          @ <td></td>
          @ <td colspan='2'>Yearly total: %d(nEventsPerYear)</td>
          @</tr>
        }
        nEventsPerYear = 0;
        memcpy(zPrevYear,zTimeframe,4);
        rowClass = ++nRowNumber % 2;
        @ <tr class='row%d(rowClass)'>
        @ <th colspan='3' class='statistics-report-row-year'>%s(zPrevYear)</th>
        @ </tr>
     }
   }
   rowClass = ++nRowNumber % 2;
   nEventTotal += nCount;
   nEventsPerYear += nCount;
   @<tr class='row%d(rowClass)'>
   @ <td>
    if(includeMonth){
      cgi_printf("<a href='%R/timeline?"
                 "ym=%t&n=%d&y=%s",
                 zTimeframe, nCount,
                 statsReportTimelineYFlag );
      /* Reminder: n=nCount is not actually correct for bymonth unless
         that was the only user who caused events.
      */
      if( zUserName ){
        cgi_printf("&u=%t", zUserName);
      }
      cgi_printf("' target='_new'>%s</a>",zTimeframe);
    }else {
      cgi_printf("<a href='?view=byweek&y=%s&type=%c",
                 zTimeframe, (char)statsReportType);
      if( zUserName ){
        cgi_printf("&u=%t", zUserName);
      }
      cgi_printf("'>%s</a>", zTimeframe);
    }
    @ </td><td>%d(nCount)</td>
    @ <td>
    @ <div class='statistics-report-graph-line'
    @  style='width:%d(nSize)%%;'>&nbsp;</div>
    @ </td>
    @</tr>
    if(includeWeeks){
      /* This part works fine for months but it terribly slow (4.5s on my PC),
         so it's only shown for by-year for now. Suggestions/patches for
         a better/faster layout are welcomed. */
      @ <tr class='row%d(rowClass)'>
      @ <td colspan='2' class='statistics-report-week-number-label'>Week #:</td>
      @ <td class='statistics-report-week-of-year-list'>
      stats_report_output_week_links(zTimeframe);
      @ </td></tr>
    }

    /*
      Potential improvement: calculate the min/max event counts and
      use percent-based graph bars.
    */
  }
  db_finalize(&query);
  if(includeMonth && !showYearTotal && *zPrevYear){
    /* Add final year total separator. */
    rowClass = ++nRowNumber % 2;
    @ <tr class='row%d(rowClass)'>
    @ <td></td>
    @ <td colspan='2'>Yearly total: %d(nEventsPerYear)</td>
    @</tr>
  }
  @ </tbody></table>
  if(nEventTotal){
    const char *zAvgLabel = includeMonth ? "month" : "year";
    int nAvg = iterations ? (nEventTotal/iterations) : 0;
    @ <br><div>Total events: %d(nEventTotal)
    @ <br>Average per active %s(zAvgLabel): %d(nAvg)
    @ </div>
  }
  if( !includeMonth ){
    output_table_sorting_javascript("statsTable","tnx",-1);
  }
}

/*
** Implements the "byuser" view for /reports.
*/
static void stats_report_by_user(){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  stats_report_init_view();
  stats_report_event_types_menu("byuser", NULL);
  @ <h1>Timeline Events
  @ (%s(stats_report_label_for_type())) by User</h1>
  db_multi_exec(
    "CREATE TEMP TABLE piechart(amt,label);"
    "INSERT INTO piechart SELECT count(*), user FROM v_reports"
                         " GROUP BY user ORDER BY count(*) DESC;"
  );
  if( db_int(0, "SELECT count(*) FROM piechart")>=2 ){
    @ <center><svg width=700 height=400>
    piechart_render(700, 400, PIE_OTHER|PIE_PERCENT);
    @ </svg></centre><hr/>
  }
  @ <table class='statistics-report-table-events' border='0'
  @ cellpadding='2' cellspacing='0' id='statsTable'>
  @ <thead><tr>
  @ <th>User</th>
  @ <th>Events</th>
  @ <th width='90%%'><!-- relative commits graph --></th>
  @ </tr></thead><tbody>
  db_prepare(&query,
               "SELECT user, "
               "COUNT(*) AS eventCount "
               "FROM v_reports "
               "GROUP BY user ORDER BY eventCount DESC");
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const char *zUser = db_column_text(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 0;
    if(!nCount) continue /* arguable! Possible? */;
    else if(!nSize) nSize = 1;
    rowClass = ++nRowNumber % 2;
    nEventTotal += nCount;
    @<tr class='row%d(rowClass)'>
    @ <td>
    @ <a href="?view=bymonth&user=%h(zUser)&type=%c((char)statsReportType)">%h(zUser)</a>
    @ </td><td data-sortkey='%08x(-nCount)'>%d(nCount)</td>
    @ <td>
    @ <div class='statistics-report-graph-line'
    @  style='width:%d(nSize)%%;'>&nbsp;</div>
    @ </td>
    @</tr>
    /*
      Potential improvement: calculate the min/max event counts and
      use percent-based graph bars.
    */
  }
  @ </tbody></table>
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","tkx",2);
}

/*
** Implements the "byfile" view for /reports. If zUserName is not NULL then the
** report is restricted to events created by the named user account.
*/
static void stats_report_by_file(const char *zUserName){
  Stmt query;
  int mxEvent = 1;       /* max number of events across all rows */
  int nRowNumber = 0;

  db_multi_exec(
    "CREATE TEMP TABLE statrep(filename, cnt);"
    "INSERT INTO statrep(filename, cnt)"
    "  SELECT filename.name, count(distinct mlink.mid)"
    "    FROM filename, mlink, event"
    "   WHERE filename.fnid=mlink.fnid"
    "     AND mlink.mid=event.objid"
    "     AND ifnull(coalesce(euser,user,'')=%Q,1)"
    "   GROUP BY 1", zUserName
  );
  db_prepare(&query,
    "SELECT filename, cnt FROM statrep ORDER BY cnt DESC, filename /*sort*/"
  );
  mxEvent = db_int(1, "SELECT max(cnt) FROM statrep");
  @ <h1>Check-ins Per File
  if( zUserName ){
    @ for user %h(zUserName)
  }
  @ </h1>
  @ <table class='statistics-report-table-events' border='0'
  @ cellpadding='2' cellspacing='0' id='statsTable'>
  @ <thead><tr>
  @ <th>File</th>
  @ <th>Check-ins</th>
  @ <th width='90%%'><!-- relative commits graph --></th>
  @ </tr></thead><tbody>
  while( SQLITE_ROW == db_step(&query) ){
    const char *zFile = db_column_text(&query, 0);
    const int n = db_column_int(&query, 1);
    int sz;
    if( n<=0 ) continue;
    sz = (int)(100*n/mxEvent);
    if( sz==0 ) sz = 1;
    @<tr class='row%d(++nRowNumber%2)'>
    @ <td>%z(href("%R/finfo?name=%T",zFile))%h(zFile)</a></td>
    @ <td>%d(n)</td>
    @ <td>
    @ <div class='statistics-report-graph-line'
    @  style='width:%d(sz)%%;'>&nbsp;</div>
    @ </td>
    @</tr>
  }
  @ </tbody></table>
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","tNx",2);
}

/*
** Implements the "byweekday" view for /reports. If zUserName is not NULL then
** the report is restricted to events created by the named user account.
*/
static void stats_report_day_of_week(const char *zUserName){
  Stmt query = empty_Stmt;
  int nRowNumber = 0;                /* current TR number */
  int nEventTotal = 0;               /* Total event count */
  int rowClass = 0;                  /* counter for alternating
                                        row colors */
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  Blob userFilter = empty_blob;      /* Optional user=johndoe query string */
  static const char *const daysOfWeek[] = {
  "Monday", "Tuesday", "Wednesday", "Thursday",
  "Friday", "Saturday", "Sunday"
  };

  stats_report_init_view();
  if( zUserName ){
    blob_appendf(&userFilter, "user=%s", zUserName);
  }
  stats_report_event_types_menu("byweekday", blob_str(&userFilter));
  db_prepare(&query,
               "SELECT cast(mtime %% 7 AS INTEGER) dow,"
               "       COUNT(*) AS eventCount"
               "  FROM v_reports"
               " WHERE ifnull(coalesce(euser,user,'')=%Q,1)"
               " GROUP BY dow ORDER BY dow", zUserName);
  @ <h1>Timeline Events (%h(stats_report_label_for_type())) by Day of the Week
  if( zUserName ){
    @ for user %h(zUserName)
  }
  @ </h1>
  db_multi_exec(
    "CREATE TEMP TABLE piechart(amt,label);"
    "INSERT INTO piechart SELECT count(*), cast(mtime %% 7 AS INT) FROM v_reports"
                         " WHERE ifnull(coalesce(euser,user,'')=%Q,1)"
                         " GROUP BY 2 ORDER BY 2;"
    "UPDATE piechart SET label = CASE label WHEN 0 THEN 'Monday' WHEN 1 THEN 'Tuesday'"
    "  WHEN 2 THEN 'Wednesday' WHEN 3 THEN 'Thursday' WHEN 4 THEN 'Friday'"
    "  WHEN 5 THEN 'Saturday' ELSE 'Sunday' END;", zUserName
  );
  if( db_int(0, "SELECT count(*) FROM piechart")>=2 ){
    @ <center><svg width=700 height=400>
    piechart_render(700, 400, PIE_OTHER|PIE_PERCENT);
    @ </svg></centre><hr/>
  }
  @ <table class='statistics-report-table-events' border='0'
  @ cellpadding='2' cellspacing='0' id='statsTable'>
  @ <thead><tr>
  @ <th>DoW</th>
  @ <th>Day</th>
  @ <th>Events</th>
  @ <th width='90%%'><!-- relative commits graph --></th>
  @ </tr></thead><tbody>
  while( SQLITE_ROW == db_step(&query) ){
    const int nCount = db_column_int(&query, 1);
    if(nCount>nMaxEvents){
      nMaxEvents = nCount;
    }
  }
  db_reset(&query);
  while( SQLITE_ROW == db_step(&query) ){
    const int dayNum =db_column_int(&query, 0);
    const int nCount = db_column_int(&query, 1);
    int nSize = nCount
      ? (int)(100 * nCount / nMaxEvents)
      : 0;
    if(!nCount) continue /* arguable! Possible? */;
    else if(!nSize) nSize = 1;
    rowClass = ++nRowNumber % 2;
    nEventTotal += nCount;
    @<tr class='row%d(rowClass)'>
    @ <td>%d(dayNum)</td>
    @ <td>%s(daysOfWeek[dayNum])</td>
    @ <td>%d(nCount)</td>
    @ <td>
    @ <div class='statistics-report-graph-line'
    @  style='width:%d(nSize)%%;'>&nbsp;</div>
    @ </td>
    @</tr>
  }
  @ </tbody></table>
  db_finalize(&query);
  output_table_sorting_javascript("statsTable","ntnx",1);
}


/*
** Helper for stats_report_by_month_year(), which generates a list of
** week numbers. zTimeframe should be either a timeframe in the form YYYY
** or YYYY-MM. If zUserName is not NULL then the report is restricted to events
** created by the named user account.
*/
static void stats_report_year_weeks(const char *zUserName){
  const char *zYear = P("y");
  int nYear = zYear ? strlen(zYear) : 0;
  int i = 0;
  Stmt qYears = empty_Stmt;
  char *zDefaultYear = NULL;
  int nMaxEvents = 1;                /* max number of events for
                                        all rows. */
  int iterations = 0;                /* # of active time periods. */
  Blob urlParams = empty_blob;
  stats_report_init_view();
  if(4==nYear){
    blob_appendf(&urlParams, "y=%T", zYear);
  }
  if( zUserName ){
    blob_appendf(&urlParams, "%suser=%s", blob_size(&urlParams) ? "&" : "",
                 zUserName);
  }
  stats_report_event_types_menu("byweek", blob_str(&urlParams));
  blob_reset(&urlParams);
  db_prepare(&qYears,
             "SELECT DISTINCT substr(date(mtime),1,4) AS y"
             "  FROM v_reports"
             " WHERE ifnull(coalesce(euser,user,'')=%Q,1)"
             " GROUP BY y ORDER BY y", zUserName);
  cgi_printf("Select year: ");
  while( SQLITE_ROW == db_step(&qYears) ){
    const char *zT = db_column_text(&qYears, 0);
    if( i++ ){
      cgi_printf(" ");
    }
    cgi_printf("<a href='?view=byweek&y=%s&type=%c", zT,
               (char)statsReportType);
    if( zUserName ){
      cgi_printf("&user=%t",zUserName);
    }
    cgi_printf("'>%s</a>",zT);
  }
  db_finalize(&qYears);
  cgi_printf("<br/>");
  if(!zYear || !*zYear){
    zDefaultYear = db_text("????", "SELECT strftime('%%Y')");
    zYear = zDefaultYear;
    nYear = 4;
  }
  if(4 == nYear){
    Stmt stWeek = empty_Stmt;
    int rowCount = 0;
    int total = 0;
    db_prepare(&stWeek,
               "SELECT DISTINCT strftime('%%W',mtime) AS wk, "
               "       count(*) AS n "
               "  FROM v_reports "
               " WHERE %Q=substr(date(mtime),1,4) "
               "   AND mtime < current_timestamp "
               "   AND ifnull(coalesce(euser,user,'')=%Q,1)"
               " GROUP BY wk ORDER BY wk DESC", zYear, zUserName);
    @ <h1>Timeline events (%h(stats_report_label_for_type()))
    @ for the calendar weeks of %h(zYear)
    if( zUserName ){
      @  for user %h(zUserName)
    }
    @ </h1>
    cgi_printf("<table class='statistics-report-table-events' "
               "border='0' cellpadding='2' width='100%%' "
               "cellspacing='0' id='statsTable'>");
    cgi_printf("<thead><tr>"
               "<th>Week</th>"
               "<th>Events</th>"
               "<th width='90%%'><!-- relative commits graph --></th>"
               "</tr></thead>"
               "<tbody>");
    while( SQLITE_ROW == db_step(&stWeek) ){
      const int nCount = db_column_int(&stWeek, 1);
      if(nCount>nMaxEvents){
        nMaxEvents = nCount;
      }
      ++iterations;
    }
    db_reset(&stWeek);
    while( SQLITE_ROW == db_step(&stWeek) ){
      const char *zWeek = db_column_text(&stWeek,0);
      const int nCount = db_column_int(&stWeek,1);
      int nSize = nCount
        ? (int)(100 * nCount / nMaxEvents)
        : 0;
      if(!nSize) nSize = 1;
      total += nCount;
      cgi_printf("<tr class='row%d'>", ++rowCount % 2 );
      cgi_printf("<td><a href='%R/timeline?yw=%t-%s&n=%d&y=%s",
                 zYear, zWeek, nCount,
                 statsReportTimelineYFlag);
      if( zUserName ){
        cgi_printf("&u=%t",zUserName);
      }
      cgi_printf("'>%s</a></td>",zWeek);

      cgi_printf("<td>%d</td>",nCount);
      cgi_printf("<td>");
      if(nCount){
        cgi_printf("<div class='statistics-report-graph-line'"
                   "style='width:%d%%;'>&nbsp;</div>",
                   nSize);
      }
      cgi_printf("</td></tr>");
    }
    db_finalize(&stWeek);
    free(zDefaultYear);
    cgi_printf("</tbody></table>");
    if(total){
      int nAvg = iterations ? (total/iterations) : 0;
      cgi_printf("<br><div>Total events: %d<br>"
                 "Average per active week: %d</div>",
                 total, nAvg);
    }
    output_table_sorting_javascript("statsTable","tnx",-1);
  }
}

/*
** WEBPAGE: reports
**
** Shows activity reports for the repository.
**
** Query Parameters:
**
**   view=REPORT_NAME  Valid values: bymonth, byyear, byuser
**   user=NAME         Restricts statistics to the given user
**   type=TYPE         Restricts the report to a specific event type:
**                     ci (check-in), w (wiki), t (ticket), g (tag)
**                     Defaulting to all event types.
**
** The view-specific query parameters include:
**
** view=byweek:
**
**   y=YYYY            The year to report (default is the server's
**                     current year).
*/
void stats_report_page(){
  HQuery url;                        /* URL for various branch links */
  const char *zView = P("view");    /* Which view/report to show. */
  const char *zUserName = P("user");
  int haveU = !zUserName;

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  if(!zUserName){
    zUserName = P("u");
    haveU = !!zUserName;
  }
  if(zUserName && !*zUserName){
    zUserName = NULL;
  }
  url_initialize(&url, "reports");
  if(0!=fossil_strcmp(zView,"byuser")){
    style_submenu_entry(haveU ? "u" : "user", "User:", 12, 0);
    if(zUserName){
      url_add_parameter(&url, haveU ? "u" : "user", zUserName);
      statrep_submenu(&url, "(Remove User Flag)", "view", zView,
                      haveU ? "u" : "user");
    }
  }
  statrep_submenu(&url, "By Year", "view", "byyear", 0);
  statrep_submenu(&url, "By Month", "view", "bymonth", 0);
  statrep_submenu(&url, "By Week", "view", "byweek", 0);
  statrep_submenu(&url, "By Weekday", "view", "byweekday", 0);
  statrep_submenu(&url, "By User", "view", "byuser", "user");
  statrep_submenu(&url, "By File", "view", "byfile", "file");
  style_submenu_element("Stats", "Stats", "%R/stat");
  url_reset(&url);
  style_header("Activity Reports");
  if(0==fossil_strcmp(zView,"byyear")){
    stats_report_by_month_year(0, 0, zUserName);
  }else if(0==fossil_strcmp(zView,"bymonth")){
    stats_report_by_month_year(1, 0, zUserName);
  }else if(0==fossil_strcmp(zView,"byweek")){
    stats_report_year_weeks(zUserName);
  }else if(0==fossil_strcmp(zView,"byuser")){
    stats_report_by_user();
  }else if(0==fossil_strcmp(zView,"byweekday")){
    stats_report_day_of_week(zUserName);
  }else if(0==fossil_strcmp(zView,"byfile")){
    stats_report_by_file(zUserName);
  }else{
    @ <h1>Activity Reports:</h1>
    @ <ul>
    @ <li>%z(href("?view=byyear"))Events by year</a></li>
    @ <li>%z(href("?view=bymonth"))Events by month</a></li>
    @ <li>%z(href("?view=byweek"))Events by calendar week</a></li>
    @ <li>%z(href("?view=byweekday"))Events by day of the week</a></li>
    @ <li>%z(href("?view=byuser"))Events by user</a></li>
    @ <li>%z(href("?view=byfile"))Events by file</a></li>
    @ </ul>
  }

  style_footer();
}
