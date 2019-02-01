# This is an automatically generated record.
# The area between QNX Internal Start and QNX Internal End is controlled by
# the QNX IDE properties.

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

#===== USEFILE - the file containing the usage message for the application. 
USEFILE=

# Next lines are for C++ projects only

EXTRA_SUFFIXES+=cxx cpp

#===== LDFLAGS - add the flags to the linker command line.
LDFLAGS+=-lang-c++ -Y _gpp

#===== CCFLAGS - add the flags to the C compiler command line. 
CCFLAGS+= \
	-DQNX  \
	-DQNX_SCREEN  \
	-DKZ_OPENGL_VERSION=KZ_OPENGL_ES_2_0  \
	-D__GLIBCXX__  \
	-DKANZI_COMPONENTS_API=  \
	-DKANZI_V8_API=  \
	-DPROJECT_PAGANI_C9  \
	-lang-c++  \
	-Wc,-std=gnu++0x  \
	-fexceptions  \
	-frtti  \
	-fPIC  \
	-Wall  \
	-Wextra  \
	-Wno-unused-parameter  \
	-Wno-unused-variable  \
	-Wno-unused-function  \
	-Wno-overlength-strings  \
	-Wno-missing-braces  \
	-Wno-switch  \
	-Wno-strict-aliasing  \
	-Wno-unused-but-set-variable  \
	-Y _gpp
CCFLAGS_g+=-DDEBUG

#===== EXTRA_INCVPATH - a space-separated list of directories to search for include files.
EXTRA_INCVPATH+= \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/headers/user/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/headers/system/common/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/headers/core/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/libraries/common/boost/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/headers/kanzi_components/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/headers/kanzi_v8/include  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/ssincludes

#===== EXTRA_LIBVPATH - a space-separated list of directories to search for library files.
EXTRA_LIBVPATH+= \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/lib/qnx_screen/ES2_Release  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/libraries/platforms/qnx-arm/freetype/lib  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/libraries/platforms/qnx-arm/libjpeg/lib  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/libraries/platforms/qnx-arm/libpng/lib  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/Kanzi-v3_2_0_261_80627-qnx_screen/libraries/platforms/qnx-arm/zlib/lib  \
	${PROJECT_ROOT}/../../../../PROJECTSDK/CMS_SDK/sslibs

#===== LIBS - a space-separated list of library items to be included in the link.
LIBS+= \
	-Bstatic  \
	kanzi_components  \
	applicationframework  \
	user  \
	core  \
	system  \
	png  \
	jpeg  \
	freetype  \
	z  \
	-Bdynamic  \
	screen  \
	m  \
	stdc++  \
	EGL  \
	GLESv2  \
	socket

include $(MKFILES_ROOT)/qmacros.mk
ifndef QNX_INTERNAL
QNX_INTERNAL=$(PROJECT_ROOT)/.qnx_internal.mk
endif
include $(QNX_INTERNAL)

include $(MKFILES_ROOT)/qtargets.mk

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))

