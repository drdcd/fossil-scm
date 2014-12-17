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
** This file contains code to implement the sitemap webpage.
*/
#include "config.h"
#include "sitemap.h"
#include <assert.h>

/*
** WEBPAGE:  sitemap
**
** Show an incomplete list of web pages offered by the Fossil web engine.
*/
void sitemap_page(void){
  login_check_credentials();
  style_header("Site Map");
  @ <p>
  @ The following links are just a few of the many web-pages available for
  @ this Fossil repository:
  @ </p>
  @
  @ <ul>
  @ <li>%z(href("%R/home"))Home Page</a>
  @ <li>%z(href("%R/tree"))File Browser</a>
  @ <ul>
  @   <li>%z(href("%R/tree?ci=trunk"))Tree-view, Trunk Checkin</a>
  @   <li>%z(href("%R/tree?type=flat"))Flat-view</a>
  @   <li>%z(href("%R/fileage?name=trunk"))File ages for Trunk</a>
  @ </ul>
  @ <li>%z(href("%R/timeline?n=200"))Project Timeline</a>
  @ <ul>
  @   <li>%z(href("%R/timeline?a=1970-01-01&y=ci&n=10"))First 10 checkins</a>
  @   <li>%z(href("%R/timeline?n=0&namechng"))All checkins with file name
  @        changes</a>
  @ </ul>
  @ <li>%z(href("%R/brlist"))Branches</a>
  @ <li>%z(href("%R/leaves"))Leaf Checkins</a>
  @ <li>%z(href("%R/taglist"))List of Tags</a>
  @ <li>%z(href("%R/wcontent"))List of Wiki Pages</a>
  @ <li>%z(href("%R/reportlist"))Ticket Reports</a>
  @ <li>%z(href("%R/reports"))Activity Reports</a>
  @ <li>%z(href("%R/login"))Login/Logout/Change Password</a>
  @ <li>%z(href("%R/attachlist"))List of Attachments</a>
  @ <li>Repository Status
  @   <ul>
  @   <li>%z(href("%R/stat"))Status Summary</a>
  @   <li>%z(href("%R/urllist"))List of URLs used to access this repository</a>
  @   <li>%z(href("%R/bloblist"))List of Artifacts</a>
  @   </ul>
  @ <li>On-line Documentation
  @   <ul>
  @   <li>%z(href("%R/help"))List of All Commands and Web Pages</a>
  @   <li>%z(href("%R/test-all-help"))All "help" text on a single page</a>
  @   <li>%z(href("%R/wiki_rules"))Wiki Formatting Rules</a>
  @   </ul>
  @ <li>Administration Pages
  @   <ul>
  @   <li>%z(href("%R/setup"))Configuration and Setup Menu</a>
  @   <li>%z(href("%R/modreq"))Pending Moderation Requests</a>
  @   </ul>
  @ <li>Test Pages
  @   <ul>
  @   <li>%z(href("%R/test_env"))CGI Environment Test</a>
  @   <li>%z(href("%R/test_timewarps"))List of "Timewarp" Checkins</a>
  @   <li>%z(href("%R/test-rename-list"))List of file renames</a>
  @   <li>%z(href("%R/hash-color-test"))Page to experiment with the automatic
  @       colors assigned to branch names</a>
  @   </ul>
  @ </ul>
  style_footer();
}