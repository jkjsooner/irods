#
# config/common.mk
#
# Set common variables for iRODS builds.
#

# API and utility library name.  Do not include 'lib' prefix or '.a' suffix.
LIBRARY_NAME =	RodsAPIs

LIBRARY =	$(libCoreObjDir)/lib$(LIBRARY_NAME).a

# All library includes
LIB_INCLUDES =	-I$(libCoreIncDir) -I$(libApiIncDir) -I$(libMd5IncDir)

# All server includes
SVR_INCLUDES =	-I$(svrCoreIncDir) -I$(svrReIncDir) -I$(svrIcatIncDir) \
		-I$(svrDriversIncDir)

configMk = $(configDir)/config.mk
platformMk = $(configDir)/platform.mk

# Depend upon configuration file changes and changes to any
# of the include files
DEPEND = $(configMk) $(platformMk) \
		$(wildcard $(libCoreIncDir)/*.hpp) \
		$(wildcard $(libMd5IncDir)/*.hpp) \
		$(wildcard $(libApiIncDir)/*.hpp) \
		$(wildcard $(svrCoreIncDir)/*.hpp) \
		$(wildcard $(svrApiIncDir)/*.hpp) \
		$(wildcard $(svrDriversIncDir)/*.hpp) \
		$(wildcard $(svrIcatIncDir)/*.hpp) \
		$(wildcard $(svrReIncDir)/*.hpp) \
