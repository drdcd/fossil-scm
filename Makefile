#!/usr/bin/make
#
#### The toplevel directory of the source tree.  Fossil can be built
#    in a directory that is separate from the source tree.  Just change
#    the following to point from the build directory to the src/ folder.
#
SRCDIR = ./src

#### C Compiler and options for use in building executables that
#    will run on the platform that is doing the build.  This is used
#    to compile code-generator programs as part of the build process.
#    See TCC below for the C compiler for building the finished binary.
#
BCC = gcc -g -O2

#### The suffix to add to executable files.  ".exe" for windows.
#    Nothing for unix.
#
E =

#### C Compile and options for use in building executables that 
#    will run on the target platform.  This is usually the same
#    as BCC, unless you are cross-compiling.  This C compiler builds
#    the finished binary for fossil.  The BCC compiler above is used
#    for building intermediate code-generator tools.
#
#TCC = gcc -O6
#TCC = gcc -g -O0 -Wall -fprofile-arcs -ftest-coverage
TCC = gcc -g -Os -Wall

#### Extra arguments for linking the finished binary.  Fossil needs
#    to link against the Z-Lib compression library.  There are no
#    other dependencies.  We sometimes add the -static option here
#    so that we can build a static executable that will run in a
#    chroot jail.
#
LIB = -lz $(LDFLAGS)
# If you're on OpenSolaris, and probably other Solaris versions,
# you'll need:
# LIB += -lsocket


#### Tcl shell for use in running the fossil testsuite.
#
TCLSH = tclsh

# You should not need to change anything below this line
###############################################################################
include $(SRCDIR)/main.mk
