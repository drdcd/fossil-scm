# -----------------------------------------------------------------------------
# Tool packages. Main control module for importing from a CVS repository.

# -----------------------------------------------------------------------------
# Requirements

package require Tcl 8.4
package require cvs             ; # Frontend, reading from source repository
package require vc::fossil::ws  ; # Backend,  writing to destination repository.
package require vc::tools::log  ; # User feedback

namespace eval ::vc::fossil::import::cvs {
    vc::tools::log::system import
    namespace import ::vc::tools::log::write
}

# -----------------------------------------------------------------------------
# API

# Configuration
#
#	vc::fossil::import::cvs::configure key value - Set configuration
#
#	Legal keys:	-nosign		<bool>, default false
#			-debugcommit	<bool>, default false
#			-stopat		<int>,  default :none:
#
# Functionality
#
#	vc::fossil::import::cvs::run src dst         - Perform an import.

# -----------------------------------------------------------------------------
# API Implementation - Functionality

proc ::vc::fossil::import::cvs::configure {key value} {
    variable nosign
    variable stopat

    switch -exact -- $key {
	-debugcommit {
	    if {![string is boolean -strict $value]} {
		return -code error "Expected boolean, got \"$value\""
	    }
	    vc::fossil::ws::debugcommit $value
	}
	-nosign {
	    if {![string is boolean -strict $value]} {
		return -code error "Expected boolean, got \"$value\""
	    }
	    set nosign $value
	}
	-stopat {
	    set stopat $value
	}
	default {
	    return -code error "Unknown switch $key, expected one of \
                                   -debugcommit, -nosign, or -stopat"
	}
    }
    return
}

# Import the CVS repository found at directory 'src' into the new
# fossil repository at 'dst'.

proc ::vc::fossil::import::cvs::run {src dst} {
    variable stopat

    cvs::at       $src  ; # Define location of CVS repository

    cvs::scan           ; # Gather revision data from the archives
    cvs::csets          ; # Group changes into sets
    cvs::rtree          ; # Build revision tree (trunk only right now).

    set tot 0.0
    set nto 0

    write 0 import {Begin conversion}
    write 0 import {Setting up workspaces}

    cvs::workspace      ; # cd's to workspace
    vc::fossil::ws::new ; # Uses cwd as workspace to connect to.

    set ntrunk [cvs::ntrunk] ; set ntfmt %[string length $ntrunk]s
    set nmax   [cvs::ncsets] ; set nmfmt %[string length $nmax]s

    cvs::foreach_cset cset [cvs::root] {
	write 0 import "ChangeSet [format $nmfmt $cset] @ [format $ntfmt $nto]/$ntrunk ([format %6.2f [expr {$nto*100.0/$ntrunk}]]%)"
	Statistics [OneChangeSet $cset]
    }

    write 0 import "========= [string repeat = 61]"
    write 0 import "Imported $nto [expr {($nto == 1) ? "changeset" : "changesets"}]"
    write 0 import "Within [format %.2f $tot] seconds (avg [format %.2f [expr {$tot/$nto}]] seconds/changeset)"

    if {$stopat == $cset} return

    cvs::wsclear
    vc::fossil::ws::destination $dst
    write 0 import Ok.
    return
}

# -----------------------------------------------------------------------------
# Internal operations - Import a single changeset.

proc ::vc::fossil::import::cvs::Statistics {sec} {
    upvar 1 tot tot nto nto ntrunk ntrunk

    # No statistics if the commit was stopped before it was run
    if {$sec eq ""} return

    incr nto

    set tot [expr {$tot + $sec}]
    set avg [expr {$tot/$nto}]
    set max [expr {$ntrunk * $avg}]
    set rem [expr {$max - $tot}]

    write 3 import "st avg [format %.2f $avg] sec"
    write 3 import "st run [format %7.2f $tot] sec [format %6.2f [expr {$tot/60}]] min [format %5.2f [expr {$tot/3600}]] hr"
    write 3 import "st end [format %7.2f $max] sec [format %6.2f [expr {$max/60}]] min [format %5.2f [expr {$max/3600}]] hr"
    write 3 import "st rem [format %7.2f $rem] sec [format %6.2f [expr {$rem/60}]] min [format %5.2f [expr {$rem/3600}]] hr"
    return
}

proc ::vc::fossil::import::cvs::OneChangeSet {cset} {
    variable nosign
    variable stopat

    if {$stopat == $cset} {
	vc::fossil::ws::commit 1 cvs2fossil $nosign \
	    [cvs::wssetup $cset] ::cvs::wsignore
	write 0 import Stopped.
	return -code break
    }

    set usec [lindex [time {
	foreach {uuid ad rm ch} \
	    [vc::fossil::ws::commit 0 cvs2fossil $nosign \
		 [cvs::wssetup $cset] ::cvs::wsignore] \
	    break
    } 1] 0]
    cvs::uuid $cset $uuid

    set sec [expr {$usec/1e6}]

    write 2 import "== $uuid +${ad}-${rm}*${ch}"
    write 2 import "st in  [format %.2f $sec] sec"

    return $sec
}

# -----------------------------------------------------------------------------

namespace eval ::vc::fossil::import::cvs {
    variable debugcommit 0  ; # Debug the commit operation.
    variable nosign      0  ; # Require signing
    variable stopat      {} ; # Stop nowhere

    namespace export run configure
}

# -----------------------------------------------------------------------------
# Ready

package provide vc::fossil::import::cvs 1.0
return
