# This file evaluates the variables set on the command line and acts on them.
# targ is set to something such as POLG68K, RTPXMSP3, etc.
# RELENV should be N for ebsenv, Y otherwise.
# v=v for debug, v=o for optimized.

########################################################################
targ = LINUX

# set the following to "Y" for RTUPnP, "N" otherwise.
dortupnp = "Y"


########################################################################

root    = ..
kernel  = c:

#source path
lib_to_http_common_path      = $(HTTP_PATH)/source
lib_to_http_example_path      = $(HTTP_PATH)/examples
#include path
lib_to_htp_common_include   = $(HTTP_PATH)/source

lib_to_rtptarget              = $(HTTP_PATH)/../../rtplatform/v0.4.2/source/linux
lib_to_rtpgeneric             = $(HTTP_PATH)/../../rtplatform/v0.4.2/source/generic
lib_to_rtptarget_include      = $(HTTP_PATH)/../../rtplatform/v0.4.2/include/linux
lib_to_rtpgeneric_include     = $(HTTP_PATH)/../../rtplatform/v0.4.2/include

########################################################################

known = "N"

ifeq ($(targ),LINUX)
known = "Y"
# Set the compiler type for later conditionals
compiler = GNUCOMPILER
endif

ifneq ($(known),"Y")
.PHONY: error
error:
ifneq ($(targ),)
	@echo Incorrect target: $(targ)
else
	@echo Please set the targ and v variables.
endif
	@echo Please edit
endif

########################################################################

ifeq ($(compiler),GNUCOMPILER)
# Set the file extensions
obj = o
lib = a
exe = out
endif

############################ THE END ###################################
