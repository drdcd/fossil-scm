#### OS-specific configuration for building Fossil on OpenSolaris systems.
#    NOTE: You will need to have GNU Make installed to use this.
#

#### The suffix to add to executable files.  ".exe" for windows.
#    Nothing for unix.
#
E =

#### The directory into which object code files should be written.
#
OBJDIR = ./obj

#### The following variable definitions decide which features are turned on or
#    of when building Fossil.  Comment out the features which are not needed by
#    this platform.
#
#ENABLE_STATIC = 1	# we want a static build
ENABLE_SSL = 1		# we are using SSL
ENABLE_SOCKET = 1	# we are using libsocket (OpenSolaris and Solaris)
ENABLE_NSL = 1		# we are using libnsl library (OpenSolaris and Solaris)
ENABLE_I18N = 1		# we are using i18n settings

