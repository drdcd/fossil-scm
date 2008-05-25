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
** This file contains code to implement the ticket configuration
** setup screens.
*/
#include "config.h"
#include "tktsetup.h"
#include <assert.h>

/*
** Main sub-menu for configuring the ticketing system.
** WEBPAGE: tktsetup
*/
void tktsetup_page(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  style_header("Ticket Setup");
  @ <table border="0" cellspacing="20">
  setup_menu_entry("Table", "tktsetup_tab",
    "Specify the schema of the  \"ticket\" table in the database.");
  setup_menu_entry("Common", "tktsetup_com",
    "Common TH1 code run before all ticket processing.");
  setup_menu_entry("New Ticket Page", "tktsetup_newpage",
    "HTML with embedded TH1 code for the \"new ticket\" webpage.");
  setup_menu_entry("View Ticket Page", "tktsetup_viewpage",
    "HTML with embedded TH1 code for the \"view ticket\" webpage.");
  setup_menu_entry("Edit Ticket Page", "tktsetup_editpage",
    "HTML with embedded TH1 code for the \"edit ticket\" webpage.");
  setup_menu_entry("Report Format", "tktsetup_drep",
    "The default ticket report format.");
  @ </table>
  style_footer();
}

/* @-comment: ** */
static char zDefaultTab[] =
@ CREATE TABLE ticket(
@   -- Do not change any column that begins with tkt_
@   tkt_id INTEGER PRIMARY KEY,
@   tkt_uuid TEXT,
@   tkt_mtime DATE,
@   -- Add as many field as required below this line
@   type TEXT,
@   status TEXT,
@   subsystem TEXT,
@   priority TEXT,
@   severity TEXT,
@   foundin TEXT,
@   contact TEXT,
@   resolution TEXT,
@   title TEXT,
@   comment TEXT,
@   -- Do not alter this UNIQUE clause:
@   UNIQUE(tkt_uuid, tkt_mtime)
@ );
;

/*
** Common implementation for the ticket setup editor pages.
*/
static void tktsetup_generic(
  const char *zTitle,           /* Page title */
  const char *zDbField,         /* Configuration field being edited */
  char *zDfltValue,             /* Default text value */
  const char *zDesc,            /* Description of this field */
  int height                    /* Height of the edit box */
){
  const char *z;
  int isSubmit;
  
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }
  if( P("setup") ){
    cgi_redirect("tktsetup");
  }
  isSubmit = P("submit")!=0;
  db_begin_transaction();
  z = P("x");
  if( z==0 ){
    z = db_get(zDbField, zDfltValue);
  }
  style_header("Edit %s", zTitle);
  if( P("clear")!=0 ){
    db_unset(zDbField, 0);
    z = zDfltValue;
  }else if( isSubmit ){
    db_set(zDbField, z, 0);
  }
  @ <form action="%s(g.zBaseURL)/%s(g.zPath)" method="POST">
  @ %s(zDesc)
  @ <textarea name="tab" rows="%d(height)" cols="80">%h(z)</textarea>
  @ <br />
  @ <input type="submit" name="submit" value="Apply Changes">
  @ <input type="submit" name="clear" value="Revert To Default">
  @ <input type="submit" name="setup" value="Ticket Setup Menu">
  @ </form>
  @ <hr>
  @ <h2>Default %s(zTitle)</h2>
  @ <blockquote><pre>
  @ %h(zDfltValue)
  @ </pre></blockquote>
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: tktsetup_tab
*/
void tktsetup_tab_page(void){
  static const char zDesc[] =
  @ <p>Enter a valid CREATE TABLE statement for the "ticket" table.  The
  @ table must contain columns named "tkt_id", "tkt_uuid", and "tkt_mtime"
  @ with an unique index on "tkt_uuid" and "tkt_mtime".</p>
  ;
  tktsetup_generic(
    "Ticket Table Schema",
    "ticket-table",
    zDefaultTab,
    zDesc,
    20
  );
}

static char zDefaultCom[] =
@ set type_choices {
@    Code_Defect
@    Build_Problem
@    Documentation
@    Feature_Request
@    Incident
@ }
@ set priority_choices {
@   Immediate
@   High
@   Medium
@   Low
@   Zero
@ }
@ set severity_choices {
@   Critical
@   Severe
@   Important
@   Minor
@   Cosmetic
@ }
@ set resolution_choices {
@   Open
@   Fixed
@   Rejected
@   Unable_To_Reproduce
@   Works_As_Designed
@   External_Bug
@   Not_A_Bug
@   Duplicate
@   Overcome_By_Events
@   Drive_By_Patch
@ }
@ set status_choices {
@   Open
@   Verified
@   In_Process
@   Deferred
@   Fixed
@   Tested
@   Closed
@ }
@ set subsystem_choices {one two three}
;

/*
** WEBPAGE: tktsetup_com
*/
void tktsetup_com_page(void){
  static const char zDesc[] =
  @ <p>Enter TH1 script that initializes variables prior to generating
  @ any of the ticket view, edit, or creation pages.</p>
  ;
  tktsetup_generic(
    "Ticket Common Script",
    "ticket-common",
    zDefaultCom,
    zDesc,
    30
  );
}

static char zDefaultNew[] =
@ <th1>
@   if {[info exists submit]} {
@      set status Open
@      submit_ticket
@   }
@ </th1>
@ <table cellpadding="5">
@ <tr>
@ <td colspan="2">
@ Enter a one-line summary of the problem:<br>
@ <input type="text" name="title" size="60" value="$<title>">
@ </td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Type:
@ <th1>combobox type $type_choices 1</th1>
@ </td>
@ <td>What type of ticket is this?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Version: 
@ <input type="text" name="foundin" size="20" value="$<foundin>">
@ </td>
@ <td>In what version or build number do you observe the problem?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">Severity:
@ <th1>combobox severity $severity_choices 1</th1>
@ </td>
@ <td>How debilitating is the problem?  How badly does the problem
@ effect the operation of the product?</td>
@ </tr>
@ 
@ <tr>
@ <td align="right">EMail:
@ <input type="text" name="contact" value="$<contact>" size="30">
@ </td>
@ <td>Not publically visible. Used by developers to contact you with
@ questions.</td>
@ </tr>
@ 
@ <tr>
@ <td colspan="2">
@ Enter a detailed description of the problem.
@ For code defects, be sure to provide details on exactly how
@ the problem can be reproduced.  Provide as much detail as
@ possible.
@ <br>
@ <th1>set nline [linecount $comment 50 10]</th1>
@ <textarea name="comment" cols="80" rows="$nline"
@  wrap="virtual" class="wikiedit">$<comment></textarea><br>
@ <input type="submit" name="preview" value="Preview">
@ </tr>
@
@ <th1>enable_output [info exists preview]</th1>
@ <tr><td colspan="2">
@ Description Preview:<br><hr>
@ <th1>wiki $comment</th1>
@ <hr>
@ </td></tr>
@ <th1>enable_output 1</th1>
@ 
@ <tr>
@ <td align="right">
@ <input type="submit" name="submit" value="Submit">
@ </td>
@ <td>After filling in the information above, press this button to create
@ the new ticket</td>
@ </tr>
@ </table>
;

/*
** WEBPAGE: tktsetup_newpage
*/
void tktsetup_newpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "new ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For New Tickets",
    "ticket-newpage",
    zDefaultNew,
    zDesc,
    40
  );
}

static char zDefaultView[] =
@ <table cellpadding="5">
@ <tr><td align="right">Title:</td><td>
@ $<title>
@ </td></tr>
@ <tr><td align="right">Status:</td><td>
@ $<status>
@ </td></tr>
@ <tr><td align="right">Type:</td><td>
@ $<type>
@ </td></tr>
@ <tr><td align="right">Severity:</td><td>
@ $<severity>
@ </td></tr>
@ <tr><td align="right">Priority:</td><td>
@ $<priority>
@ </td></tr>
@ <tr><td align="right">Resolution:</td><td>
@ $<resolution>
@ </td></tr>
@ <tr><td align="right">Subsystem:</td><td>
@ $<subsystem>
@ </td></tr>
@ <th1>enable_output [hascap e]</th1>
@   <tr><td align="right">Contact:</td><td>
@   $<contact>
@   </td></tr>
@ <th1>enable_output 1</th1>
@ <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@ $<foundin>
@ </td></tr>
@ <tr><td colspan="2">
@ Description And Comments:<br>
@ <th1>wiki $comment</th1>
@ </td></tr>
@ </table>
;


/*
** WEBPAGE: tktsetup_viewpage
*/
void tktsetup_viewpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "view ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For Viewing Tickets",
    "ticket-viewpage",
    zDefaultView,
    zDesc,
    40
  );
}

static char zDefaultEdit[] =
@ <th1>
@   if {![info exists username]} {set username $login}
@   if {[info exists submit]} {
@     if {[info exists cmappnd]} {
@       if {[string length $cmappnd]>0} {
@         set ctxt "\n\n<hr><i>[htmlize $login]"
@         if {$username ne $login} {
@           set ctxt "$ctxt claiming to be [htmlize $username]"
@         }
@         set ctxt "$ctxt added on [date]:</i><br>\n$cmappnd"
@         append_field comment $ctxt
@       }
@     }
@     submit_ticket
@   }
@ </th1>
@ <table cellpadding="5">
@ <tr><td align="right">Title:</td><td>
@ <input type="text" name="title" value="$<title>" size="60">
@ </td></tr>
@ <tr><td align="right">Status:</td><td>
@ <th1>combobox status $status_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Type:</td><td>
@ <th1>combobox type $type_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Severity:</td><td>
@ <th1>combobox severity $severity_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Priority:</td><td>
@ <th1>combobox priority $priority_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Resolution:</td><td>
@ <th1>combobox resolution $resolution_choices 1</th1>
@ </td></tr>
@ <tr><td align="right">Subsystem:</td><td>
@ <th1>combobox subsystem $subsystem_choices 1</th1>
@ </td></tr>
@ <th1>enable_output [hascap e]</th1>
@   <tr><td align="right">Contact:</td><td>
@   <input type="text" name="contact" size="40" value="$<contact>">
@   </td></tr>
@ <th1>enable_output 1</th1>
@ <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@ <input type="text" name="foundin" size="50" value="$<foundin>">
@ </td></tr>
@ <tr><td colspan="2">
@ <th1>
@   if {![info exists eall]} {set eall 0}
@   if {[info exists aonlybtn]} {set eall 0}
@   if {[info exists eallbtn]} {set eall 1}
@   if {![hascap w]} {set eall 0}
@   if {![info exists cmappnd]} {set cmappnd {}}
@   set nline [linecount $comment 15 10]
@   enable_output $eall
@ </th1>
@   Description And Comments:<br>
@   <textarea name="comment" cols="80" rows="$nline"
@    wrap="virtual" class="wikiedit">$<comment></textarea><br>
@   <input type="hidden" name="eall" value="1">
@   <input type="submit" name="aonlybtn" value="Append Remark">
@ <th1>enable_output [expr {!$eall}]</th1>
@   Append Remark from 
@   <input type="text" name="username" value="$<username>" size="30">:<br>
@   <textarea name="cmappnd" cols="80" rows="15"
@    wrap="virtual" class="wikiedit">$<cmappnd></textarea><br>
@ <th1>enable_output [expr {[hascap w] && !$eall}]</th1>
@   <input type="submit" name="eallbtn" value="Edit All">
@ <th1>enable_output 1</th1>
@ </td></tr>
@ <tr><td align="right"></td><td>
@ <input type="submit" name="submit" value="Submit Changes">
@ </td></tr>
@ </table>
;

/*
** WEBPAGE: tktsetup_editpage
*/
void tktsetup_editpage_page(void){
  static const char zDesc[] =
  @ <p>Enter HTML with embedded TH1 script that will render the "edit ticket"
  @ page</p>
  ;
  tktsetup_generic(
    "HTML For Editing Tickets",
    "ticket-editpage",
    zDefaultEdit,
    zDesc,
    40
  );
}
