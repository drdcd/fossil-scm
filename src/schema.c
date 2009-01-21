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
** This file contains string constants that implement the database schema.
*/
#include "config.h"
#include "schema.h"

/*
** The database schema for the ~/.fossil configuration database.
*/
const char zConfigSchema[] = 
@ -- This file contains the schema for the database that is kept in the
@ -- ~/.fossil file and that stores information about the users setup.
@ --
@ CREATE TABLE global_config(
@   name TEXT PRIMARY KEY,
@   value TEXT
@ );
;

#if INTERFACE
/*
** The content tables have a content version number which rarely
** changes.  The aux tables have an arbitrary version number (typically
** a date) which can change frequently.  When the content schema changes,
** we have to execute special procedures to update the schema.  When
** the aux schema changes, all we need to do is rebuild the database.
*/
#define CONTENT_SCHEMA  "1"
#define AUX_SCHEMA      "2006-12-23"

#endif /* INTERFACE */


/*
** The schema for a repository database.  
**
** Schema1[] contains parts of the schema that are fixed and unchanging
** across versions.  Schema2[] contains parts of the schema that can
** change from one version to the next.  The information in Schema2[]
** can be reconstructed from the information in Schema1[].
*/
const char zRepositorySchema1[] = 
@ -- The BLOB and DELTA tables contain all records held in the repository.
@ --
@ -- The BLOB.CONTENT column is always compressed using libz.  This
@ -- column might hold the full text of the record or it might hold
@ -- a delta that is able to reconstruct the record from some other
@ -- record.  If BLOB.CONTENT holds a delta, then a DELTA table entry
@ -- will exist for the record and that entry will point to another
@ -- entry that holds the source of the delta.  Deltas can be chained.
@ --
@ CREATE TABLE blob(
@   rid INTEGER PRIMARY KEY,        -- Record ID
@   rcvid INTEGER,                  -- Origin of this record
@   size INTEGER,                   -- Size of content. -1 for a phantom.
@   uuid TEXT UNIQUE NOT NULL,      -- SHA1 hash of the content
@   content BLOB,                   -- Compressed content of this record
@   CHECK( length(uuid)==40 AND rid>0 )
@ );
@ CREATE TABLE delta(
@   rid INTEGER PRIMARY KEY,                 -- Record ID
@   srcid INTEGER NOT NULL REFERENCES blob   -- Record holding source document
@ );
@ CREATE INDEX delta_i1 ON delta(srcid);
@
@ -- Whenever new blobs are received into the repository, an entry
@ -- in this table records the source of the blob.
@ --
@ CREATE TABLE rcvfrom(
@   rcvid INTEGER PRIMARY KEY,      -- Received-From ID
@   uid INTEGER REFERENCES user,    -- User login
@   mtime DATETIME,                 -- Time or receipt
@   nonce TEXT UNIQUE,              -- Nonce used for login
@   ipaddr TEXT                     -- Remote IP address.  NULL for direct.
@ );
@
@ -- Information about users
@ --
@ CREATE TABLE user(
@   uid INTEGER PRIMARY KEY,        -- User ID
@   login TEXT,                     -- login name of the user
@   pw TEXT,                        -- password
@   cap TEXT,                       -- Capabilities of this user
@   cookie TEXT,                    -- WWW login cookie
@   ipaddr TEXT,                    -- IP address for which cookie is valid
@   cexpire DATETIME,               -- Time when cookie expires
@   info TEXT,                      -- contact information
@   photo BLOB                      -- JPEG image of this user
@ );
@
@ -- The VAR table holds miscellanous information about the repository.
@ -- in the form of name-value pairs.
@ --
@ CREATE TABLE config(
@   name TEXT PRIMARY KEY NOT NULL,  -- Primary name of the entry
@   value CLOB,                      -- Content of the named parameter
@   CHECK( typeof(name)='text' AND length(name)>=1 )
@ );
@
@ -- Artifacts that should not be processed are identified in the
@ -- "shun" table.  Artifacts that are control-file forgeries or
@ -- spam or artifacts whose contents violate administrative policy
@ -- can be shunned in order to prevent them from contaminating
@ -- the repository.
@ --
@ -- Shunned artifacts do not exist in the blob table.  Hence they
@ -- have not artifact ID (rid) and we thus must store their full
@ -- UUID.
@ --
@ CREATE TABLE shun(uuid UNIQUE);
@
@ -- Artifacts that should not be pushed are stored in the "private"
@ -- table.  Private artifacts are omitted from the "unclustered" and
@ -- "unsent" tables.
@ --
@ CREATE TABLE private(rid INTEGER PRIMARY KEY);
@
@ -- An entry in this table describes a database query that generates a
@ -- table of tickets.
@ --
@ CREATE TABLE reportfmt(
@    rn integer primary key,  -- Report number
@    owner text,              -- Owner of this report format (not used)
@    title text,              -- Title of this report
@    cols text,               -- A color-key specification
@    sqlcode text             -- An SQL SELECT statement for this report
@ );
@ INSERT INTO reportfmt(title,cols,sqlcode) VALUES('All Tickets','#ffffff Key:
@ #f2dcdc Active
@ #e8e8e8 Review
@ #cfe8bd Fixed
@ #bde5d6 Tested
@ #cacae5 Deferred
@ #c8c8c8 Closed','SELECT
@   CASE WHEN status IN (''Open'',''Verified'') THEN ''#f2dcdc''
@        WHEN status=''Review'' THEN ''#e8e8e8''
@        WHEN status=''Fixed'' THEN ''#cfe8bd''
@        WHEN status=''Tested'' THEN ''#bde5d6''
@        WHEN status=''Deferred'' THEN ''#cacae5''
@        ELSE ''#c8c8c8'' END AS ''bgcolor'',
@   substr(tkt_uuid,1,10) AS ''#'',
@   datetime(tkt_mtime) AS ''mtime'',
@   type,
@   status,
@   subsystem,
@   title
@ FROM ticket');
@
@ -- Some ticket content (such as the originators email address or contact
@ -- information) needs to be obscured to protect privacy.  This is achieved
@ -- by storing an SHA1 hash of the content.  For display, the hash is
@ -- mapped back into the original text using this table.  
@ --
@ -- This table contains sensitive information and should not be shared
@ -- with unauthorized users.
@ --
@ CREATE TABLE concealed(
@   hash TEXT PRIMARY KEY,
@   content TEXT
@ );
;

const char zRepositorySchema2[] =
@ -- Filenames
@ --
@ CREATE TABLE filename(
@   fnid INTEGER PRIMARY KEY,    -- Filename ID
@   name TEXT UNIQUE             -- Name of file page
@ );
@
@ -- Linkages between manifests, files created by that manifest, and
@ -- the names of those files.
@ --
@ -- pid==0 if the file is added by check-in mid.
@ -- fid==0 if the file is removed by check-in mid.
@ --
@ CREATE TABLE mlink(
@   mid INTEGER REFERENCES blob,        -- Manifest ID where change occurs
@   pid INTEGER REFERENCES blob,        -- File ID in parent manifest
@   fid INTEGER REFERENCES blob,        -- Changed file ID in this manifest
@   fnid INTEGER REFERENCES filename,   -- Name of the file
@   pfnid INTEGER REFERENCES filename   -- Previous name. 0 if unchanged
@ );
@ CREATE INDEX mlink_i1 ON mlink(mid);
@ CREATE INDEX mlink_i2 ON mlink(fnid);
@ CREATE INDEX mlink_i3 ON mlink(fid);
@ CREATE INDEX mlink_i4 ON mlink(pid);
@
@ -- Parent/child linkages
@ --
@ CREATE TABLE plink(
@   pid INTEGER REFERENCES blob,    -- Parent manifest
@   cid INTEGER REFERENCES blob,    -- Child manifest
@   isprim BOOLEAN,                 -- pid is the primary parent of cid
@   mtime DATETIME,                 -- the date/time stamp on cid
@   UNIQUE(pid, cid)
@ );
@ CREATE INDEX plink_i2 ON plink(cid);
@
@ -- Events used to generate a timeline
@ --
@ CREATE TABLE event(
@   type TEXT,                      -- Type of event
@   mtime DATETIME,                 -- Date and time when the event occurs
@   objid INTEGER PRIMARY KEY,      -- Associated record ID
@   uid INTEGER REFERENCES user,    -- User who caused the event
@   bgcolor TEXT,                   -- Color set by 'bgcolor' property
@   euser TEXT,                     -- User set by 'user' property
@   user TEXT,                      -- Name of the user
@   ecomment TEXT,                  -- Comment set by 'comment' property
@   comment TEXT                    -- Comment describing the event
@ );
@ CREATE INDEX event_i1 ON event(mtime);
@
@ -- A record of phantoms.  A phantom is a record for which we know the
@ -- UUID but we do not (yet) know the file content.
@ --
@ CREATE TABLE phantom(
@   rid INTEGER PRIMARY KEY         -- Record ID of the phantom
@ );
@
@ -- Unclustered records.  An unclustered record is a record (including
@ -- a cluster records themselves) that is not mentioned by some other
@ -- cluster.
@ --
@ -- Phantoms are usually included in the unclustered table.  A new cluster
@ -- will never be created that contains a phantom.  But another repository
@ -- might send us a cluster that contains entries that are phantoms to
@ -- us.
@ --
@ CREATE TABLE unclustered(
@   rid INTEGER PRIMARY KEY         -- Record ID of the unclustered file
@ );
@
@ -- Records which have never been pushed to another server.  This is
@ -- used to reduce push operations to a single HTTP request in the
@ -- common case when one repository only talks to a single server.
@ --
@ CREATE TABLE unsent(
@   rid INTEGER PRIMARY KEY         -- Record ID of the phantom
@ );
@
@ -- Each baseline or manifest can have one or more tags.  A tag
@ -- is defined by a row in the next table.
@ -- 
@ -- Wiki pages are tagged with "wiki-NAME" where NAME is the name of
@ -- the wiki page.  Tickets changes are tagged with "ticket-UUID" where 
@ -- UUID is the indentifier of the ticket.  Tags used to assign symbolic
@ -- names to baselines are branches are of the form "sym-NAME" where
@ -- NAME is the symbolic name.
@ --
@ CREATE TABLE tag(
@   tagid INTEGER PRIMARY KEY,       -- Numeric tag ID
@   tagname TEXT UNIQUE              -- Tag name.
@ );
@ INSERT INTO tag VALUES(1, 'bgcolor');         -- TAG_BGCOLOR
@ INSERT INTO tag VALUES(2, 'comment');         -- TAG_COMMENT
@ INSERT INTO tag VALUES(3, 'user');            -- TAG_USER
@ INSERT INTO tag VALUES(4, 'hidden');          -- TAG_HIDDEN
@ INSERT INTO tag VALUES(5, 'private');         -- TAG_PRIVATE
@ INSERT INTO tag VALUES(6, 'cluster');         -- TAG_CLUSTER
@ INSERT INTO tag VALUES(7, 'newbranch');       -- TAG_NEWBRANCH
@ INSERT INTO tag VALUES(8, 'closed');          -- TAG_CLOSED
@
@ -- Assignments of tags to baselines.  Note that we allow tags to
@ -- have values assigned to them.  So we are not really dealing with
@ -- tags here.  These are really properties.  But we are going to
@ -- keep calling them tags because in many cases the value is ignored.
@ --
@ CREATE TABLE tagxref(
@   tagid INTEGER REFERENCES tag,   -- The tag that added or removed
@   tagtype INTEGER,                -- 0:cancel  1:single  2:branch
@   srcid INTEGER REFERENCES blob,  -- Artifact of tag. 0 for propagated tags
@   origid INTEGER REFERENCES blob, -- check-in holding propagated tag
@   value TEXT,                     -- Value of the tag.  Might be NULL.
@   mtime TIMESTAMP,                -- Time of addition or removal
@   rid INTEGER REFERENCE blob,     -- Artifact tag is applied to
@   UNIQUE(rid, tagid)
@ );
@ CREATE INDEX tagxref_i1 ON tagxref(tagid, mtime);
@
@ -- Template for the TICKET table
@ --
@ -- NB: when changing the schema of the TICKET table here, also make the
@ -- same change in tktsetup.c.
@ --
@ CREATE TABLE ticket(
@   -- Do not change any column that begins with tkt_
@   tkt_id INTEGER PRIMARY KEY,
@   tkt_uuid TEXT UNIQUE,
@   tkt_mtime DATE,
@   -- Add as many field as required below this line
@   type TEXT,
@   status TEXT,
@   subsystem TEXT,
@   priority TEXT,
@   severity TEXT,
@   foundin TEXT,
@   private_contact TEXT,
@   resolution TEXT,
@   title TEXT,
@   comment TEXT
@ );
;

/*
** Predefined tagid values
*/
#if INTERFACE
# define TAG_BGCOLOR    1     /* Set the background color for display */
# define TAG_COMMENT    2     /* The check-in comment */
# define TAG_USER       3     /* User who made a checking */
# define TAG_HIDDEN     4     /* Do not display or sync */
# define TAG_PRIVATE    5     /* Display but do not sync */
# define TAG_CLUSTER    6     /* A cluster */
# define TAG_NEWBRANCH  7     /* First check-in of a new named branch */
# define TAG_CLOSED     8     /* Do not display this check-in as a leaf */
#endif
#if EXPORT_INTERFACE
# define MAX_INT_TAG    8     /* The largest pre-assigned tag id */
#endif

/*
** The schema for the locate FOSSIL database file found at the root
** of very check-out.  This database contains the complete state of
** the checkout.
*/
const char zLocalSchema[] =
@ -- The VVAR table holds miscellanous information about the local database
@ -- in the form of name-value pairs.  This is similar to the VAR table
@ -- table in the repository except that this table holds information that
@ -- is specific to the local checkout.
@ --
@ -- Important Variables:
@ --
@ --     repository        Full pathname of the repository database
@ --     user-id           Userid to use
@ --
@ CREATE TABLE vvar(
@   name TEXT PRIMARY KEY NOT NULL,  -- Primary name of the entry
@   value CLOB,                      -- Content of the named parameter
@   CHECK( typeof(name)='text' AND length(name)>=1 )
@ );
@
@ -- Each entry in the vfile table represents a single file in the
@ -- current checkout.
@ --
@ -- The file.rid field is 0 for files or folders that have been
@ -- added but not yet committed.
@ --
@ -- Vfile.chnged is 0 for unmodified files, 1 for files that have
@ -- been edited or which have been subjected to a 3-way merge.  
@ -- Vfile.chnged is 2 if the file has been replaced from a different
@ -- version by the merge and 3 if the file has been added by a merge.
@ -- The difference between vfile.chnged==2 and a regular add is that
@ -- with vfile.chnged==2 we know that the current version of the file
@ -- is already in the repository.
@ -- 
@ --
@ CREATE TABLE vfile(
@   id INTEGER PRIMARY KEY,           -- ID of the checked out file
@   vid INTEGER REFERENCES blob,      -- The baseline this file is part of.
@   chnged INT DEFAULT 0,             -- 0:unchnged 1:edited 2:m-chng 3:m-add
@   deleted BOOLEAN DEFAULT 0,        -- True if deleted 
@   rid INTEGER,                      -- Originally from this repository record
@   mrid INTEGER,                     -- Based on this record due to a merge
@   mtime INTEGER,                    -- Modification time of file on disk
@   pathname TEXT,                    -- Full pathname relative to root
@   origname TEXT,                    -- Original pathname. NULL if unchanged
@   UNIQUE(pathname,vid)
@ );
@
@ -- This table holds a record of uncommitted merges in the local
@ -- file tree.  If a VFILE entry with id has merged with another
@ -- record, there is an entry in this table with (id,merge) where
@ -- merge is the RECORD table entry that the file merged against.
@ -- An id of 0 here means the version record itself.
@
@ CREATE TABLE vmerge(
@   id INTEGER REFERENCES vfile,      -- VFILE entry that has been merged
@   merge INTEGER,                    -- Merged with this record
@   UNIQUE(id, merge)
@ );
@   
;
