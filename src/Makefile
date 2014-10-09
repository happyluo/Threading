# **********************************************************************
#
# Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
#
# <Email: luo (dot) xiaowei (at) hotmail (dot) com>
#
# **********************************************************************

top_srcdir	= ..

include $(top_srcdir)/config/Make.rules

LIBFILENAME	= $(call mklibfilename,Threading,$(VERSION))
SONAME		= $(call mksoname,Threading,$(SOVERSION))  
LIBNAME		= $(call mklibname,Threading)
TARGETS		= $(call mklibtargets,$(libdir)/$(LIBFILENAME),$(libdir)/$(SONAME),$(libdir)/$(LIBNAME))
	
SUBDIRS		=  Concurrency \
			  Logging \
			  Unicoder \
			  Util

OBJS		= $(wildcard *.o)

.PHONY: $(EVERYTHING) $(SUBDIRS)


ifeq ($(STATICLIBS),yes)
$(libdir)/$(LIBNAME): $(OBJS)
	rm -f $@
	$(call mklib,$@,$(OBJS))
else
$(libdir)/$(LIBFILENAME): $(OBJS)
	rm -f $@
	$(call mkshlib,$@,$(SONAME),$(OBJS),$(LINKWITH))

$(libdir)/$(SONAME): $(libdir)/$(LIBFILENAME)
	rm -f $@
	ln -s $(LIBFILENAME) $@

$(libdir)/$(LIBNAME): $(libdir)/$(SONAME)
	rm -f $@
	ln -s $(SONAME) $@
endif

install:: all
	$(call installlib,$(DESTDIR)$(install_libdir),$(libdir),$(LIBFILENAME),$(SONAME),$(LIBNAME))

all:: $(SUBDIRS) $(TARGETS)
	
$(SUBDIRS):
	@echo "making all in $@"
	@$(MAKE) all --directory=$@

$(EVERYTHING_EXCEPT_ALL)::
	@for subdir in $(SUBDIRS); \
        do \
            if test -d $$subdir ; \
            then \
                echo "making $@ in $$subdir"; \
                ( cd $$subdir && $(MAKE) $@ ) || exit 1; \
            fi; \
        done
