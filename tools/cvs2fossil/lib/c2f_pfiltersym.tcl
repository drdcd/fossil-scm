## -*- tcl -*-
# # ## ### ##### ######## ############# #####################
## Copyright (c) 2007 Andreas Kupries.
#
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.
#
# This software consists of voluntary contributions made by many
# individuals.  For exact contribution history, see the revision
# history and logs, available at http://fossil-scm.hwaci.com/fossil
# # ## ### ##### ######## ############# #####################

## Pass IV. Coming after the symbol collation pass this pass now
## removes all revisions and symbols referencing any of the excluded
## symbols from the persistent database.

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                               ; # Required runtime.
package require snit                                  ; # OO system.
package require vc::tools::misc                       ; # Text formatting.
package require vc::tools::log                        ; # User feedback.
package require vc::fossil::import::cvs::state        ; # State storage.
package require vc::fossil::import::cvs::integrity    ; # State storage integrity checks.
package require vc::fossil::import::cvs::project::sym ; # Project level symbols

# # ## ### ##### ######## ############# #####################
## Register the pass with the management

vc::fossil::import::cvs::pass define \
    FilterSymbols \
    {Filter symbols, remove all excluded pieces} \
    ::vc::fossil::import::cvs::pass::filtersym

# # ## ### ##### ######## ############# #####################
## 

snit::type ::vc::fossil::import::cvs::pass::filtersym {
    # # ## ### ##### ######## #############
    ## Public API

    typemethod setup {} {
	# Define names and structure of the persistent state of this
	# pass.

	state reading symbol
	state reading blocker
	state reading parent
	state reading preferedparent
	state reading revision
	state reading branch
	state reading tag

	state writing noop {
	    id    INTEGER NOT NULL  PRIMARY KEY, -- tag/branch reference
	    noop  INTEGER NOT NULL
	}
	return
    }

    typemethod load {} {
	# Pass manager interface. Executed to load data computed by
	# this pass into memory when this pass is skipped instead of
	# executed.

	# The results of this pass are fully in the persistent state,
	# there is nothing to load for the next one.
	return
    }

    typemethod run {} {
	# Pass manager interface. Executed to perform the
	# functionality of the pass.

	# The removal of excluded symbols and everything referencing
	# to them is done completely in the database.

	state transaction {
	    FilterExcludedSymbols
	    MutateSymbols
	    AdjustParents
	    RefineSymbols

	    # Strict integrity enforces that all meta entries are in
	    # the same LOD as the revision using them. At this point
	    # this may not be true any longer. If a NTDB was excluded
	    # then all revisions it shared with the trunk were moved
	    # to the trunk LOD, however their meta entries will still
	    # refer to the now gone LOD symbol. This is fine however,
	    # it will not affect our ability to use the meta entries
	    # to distinguish and group revisions into changesets. It
	    # should be noted that we cannot simply switch the meta
	    # entries over to the trunk either, as that may cause the
	    # modified entries to violate the unique-ness constrain
	    # set on that table.
	    integrity metarelaxed
	}

	log write 1 filtersym "Filtering completed"
	return
    }

    typemethod discard {} {
	# Pass manager interface. Executed for all passes after the
	# run passes, to remove all data of this pass from the state,
	# as being out of date.
	return
    }

    # # ## ### ##### ######## #############
    ## Internal methods

    proc FilterExcludedSymbols {} {
	log write 3 filtersym "Filter out excluded symbols and users"

	# We pull all the excluded symbols together into a table for
	# easy reference by the upcoming DELETE and other statements.
	# ('x IN table' clauses).

	set excl [project::sym excluded]

	state run {
	    CREATE TEMPORARY TABLE excludedsymbols AS
	    SELECT sid
	    FROM   symbol
	    WHERE  type = $excl
	}

	# First we have to handle the possibility of an excluded
	# NTDB. This is a special special case there we have to
	# regraft the revisions which are shared between the NTDB and
	# Trunk onto the trunk, preventing their deletion later. We
	# have code for that in 'file', however that operated on the
	# in-memory revision objects, which we do not have here. We do
	# the same now without object, by directly manipulating the
	# links in the database.

	array set ntdb {}
	array set link {}

	foreach {id parent transfer} [state run {
	    SELECT R.rid, R.parent, R.dbchild
	    FROM  revision R, symbol S
	    WHERE R.lod = S.sid
	    AND   S.sid IN excludedsymbols
	    AND   R.isdefault
	}] {
	    set ntdb($id) $parent
	    if {$transfer eq ""} continue
	    set link($id) $transfer
	}

	foreach joint [array names link] {
	    # The joints are the highest NTDB revisions which are
	    # shared with their respective trunk. We disconnect from
	    # their NTDB children, and make them parents of their
	    # 'dbchild'. The associated 'dbparent' is squashed
	    # instead. All parents of the joints are moved to the
	    # trunk as well.

	    set tjoint $link($joint)
	    set tlod [lindex [state run {
		SELECT lod FROM revision WHERE rid = $tjoint
	    }] 0]

	    # Covnert db/parent/child into regular parent/child links.
	    state run {
		UPDATE revision SET dbparent = NULL, parent = $joint  WHERE rid = $tjoint ;
		UPDATE revision SET dbchild  = NULL, child  = $tjoint WHERE rid = $joint  ;
	    }
	    while {1} {
		# Move the NTDB trunk revisions to trunk.
		state run {
		    UPDATE revision SET lod = $tlod, isdefault = 0 WHERE rid = $joint
		}
		set last $joint
		set joint $ntdb($joint)
		if {![info exists ntdb($joint)]} break
	    }

	    # Reached the NTDB basis in the trunk. Finalize the
	    # parent/child linkage and squash the branch parent symbol
	    # reference.

	    state run {
		UPDATE revision SET child   = $last WHERE rid = $joint ;
		UPDATE revision SET bparent = NULL  WHERE rid = $last  ;
	    }
	}

	# Now that the special case is done we can simply kill all the
	# revisions, tags, and branches referencing any of the
	# excluded symbols in some way. This is easy as we do not have
	# to select them again and again from the base tables any
	# longer.

	state run {
	    DELETE FROM revision WHERE lod IN excludedsymbols;
	    DELETE FROM tag      WHERE lod IN excludedsymbols;
	    DELETE FROM tag      WHERE sid IN excludedsymbols;
	    DELETE FROM branch   WHERE lod IN excludedsymbols;
	    DELETE FROM branch   WHERE sid IN excludedsymbols;

	    DROP TABLE excludedsymbols;
	}
	return
    }

    proc MutateSymbols {} {
	# Next, now that we know which symbols are what we look for
	# file level tags which are actually converted as branches
	# (project level, and vice versa), and move them to the
	# correct tables.

	# # ## ### ##### ######## #############

	log write 3 filtersym "Mutate symbols, preparation"

	set branch [project::sym branch]
	set tag    [project::sym tag]

	set tagstomutate [state run {
	    SELECT T.tid, T.fid, T.lod, T.sid, T.rev
	    FROM tag T, symbol S
	    WHERE T.sid = S.sid
	    AND S.type = $branch
	}]

	set branchestomutate [state run {
	    SELECT B.bid, B.fid, B.lod, B.sid, B.root, B.first, B.bra
	    FROM branch B, symbol S
	    WHERE B.sid = S.sid
	    AND S.type = $tag
	}]

	log write 4 filtersym "Changing [nsp [expr {[llength $tagstomutate]/5}] tag] into branches"
	log write 4 filtersym "Changing [nsp [expr {[llength $branchestomutate]/7}] branch branches] into tags"

	# # ## ### ##### ######## #############

	log write 3 filtersym "Mutate tags to branches"

	foreach {id fid lod sid rev} $tagstomutate {
	    state run {
		DELETE FROM tag WHERE tid = $id ;
		INSERT INTO branch (bid, fid,  lod,  sid,  root, first, bra, pos)
		VALUES             ($id, $fid, $lod, $sid, $rev, NULL,  '',  -1);
	    }
	}

	log write 3 filtersym "Ok."

	# # ## ### ##### ######## #############

	log write 3 filtersym "Mutate branches to tags"

	foreach {id fid lod sid root first bra} $branchestomutate {
	    state run {
		DELETE FROM branch WHERE bid = $id ;
		INSERT INTO tag (tid, fid,  lod,  sid,  rev)
		VALUES          ($id, $fid, $lod, $sid, $root);
	    }
	}

	log write 3 filtersym "Ok."

	# # ## ### ##### ######## #############
	return
    }

    # Adjust the parents of symbols to their preferred parents.

    # If a file level ymbol has a preferred parent that is different
    # than its current parent, and if the preferred parent is an
    # allowed parent of the symbol in this file, then we graft the
    # aSymbol onto its preferred parent.

    proc AdjustParents {} {
	log write 3 filtersym "Adjust parents, loading data in preparation"

	# We pull important maps once into memory so that we do quick
	# hash lookup later when processing the graft candidates.

	# Tag/Branch names ...
	array set sn [state run { SELECT T.tid, S.name FROM tag T,    symbol S WHERE T.sid = S.sid }]
	array set sn [state run { SELECT B.bid, S.name FROM branch B, symbol S WHERE B.sid = S.sid }]
	# Symbol names ...
	array set sx [state run { SELECT L.sid, L.name FROM symbol L }]
	# Files and projects.
	array set fpn {}
	foreach {id fn pn} [state run {
		SELECT F.fid, F.name, P.name
		FROM   file F, project P
		WHERE  F.pid = P.pid
	}] { set fpn($id) [list $fn $pn] }

	set tagstoadjust [state run {
	    SELECT T.tid, T.fid, T.lod, P.pid, S.name, R.rev, R.rid
	    FROM tag T, preferedparent P, symbol S, revision R
	    WHERE T.sid = P.sid
	    AND   T.lod != P.pid
	    AND   P.pid = S.sid
	    AND   S.name != ':trunk:'
	    AND   T.rev = R.rid	
	}]

	set branchestoadjust [state run {
	    SELECT B.bid, B.fid, B.lod, B.pos, P.pid, S.name, R.rev, R.rid
	    FROM branch B, preferedparent P, symbol S, revision R
	    WHERE B.sid = P.sid
	    AND   B.lod != P.pid
	    AND   P.pid = S.sid
	    AND   S.name != ':trunk:'
	    AND   B.root = R.rid	
	}]

	set tmax [expr {[llength $tagstoadjust] / 7}]
	set bmax [expr {[llength $branchestoadjust] / 8}]

	log write 4 filtersym "Reparenting at most [nsp $tmax tag]"
	log write 4 filtersym "Reparenting at most [nsp $bmax branch branches]"

	log write 3 filtersym "Adjust tag parents"

	# Find the tags whose current parent (lod) is not the prefered
	# parent, the prefered parent is not the trunk, and the
	# prefered parent is a possible parent per the tag's revision.

	set fmt %[string length $tmax]s
	set mxs [format $fmt $tmax]

	set n 0
	foreach {id fid lod pid preferedname revnr rid} $tagstoadjust {

	    # BOTTLE-NECK ...
	    #
	    # The check if the candidate (pid) is truly viable is
	    # based finding the branch as possible parent, and done
	    # now instead of as part of the already complex join.
	    #
	    # ... AND P.pid IN (SELECT B.sid
	    #                   FROM branch B
	    #                   WHERE B.root = R.rid)

	    if {![lindex [state run {
		SELECT COUNT(*)
		FROM branch B
		WHERE  B.sid  = $pid
		AND    B.root = $rid
	    }] 0]} {
		incr tmax -1
		set  mxs [format $fmt $tmax]
		continue
	    }

	    #
	    # BOTTLE-NECK ...

	    # The names for use in the log output are retrieved
	    # separately, to keep the join selecting the adjustable
	    # tags small, not burdened with the dereferencing of links
	    # to name.

	    set tagname $sn($id)
	    set oldname $sx($lod)
	    struct::list assign $fpn($fid) fname prname

	    # Do the grafting.

	    log write 4 filtersym "\[[format $fmt $n]/$mxs\] $prname : Grafting tag '$tagname' on $fname/$revnr from '$oldname' onto '$preferedname'"
	    state run { UPDATE tag SET lod = $pid WHERE tid = $id ; }
	    incr n
	}

	log write 3 filtersym "Reparented [nsp $n tag]"

	log write 3 filtersym "Adjust branch parents"

	# Find the branches whose current parent (lod) is not the
	# prefered parent, the prefered parent is not the trunk, and
	# the prefered parent is a possible parent per the branch's
	# revision.

	set fmt %[string length $bmax]s
	set mxs [format $fmt $bmax]

	set n 0
	foreach {id fid lod pos pid preferedname revnr rid} $branchestoadjust {

	    # BOTTLE-NECK ...
	    #
	    # The check if the candidate (pid) is truly viable is
	    # based on the branch positions in the spawning revision,
	    # and done now instead of as part of the already complex
	    # join.
	    #
	    # ... AND P.pid IN (SELECT BX.sid
	    #                   FROM branch BX
	    #                   WHERE BX.root = R.rid
	    #                   AND   BX.pos > B.pos)

	    if {![lindex [state run {
		SELECT COUNT(*)
		FROM branch B
		WHERE  B.sid  = $pid
		AND    B.root = $rid
		AND    B.pos  > $pos
	    }] 0]} {
		incr bmax -1
		set  mxs [format $fmt $bmax]
		continue
	    }

	    #
	    # BOTTLE-NECK ...

	    # The names for use in the log output are retrieved
	    # separately, to keep the join selecting the adjustable
	    # tags small, not burdened with the dereferencing of links
	    # to name.

	    set braname $sn($id)
	    set oldname $sx($lod)
	    struct::list assign $fpn($fid) fname prname

	    # Do the grafting.

	    log write 4 filtersym "\[[format $fmt $n]/$mxs\] $prname : Grafting branch '$braname' on $fname/$revnr from '$oldname' onto '$preferedname'"
	    state run { UPDATE tag SET lod = $pid WHERE tid = $id ; }
	    incr n
	}

	log write 3 filtersym "Reparented [nsp $n branch branches]"
	return
    }

    proc RefineSymbols {} {
	# Tags and branches are marked as normal/noop based on the op
	# of their revision.

	log write 3 filtersym "Refine symbols (no-op or not?)"

	log write 4 filtersym "    Regular tags"
	state run {
	    INSERT INTO noop
	    SELECT T.tid, 0
	    FROM tag T, revision R
	    WHERE T.rev  = R.rid
	    AND   R.op  != 0 -- 0 == nothing
	}

	log write 4 filtersym "    No-op tags"
	state run {
	    INSERT INTO noop
	    SELECT T.tid, 1
	    FROM tag T, revision R
	    WHERE T.rev  = R.rid
	    AND   R.op   = 0 -- nothing
	}

	log write 4 filtersym "    Regular branches"
	state run {
	    INSERT INTO noop
	    SELECT B.bid, 0
	    FROM branch B, revision R
	    WHERE B.root = R.rid
	    AND   R.op  != 0 -- nothing
	}

	log write 4 filtersym "    No-op branches"
	state run {
	    INSERT INTO noop
	    SELECT B.bid, 1
	    FROM branch B, revision R
	    WHERE B.root = R.rid
	    AND   R.op   = 0 -- nothing
	}
	return
    }

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hasinstances   no ; # singleton
    pragma -hastypeinfo    no ; # no introspection
    pragma -hastypedestroy no ; # immortal

    # # ## ### ##### ######## #############
}

namespace eval ::vc::fossil::import::cvs::pass {
    namespace export filtersym
    namespace eval filtersym {
	namespace import ::vc::fossil::import::cvs::state
	namespace import ::vc::fossil::import::cvs::integrity
	namespace eval project {
	    namespace import ::vc::fossil::import::cvs::project::sym
	}
	namespace import ::vc::tools::misc::nsp
	namespace import ::vc::tools::log
	log register filtersym
    }
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::pass::filtersym 1.0
return
