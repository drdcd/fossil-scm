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
** This file contains a string constant that is the default ticket
** configuration.
*/
#include "config.h"
#include "tktconfig.h"

/*
** This is a sample "ticket configuration" file for fossil.
**
** There is considerable flexibility in how tickets are defined
** in fossil.  Each repository can define its own ticket setup
** differently.  Each repository has an instance of a file, like
** this one that defines how that repository deals with tickets.
**
** This file is in the form of a script in an exceedingly 
** minimalist scripting language called "subscript".  Here are
** the rules:
**
** SYNTAX
**
**     *  The script consists of a sequence of whitespace
**        separated tokens.  Whitespace is ignored, except
**        as its role as a token separator.
**
**     *  Lines that begin with '#' are considered to be whitespace.
**
**     *  Text within matching {...} is consider to be a single "string"
**        token, even if the text spans multiple lines and includes
**        embedded whitespace.  The outermost {...} are not part of
**        the text of the token.
**
**     *  A token that begins with "/" is a string token.
**
**     *  A token that looks like a number is a string token.
**
**     *  Tokens that do not fall under the previous three rules
**        are "verb" tokens.
**
**  PROCESSING:
**
**     *  When a script is processed, the engine reads tokens
**        one by one.
**
**     *  String tokens are pushed onto the stack.
**
**     *  Verb tokens which correspond to the names of variables
**        cause the corresponding variable to be pushed onto the
**        stack.
**
**     *  Verb tokens which correspond to procedures cause the
**        procedures to run.  The procedures might push or pull 
**        values from the stack.
**
** There are just a handful of verbs.  For the purposes of this
** configuration script, there is only a single verb: "set".  The
** "set" verb pops two arguments from the stack.  The topmost is
** the name of a variable.  The second element is the value.  The
** "set" verb sets the value of the named variable.
**
**      VALUE NAME set
**
** This configuration file just sets the values of various variables.
** Some of the variables have special meanings.  The content of some
** of the variables are additional subscript scripts.
*/

/* @-comment: # */
const char zDefaultTicketConfig[] = 
@ ##########################################################################
@ #
@ # Every ticket configuration file should have a title.  When you
@ # come up with a new ticket configuration, please change the title
@ # to something descriptive.
@ #
@ {Default Ticket Configuration} /ticket_config_title set
@ 
@ ############################################################################
@ # Every ticket configuration *must* define a set of columns for the
@ # "ticket" table of the database.  These column names will also be
@ # used as CGI parameter names, so to avoid problems it is best that
@ # the names be all lower-case alphabetic characters.  The names must
@ # be unique and must not begin with "tkt_".
@ #
@ {
@    CREATE TABLE ticket(
@      -- Do not change any column that begins with tkt_
@      tkt_id INTEGER PRIMARY KEY,
@      tkt_uuid TEXT,
@      tkt_mtime DATE,
@      -- Add as many field as required below this line
@      type TEXT,
@      status TEXT,
@      subsystem TEXT,
@      priority TEXT,
@      severity TEXT,
@      foundin TEXT,
@      contact TEXT,
@      title TEXT,
@      comment TEXT,
@      -- Do not alter this UNIQUE clause:
@      UNIQUE(tkt_uuid, tkt_mtime)
@    );
@    -- Add indices as desired
@ } /ticket_sql set
@ 
@ ############################################################################
@ # You can define additional variables here.  These variables will be
@ # accessible to the page templates when they run.
@ #
@ {
@    Code_Defect
@    Build_Problem
@    Documentation
@    Feature_Request
@    Incident
@ } /type_choices set
@ {Immediate High Medium Low Zero} /priority_choices set
@ {Critical Severe Important Minor Cosmetic} /severity_choices set
@ {
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
@ } /resolution_choices set
@ {
@   Open
@   Verified
@   In_Process
@   Deferred
@   Fixed
@   Tested
@   Closed
@ } /status_choices set
@ {} /subsystem_choices set
@ 
@ ##########################################################################
@ # The "tktnew_template" variable is set to text which is a template for
@ # the HTML of the "New Ticket" page.  Within this template, text contained
@ # within [...] is subscript.  That subscript runs when the page is
@ # rendered.
@ #
@ {
@   <!-- load database field names not found in CGI with an empty string -->
@   <!-- start a form -->
@   [{Open} /status set /submit submit_new_ticket]
@   <table cellpadding="5">
@   <tr>
@   <td colspan="2">
@   Enter a one-line summary of the problem:<br>
@   <input type="text" name="title" size="60" value="[{} /title get html]">
@   </td>
@   </tr>
@   
@   <tr>
@   <td align="right">Type:
@   [/type type_choices 1 combobox]
@   </td>
@   <td>What type of ticket is this?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">Version: 
@   <input type="text" name="foundin" size="20" value="[{} /foundin get html]">
@   </td>
@   <td>In what version or build number do you observer the problem?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">Severity:
@   [/severity severity_choices 1 combobox]
@   </td>
@   <td>How debilitating is the problem?  How badly does the problem
@   effect the operation of the product?</td>
@   </tr>
@   
@   <tr>
@   <td align="right">EMail:
@   [/severity severity_choices 1 combobox]
@   </td>
@   <td>Not publically visible. Used by developers to contact you with
@   questions.</td>
@   </tr>
@   
@   <tr>
@   <td colspan="2">
@   Enter a detailed description of the problem.
@   For code defects, be sure to provide details on exactly how
@   the problem can be reproduced.  Provide as much detail as
@   possible.
@   <br>
@   <textarea name="comment" cols="80"
@    rows="[{} /comment linecount 50 max 10 min]"
@    wrap="virtual" class="wikiedit">[{} /comment get html]</textarea><br>
@   <input type="submit" name="preview" value="Preview">
@   </tr>
@ 
@   [/preview exists enable_output]
@   <tr><td colspan="2">
@   Description Preview:<br><hr>
@   [{} /comment get wiki]
@   <hr>
@   </td></tr>
@   [1 enable_output]
@   
@   <tr>
@   <td align="right">
@   <input type="submit" name="submit" value="Submit">
@   </td>
@   <td>After filling in the information above, press this button to create
@   the new ticket</td>
@   </tr>
@   </table>
@   <!-- end of form -->
@ } /tktnew_template set
@ 
@ ##########################################################################
@ # The template for the "edit ticket" page
@ {
@   <!-- database field names not found as CGI parameters are loaded
@        from the database automatically -->
@   <!-- start a form -->
@   [{<hr><i>%USER% added on %DATE%:</i><br>}
@     /cmappnd /comment append_remark]
@   [/submit submit_ticket_change]
@   <table cellpadding="5">
@   <tr><td align="right">Title:</td><td>
@   <input type="text" name="title" value="[title html] size=60">
@   </td></tr>
@   <tr><td align="right">Status:</td><td>
@   [/status status_choices 1 combobox]
@   </td></tr>
@   <tr><td align="right">Type:</td><td>
@   [/type type_choices 1 combobox]
@   </td></tr>
@   <tr><td align="right">Severity:</td><td>
@   [/severity severity_choices 1 combobox]
@   </td></tr>
@   <tr><td align="right">Priority:</td><td>
@   [/priority priority_choices 1 combobox]
@   </td></tr>
@   <tr><td align="right">Resolution:</td><td>
@   [/resolution resolution_choices 1 combobox]
@   </td></tr>
@   <tr><td align="right">Subsystem:</td><td>
@   [/subsystem subsystem_choices 1 combobox]
@   </td></tr>
@   [/e hascap enable_output]
@     <tr><td align="right">Contact:</td><td>
@     <input type="text" name="contact" size="40" value="[contact html]">
@     </td></tr>
@   [1 enable_output]
@   <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@   <input type="text" name="foundin" size="50" value="[foundin html]">
@   </td></tr>
@   <tr><td colspan="2">
@   [0 /eall 0 get /eall set]
@   [/aonlybtn exists not /eall set]
@   [/eallbtn exists /eall set]
@   [/w hascap eall and /eall set]
@   [eall enable_output]
@     Description And Comments:<br>
@     <textarea name="comment" cols="80" rows="[comment linecount 15 max 10 min]"
@      wrap="virtual" class="wikiedit">[comment html]</textarea><br>
@     <input type="hidden" name="eall" value="1">
@     <input type="submit" name="aonlybtn" value="Append Remark">
@   [eall not enable_output]
@     Append Remark:<br>
@     <textarea name="cmappnd" cols="80" rows="15"
@      wrap="virtual" class="wikiedit">[cmappnd html]</textarea><br>
@     [ok_wrtkt enable_output]
@     <input type="submit" name="eallbtn" value="Edit All">
@   [1 enable_output]
@   </td></tr>
@   <tr><td align="right"></td><td>
@   <input type="submit" name="submit" value="Submit Changes">
@   </td></tr>
@   </table>
@   <!-- end-form inserted automatically -->
@ } /tktedit_template set
@ 
@ ##########################################################################
@ # The template for the "view ticket" page
@ {
@   <!-- load database fields automatically loaded into variables -->
@   <table cellpadding="5">
@   <tr><td align="right">Title:</td><td>
@   [title html]
@   </td></tr>
@   <tr><td align="right">Status:</td><td>
@   [status html]
@   </td></tr>
@   <tr><td align="right">Type:</td><td>
@   [type html]
@   </td></tr>
@   <tr><td align="right">Severity:</td><td>
@   [severity html]
@   </td></tr>
@   <tr><td align="right">Priority:</td><td>
@   [priority html]
@   </td></tr>
@   <tr><td align="right">Resolution:</td><td>
@   [priority html]
@   </td></tr>
@   <tr><td align="right">Subsystem:</td><td>
@   [subsystem html]
@   </td></tr>
@   [{e} hascap enable_output]
@     <tr><td align="right">Contact:</td><td>
@     [contact html]
@     </td></tr>
@   [1 enable_output]
@   <tr><td align="right">Version&nbsp;Found&nbsp;In:</td><td>
@   [foundin html]
@   </td></tr>
@   <tr><td colspan="2">
@   Description And Comments:<br>
@   [comment wiki]
@   </td></tr>
@   </table>
@ } /tktview_template set
;
