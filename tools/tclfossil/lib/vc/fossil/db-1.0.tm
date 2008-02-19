## -*- tcl -*-
# # ## ### ##### ######## ############# #####################
## Copyright (c) 2008 Mark Janssen.
#
# This software is licensed as described in the file LICENSE, which
# you should have received as part of this distribution.
#
# This software consists of voluntary contributions made by many
# individuals.  For exact contribution history, see the revision
# history and logs, available at http://fossil-scm.hwaci.com/fossil
# # ## ### ##### ######## ############# #####################

## Db commands

# # ## ### ##### ######## ############# #####################
## Requirements

package require Tcl 8.5                             ; # Required runtime.
package require snit                                ; # OO system.
package require sqlite3
package require vc::fossil::schema      1.0         ; # Fossil repo schema

package provide vc::fossil::db 1.0

# # ## ### ##### ######## ############# #####################
##



namespace eval ::vc::fossil {

    snit::type db {
	typevariable schemadir [file join [file dirname [info script]] schema]
        typevariable dbcmd [namespace current]::sqldb

	typemethod create_repository {filename} {
	    if {[file exists $filename]} {
		ui panic "file already exists: $filename"
	    }
	    db init_database $filename [schema repo1] [schema repo2]
	}

	typemethod init_database {filename schema args} {
	    sqlite3 $dbcmd $filename
	    $dbcmd transaction {
		$dbcmd eval $schema
		foreach schema $args {
		    $dbcmd eval $schema
		}
	    }
	    $dbcmd close
	}
    }
}