# **********************************************************************
#
# Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
#
# <Email: luo (dot) xiaowei (at) hotmail (dot) com>
#
# **********************************************************************

top_srcdir	= .

include $(top_srcdir)/config/Make.rules

SUBDIRS		= config src include #test

INSTALL_SUBDIRS	= $(install_bindir) $(install_libdir) $(install_includedir) \
	$(install_configdir) $(install_mandir)

install:: install-common
	@for subdir in $(INSTALL_SUBDIRS); \
	do \
	    if test ! -d $(DESTDIR)$$subdir ; \
	    then \
		echo "Creating $(DESTDIR)$$subdir..." ; \
		mkdir -p $(DESTDIR)$$subdir ; \
		chmod a+rx $(DESTDIR)$$subdir ; \
	    fi ; \
	done
ifeq ($(create_runpath_symlink),yes)
	@if test -h $(embedded_runpath_prefix) ; \
	then \
	     if `\rm -f $(embedded_runpath_prefix) 2>/dev/null`; \
              then echo "Removed symbolic link $(embedded_runpath_prefix)"; fi \
        fi
	@if ! test -d $(embedded_runpath_prefix) ; \
	then \
	     if `ln -s $(prefix) $(embedded_runpath_prefix) 2>/dev/null`; \
              then echo "Created symbolic link $(embedded_runpath_prefix) --> $(prefix)"; fi \
	fi
endif

$(EVERYTHING)::
	@for subdir in $(SUBDIRS); \
	do \
	    echo "making $@ in $$subdir"; \
	    ( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	done

test::
	@python $(top_srcdir)/allTests.py
