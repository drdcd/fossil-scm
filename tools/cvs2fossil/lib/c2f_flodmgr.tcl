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

## Lines of Development in a file (Symbols, and the trunk).

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.4                             ; # Required runtime.
package require snit                                ; # OO system.

# # ## ### ##### ######## ############# #####################
## 

snit::type ::vc::fossil::import::cvs::file::lodmgr {
    # # ## ### ##### ######## #############
    ## Public API

    constructor {} {
	return
    }

    # # ## ### ##### ######## #############
    ## State

    # # ## ### ##### ######## #############
    ## Internal methods

    # # ## ### ##### ######## #############
    ## Configuration

    pragma -hastypeinfo    no  ; # no type introspection
    pragma -hasinfo        no  ; # no object introspection
    pragma -hastypemethods no  ; # type is not relevant.
    pragma -simpledispatch yes ; # simple fast dispatch

    # # ## ### ##### ######## #############
}

namespace eval ::vc::fossil::import::cvs::file {
    namespace export lodmgr
}

# # ## ### ##### ######## ############# #####################
## Ready

package provide vc::fossil::import::cvs::file::lodmgr 1.0
return
