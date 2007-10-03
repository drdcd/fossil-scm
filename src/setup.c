/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
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
** Implementation of the Setup page
*/
#include <assert.h>
#include "config.h"
#include "setup.h"


/*
** Output a single entry for a menu generated using an HTML table.
** If zLink is not NULL or an empty string, then it is the page that
** the menu entry will hyperlink to.  If zLink is NULL or "", then
** the menu entry has no hyperlink - it is disabled.
*/
void setup_menu_entry(
  const char *zTitle,
  const char *zLink,
  const char *zDesc
){
  @ <dt>
  if( zLink && zLink[0] ){
    @ <a href="%s(zLink)">%h(zTitle)</a>
  }else{
    @ %h(zTitle)
  }
  @ </dt>
  @ <dd>%h(zDesc)</dd>
}

/*
** WEBPAGE: /setup
*/
void setup_page(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  style_header("Setup");
  @ <dl id="setup">
  setup_menu_entry("Users", "setup_ulist",
    "Grant privileges to individual users.");
  setup_menu_entry("Access", "setup_access",
    "Control access settings.");
  setup_menu_entry("Configuration", "setup_config",
    "Configure the WWW components of the repository");
  setup_menu_entry("Tickets", "tktsetup",
    "Configure the trouble-ticketing system for this repository");
  @ </dl>

  style_footer();
}

/*
** WEBPAGE: setup_ulist
**
** Show a list of users.  Clicking on any user jumps to the edit
** screen for that user.
*/
void setup_ulist(void){
  Stmt s;
  
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
    return;
  }

  style_submenu_element("Add", "Add User", "setup_uedit");
  style_header("User List");
  @ <table border="0" cellpadding="0" cellspacing="25">
  @ <tr><td valign="top">
  @ <b>Users:</b>
  @ <table border="1" cellpadding="10"><tr><td>
  @ <table cellspacing=0 cellpadding=0 border=0>
  @ <tr>
  @   <th align="right">User&nbsp;ID</th><th width="15"></td>
  @   <th>Capabilities</th><th width="15"></td>
  @   <th>Contact&nbsp;Info</th>
  @ </tr>
  db_prepare(&s, "SELECT uid, login, cap, info FROM user ORDER BY login");
  while( db_step(&s)==SQLITE_ROW ){
    @ <tr>
    @ <td align="right">
    if( g.okAdmin ){
      @ <a href="setup_uedit?id=%d(db_column_int(&s,0))">
    }
    @ <nobr>%h(db_column_text(&s,1))</nobr>
    if( g.okAdmin ){
      @ </a>
    }
    @ </td><td></td>
    @ <td align="center">%s(db_column_text(&s,2))</td><td></td>
    @ <td align="left">%s(db_column_text(&s,3))</td>
    @ </tr>
  }
  @ </table></td></tr></table>
  @ <td valign="top">
  @ <b>Notes:</b>
  @ <ol>
  @ <li><p>The permission flags are as follows:</p>
  @ <ol type="a">
  @ <li value="1"><b>Admin</b>: Create and delete users</li>
  @ <li value="3"><b>Append-Tkt</b>: Append to tickets</li>
  @ <li value="4"><b>Delete</b>: Delete wiki and tickets</li>
  @ <li value="6"><b>New-Wiki</b>: Create new wiki pages</li>
  @ <li value="7"><b>Clone</b>: Clone the repository</li>
  @ <li value="8"><b>History</b>: View detail repository history</li>
  @ <li value="9"><b>Check-In</b>: Commit new versions in the repository</li>
  @ <li value="10"><b>Read-Wiki</b>: View wiki pages</li>
  @ <li value="11"><b>Write-Wiki</b>: Edit wiki pages</li>
  @ <li value="13"><b>Append-Wiki</b>: Append to wiki pages</li>
  @ <li value="14"><b>New-Tkt</b>: Create new tickets</li>
  @ <li value="15"><b>Check-Out</b>: Check out versions</li>
  @ <li value="16"><b>Password</b>: Change your own password</li>
  @ <li value="17"><b>Query</b>: Create new queries against tickets</li>
  @ <li value="18"><b>Read-Tkt</b>: View tickets</li>
  @ <li value="19"><b>Setup:</b> Setup and configure this website</li>
  @ <li value="23"><b>Write-Tkt</b>: Edit tickets</li>
  @ </ol>
  @ </p></li>
  @
  @ <li><p>
  @ Every user, logged in or not, has the privileges of <b>nobody</b>.
  @ Any human can login as <b>anonymous</b> since the password is
  @ clearly displayed on the login page for them to type.  The purpose
  @ of requiring anonymous to log in is to prevent access by spiders.
  @ </p></li>
  @
  @ </ol>
  @ </td></tr></table>
  style_footer();
}

/*
** WEBPAGE: /setup_uedit
*/
void user_edit(void){
  const char *zId, *zLogin, *zInfo, *zCap;
  char *oaa, *oas, *oar, *oaw, *oan, *oai, *oaj, *oao, *oap ;
  char *oak, *oad, *oaq, *oac, *oaf, *oam, *oah, *oag;
  int doWrite;
  int uid;
  int higherUser = 0;  /* True if user being edited is SETUP and the */
                       /* user doing the editing is ADMIN.  Disallow editing */

  /* Must have ADMIN privleges to access this page
  */
  login_check_credentials();
  if( !g.okAdmin ){ login_needed(); return; }

  /* Check to see if an ADMIN user is trying to edit a SETUP account.
  ** Don't allow that.
  */
  zId = PD("id", "0");
  uid = atoi(zId);
  if( zId && !g.okSetup && uid>0 ){
    char *zOldCaps;
    zOldCaps = db_text(0, "SELECT caps FROM user WHERE uid=%d",uid);
    higherUser = zOldCaps && strchr(zOldCaps,'s');
  }

  if( P("can") ){
    cgi_redirect("setup_ulist");
    return;
  }

  /* If we have all the necessary information, write the new or
  ** modified user record.  After writing the user record, redirect
  ** to the page that displays a list of users.
  */
  doWrite = cgi_all("login","info","pw") && !higherUser;
  if( doWrite ){
    const char *zPw;
    const char *zLogin;
    char zCap[30];
    int i = 0;
    int aa = P("aa")!=0;
    int ad = P("ad")!=0;
    int ai = P("ai")!=0;
    int aj = P("aj")!=0;
    int ak = P("ak")!=0;
    int an = P("an")!=0;
    int ao = P("ao")!=0;
    int ap = P("ap")!=0;
    int aq = P("aq")!=0;
    int ar = P("ar")!=0;
    int as = g.okSetup && P("as")!=0;
    int aw = P("aw")!=0;
    int ac = P("ac")!=0;
    int af = P("af")!=0;
    int am = P("am")!=0;
    int ah = P("ah")!=0;
    int ag = P("ag")!=0;
    if( aa ){ zCap[i++] = 'a'; }
    if( ac ){ zCap[i++] = 'c'; }
    if( ad ){ zCap[i++] = 'd'; }
    if( af ){ zCap[i++] = 'f'; }
    if( ah ){ zCap[i++] = 'h'; }
    if( ag ){ zCap[i++] = 'g'; }
    if( ai ){ zCap[i++] = 'i'; }
    if( aj ){ zCap[i++] = 'j'; }
    if( ak ){ zCap[i++] = 'k'; }
    if( am ){ zCap[i++] = 'm'; }
    if( an ){ zCap[i++] = 'n'; }
    if( ao ){ zCap[i++] = 'o'; }
    if( ap ){ zCap[i++] = 'p'; }
    if( aq ){ zCap[i++] = 'q'; }
    if( ar ){ zCap[i++] = 'r'; }
    if( as ){ zCap[i++] = 's'; }
    if( aw ){ zCap[i++] = 'w'; }

    zCap[i] = 0;
    zPw = P("pw");
    if( zPw==0 || zPw[0]==0 ){
      zPw = db_text(0, "SELECT pw FROM user WHERE uid=%d", uid);
    }
    zLogin = P("login");
    if( uid>0 && 
        db_exists("SELECT 1 FROM user WHERE login=%Q AND uid!=%d", zLogin, uid)
    ){
      style_header("User Creation Error");
      @ <font color="red">Login "%h(zLogin)" is already used by a different
      @ user.</font>
      @
      @ <p><a href="setup_uedit?id=%d(uid))>[Bummer]</a></p>
      style_footer();
      return;
    }
    db_multi_exec(
       "REPLACE INTO user(uid,login,info,pw,cap) "
       "VALUES(nullif(%d,0),%Q,%Q,%Q,'%s')",
      uid, P("login"), P("info"), zPw, zCap
    );
    cgi_redirect("setup_ulist");
    return;
  }

  /* Load the existing information about the user, if any
  */
  zLogin = "";
  zInfo = "";
  zCap = "";
  oaa = oac = oad = oaf = oag = oah = oai = oaj = oak = oam =
        oan = oao = oap = oaq = oar = oas = oaw = "";
  if( uid ){
    zLogin = db_text("", "SELECT login FROM user WHERE uid=%d", uid);
    zInfo = db_text("", "SELECT info FROM user WHERE uid=%d", uid);
    zCap = db_text("", "SELECT cap FROM user WHERE uid=%d", uid);
    if( strchr(zCap, 'a') ) oaa = " checked";
    if( strchr(zCap, 'c') ) oac = " checked";
    if( strchr(zCap, 'd') ) oad = " checked";
    if( strchr(zCap, 'f') ) oaf = " checked";
    if( strchr(zCap, 'g') ) oag = " checked";
    if( strchr(zCap, 'h') ) oah = " checked";
    if( strchr(zCap, 'i') ) oai = " checked";
    if( strchr(zCap, 'j') ) oaj = " checked";
    if( strchr(zCap, 'k') ) oak = " checked";
    if( strchr(zCap, 'm') ) oam = " checked";
    if( strchr(zCap, 'n') ) oan = " checked";
    if( strchr(zCap, 'o') ) oao = " checked";
    if( strchr(zCap, 'p') ) oap = " checked";
    if( strchr(zCap, 'q') ) oaq = " checked";
    if( strchr(zCap, 'r') ) oar = " checked";
    if( strchr(zCap, 's') ) oas = " checked";
    if( strchr(zCap, 'w') ) oaw = " checked";
  }

  /* Begin generating the page
  */
  style_submenu_element("Cancel", "Cancel", "setup_ulist");
  if( uid ){
    style_header(mprintf("Edit User %h", zLogin));
  }else{
    style_header("Add A New User");
  }
  @ <table align="left" hspace="20" vspace="10"><tr><td>
  @ <form action="%s(g.zPath)" method="POST">
  @ <table>
  @ <tr>
  @   <td align="right"><nobr>User ID:</nobr></td>
  if( uid ){
    @   <td>%d(uid) <input type="hidden" name="id" value="%d(uid)"></td>
  }else{
    @   <td>(new user)<input type="hidden" name="id" value=0></td>
  }
  @ </tr>
  @ <tr>
  @   <td align="right"><nobr>Login:</nobr></td>
  @   <td><input type="text" name="login" value="%h(zLogin)"></td>
  @ </tr>
  @ <tr>
  @   <td align="right"><nobr>Contact&nbsp;Info:</nobr></td>
  @   <td><input type="text" name="info" size=40 value="%h(zInfo)"></td>
  @ </tr>
  @ <tr>
  @   <td align="right" valign="top">Capabilities:</td>
  @   <td>
  if( g.okSetup ){
    @     <input type="checkbox" name="as"%s(oas)>Setup</input><br>
  }
  @     <input type="checkbox" name="aa"%s(oaa)>Admin</input><br>
  @     <input type="checkbox" name="ad"%s(oad)>Delete</input><br>
  @     <input type="checkbox" name="ap"%s(oap)>Password</input><br>
  @     <input type="checkbox" name="aq"%s(oaq)>Query</input><br>
  @     <input type="checkbox" name="ai"%s(oai)>Check-In</input><br>
  @     <input type="checkbox" name="ao"%s(oao)>Check-Out</input><br>
  @     <input type="checkbox" name="ah"%s(oah)>History</input><br>
  @     <input type="checkbox" name="ag"%s(oag)>Clone</input><br>
  @     <input type="checkbox" name="aj"%s(oaj)>Read Wiki</input><br>
  @     <input type="checkbox" name="af"%s(oaf)>New Wiki</input><br>
  @     <input type="checkbox" name="am"%s(oam)>Append Wiki</input><br>
  @     <input type="checkbox" name="ak"%s(oak)>Write Wiki</input><br>
  @     <input type="checkbox" name="ar"%s(oar)>Read Tkt</input><br>
  @     <input type="checkbox" name="an"%s(oan)>New Tkt</input><br>
  @     <input type="checkbox" name="ac"%s(oac)>Append Tkt</input><br>
  @     <input type="checkbox" name="aw"%s(oaw)>Write Tkt</input>
  @   </td>
  @ </tr>
  @ <tr>
  @   <td align="right">Password:</td>
  @   <td><input type="password" name="pw" value=""></td>
  @ </tr>
  if( !higherUser ){
    @ <tr>
    @   <td>&nbsp</td>
    @   <td><input type="submit" name="submit" value="Apply Changes">
    @ </tr>
  }
  @ </table></td></tr></table>
  @ <p><b>Notes:</b></p>
  @ <ol>
  if( higherUser ){
    @ <li><p>
    @ User %h(zId) has Setup privileges and you only have Admin privileges
    @ so you are not permitted to make changes to %h(zId).
    @ </p></li>
    @
  }
  @
  @ <li><p>
  @ The <b>Delete</b> privilege give the user the ability to erase
  @ wiki, tickets, and atttachments that have been added by anonymous
  @ users.  This capability is intended for deletion of spam.
  @ </p></li>
  @
  @ <li><p>
  @ The <b>Query</b> privilege allows the user to create or edit
  @ report formats by specifying appropriate SQL.  Users can run 
  @ existing reports without the Query privilege.
  @ </p></li>
  @
  @ <li><p>
  @ An <b>Admin</b> user can add other users, create new ticket report
  @ formats, and change system defaults.  But only the <b>Setup</b> user
  @ is able to change the repository to
  @ which this program is linked.
  @ </p></li>
  @
  @ <li><p>
  @ The <b>History</b> privilege allows a user to see a timeline
  @ with hyperlinks to version information, to download ZIP archives
  @ of individual versions.
  @ </p></li>
  @
  @ <li><p>
  @ No login is required for user "<b>nobody</b>".  The capabilities
  @ of this user are available to anyone without supplying a username or
  @ password.  To disable nobody access, make sure there is no user
  @ with an ID of <b>nobody</b> or that the nobody user has no
  @ capabilities enabled.  The password for nobody is ignore.  To
  @ avoid problems with spiders overloading the server, it is suggested
  @ that the 'h' (History) capability be turned off for user nobody.
  @ </p></li>
  @
  @ <li><p>
  @ Login is required for user "<b>anonymous</b>" but the password
  @ is displayed on the login screen beside the password entry box
  @ so anybody who can read should be able to login as anonymous.
  @ On the other hand, spiders and web-crawlers will typically not
  @ be able to login.  Set the capabilities of the anonymous user
  @ to things that you want any human to be able to do, but no any
  @ spider.
  @ </p></li>
  @ </form>
  style_footer();
}


/*
** Generate a checkbox for an attribute.
*/
static void onoff_attribute(
  const char *zLabel,   /* The text label on the checkbox */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQParm,   /* The query parameter */
  int dfltVal           /* Default value if VAR table entry does not exist */
){
  const char *zVal = db_get(zVar, 0);
  const char *zQ = P(zQParm);
  int iVal;
  if( zVal ){
    iVal = atoi(zVal);
  }else{
    iVal = dfltVal;
  }
  if( zQ==0 && P("submit") ){
    zQ = "off";
  }
  if( zQ ){
    int iQ = strcmp(zQ,"on")==0 || atoi(zQ);
    if( iQ!=iVal ){
      db_set(zVar, iQ ? "1" : "0", 0);
      iVal = iQ;
    }
  }
  if( iVal ){
    @ <input type="checkbox" name="%s(zQParm)" checked>%s(zLabel)</input>
  }else{
    @ <input type="checkbox" name="%s(zQParm)">%s(zLabel)</input>
  }
}

/*
** Generate an entry box for an attribute.
*/
static void entry_attribute(
  const char *zLabel,   /* The text label on the entry box */
  int width,            /* Width of the entry box */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQParm,   /* The query parameter */
  const char *zDflt     /* Default value if VAR table entry does not exist */
){
  const char *zVal = db_get(zVar, zDflt);
  const char *zQ = P(zQParm);
  if( zQ && strcmp(zQ,zVal)!=0 ){
    db_set(zVar, zQ, 0);
    zVal = zQ;
  }
  @ <input type="text" name="%s(zQParm)" value="%h(zVal)" size="%d(width)">
  @ %s(zLabel)
}

/*
** Generate a text box for an attribute.
*/
static void textarea_attribute(
  const char *zLabel,   /* The text label on the textarea */
  int rows,             /* Rows in the textarea */
  int cols,             /* Columns in the textarea */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQParm,   /* The query parameter */
  const char *zDflt     /* Default value if VAR table entry does not exist */
){
  const char *zVal = db_get(zVar, zDflt);
  const char *zQ = P(zQParm);
  if( zQ && strcmp(zQ,zVal)!=0 ){
    db_set(zVar, zQ, 0);
    zVal = zQ;
  }
  @ <textarea name="%s(zQParm)" rows="%d(rows)" cols="%d(cols)">%h(zVal)</textarea>
  @ %s(zLabel)
}


/*
** WEBPAGE: setup_access
*/
void setup_access(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  style_header("Access Control Settings");
  db_begin_transaction();
  @ <form action="%s(g.zBaseURL)/setup_access" method="POST">

  @ <hr>
  onoff_attribute("Require password for local access",
     "localauth", "localauth", 1);
  @ <p>When enabled, the password sign-in is required for
  @ web access coming from 127.0.0.1.  When disabled, web access
  @ from 127.0.0.1 is allows without any login - the user id is selected
  @ from the ~/.fossil database. Password login is always required
  @ for incoming web connections on internet addresses other than
  @ 127.0.0.1.</p></li>

  @ <hr>
  entry_attribute("Login expiration time", 6, "cookie-expire", "cex", "8766");
  @ <p>The number of hours for which a login is valid.  This must be a
  @ positive number.  The default is 8760 hours which is approximately equal
  @ to a year.</p>
   
  @ <hr>
  onoff_attribute("Allow anonymous signup", "anon-signup", "asu", 0);
  @ <p>Allow users to create their own accounts</p>
   
  @ <hr>
  @ <p><input type="submit"  name="submit" value="Apply Changes"></p>
  @ </form>
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_config
*/
void setup_config(void){
  login_check_credentials();
  if( !g.okSetup ){
    login_needed();
  }

  style_header("WWW Configuration");
  db_begin_transaction();
  @ <form action="%s(g.zBaseURL)/setup_config" method="POST">

  @ <hr>
  entry_attribute("Home page", 60, "homepage", "hp", "");
  @ <p>The name of a wiki file that is the homepage for the website.
  @ The home page is the page that is displayed by the "Home" link
  @ at the top of this screen.  Omit the path and the ".wiki"
  @ suffix.  </p>

  entry_attribute("Ticket subdirectory", 60, "ticket-subdir", "tsd", "");
  @ <p>A subdirectory in the file hierarchy that contains all trouble
  @ tickets.  Leave this blank to disable ticketing.  Tickets text
  @ files within this subdirectory containing a particular format
  @ (documented separately) and with the ".tkt" suffix.</p>

  entry_attribute("Wiki subdirectory", 60, "wiki-subdir", "wsd", "");
  @ <p>A subdirectory in the file hierarchy that contains wiki pages.
  @ Leave this blank to disable wiki.  Wiki pages are
  @ files within this subdirectory whose name is he wiki page title
  @ and with the suffix ".wiki".</p>
  
  entry_attribute("RSS Feed Title", 60, "rss-title", "rst", "");
  @ <p>The title of the RSS feed that publishes the changes to the
  @ repository. If left blank, the system will generate a generic
  @ title that, unfortunantly, not very helpful.</p>
  
  textarea_attribute("RSS Feed Description", 5, 60, "rss-description", "rsd", "");
  @ <p>The description of the RSS feed that publishes the changes to
  @ the repository. If left blank, the system will use the RSS Feed Title.
   
  @ <hr>
  @ <p><input type="submit"  name="submit" value="Apply Changes"></p>
  @ </form>
  db_end_transaction(0);
  style_footer();
}
