# **********************************************************************
#
# Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
#
# <Email: luo (dot) xiaowei (at) hotmail (dot) com>
#
# **********************************************************************

top_srcdir	= ../..

include $(top_srcdir)/config/Make.rules

install::
	@if test ! -d $(DESTDIR)$(install_includedir)/Concurrency ; \
	then \
	    echo "Creating $(DESTDIR)$(install_includedir)/Concurrency..." ; \
	    $(call mkdir,$(DESTDIR)$(install_includedir)/Concurrency) ; \
	fi

	@for i in *.h ; \
	do \
	    echo "Installing $$i" ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(install_includedir)/Concurrency/$$i ; \
	    chmod a+r $(DESTDIR)$(install_includedir)/Concurrency/$$i ; \
	done
