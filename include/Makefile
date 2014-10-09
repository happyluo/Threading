# **********************************************************************
#
# Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
#
# <Email: luo (dot) xiaowei (at) hotmail (dot) com>
#
# **********************************************************************

top_srcdir	= ..

include $(top_srcdir)/config/Make.rules

SUBDIRS		= Build \
		  Concurrency \
		  Logging \
		  Unicoder \
		  Util

install::
	@for i in *.h ; \
	do \
	    echo "Installing $$i" ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(install_includedir)/$$i ; \
	    chmod a+r $(DESTDIR)$(install_includedir)/$$i ; \
	done

$(EVERYTHING)::
	@for subdir in $(SUBDIRS); \
	do \
	    echo "making $@ in $$subdir"; \
	    ( cd $$subdir && $(MAKE) $@ ) || exit 1; \
	done
