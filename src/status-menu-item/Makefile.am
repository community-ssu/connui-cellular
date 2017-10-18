hildondesktoplib_LTLIBRARIES = \
	connui-cellular-status-item.la

connui_cellular_status_item_ladir = $(hildondesktoplibdir)
connui_cellular_status_item_la_SOURCES = \
	status-item.c

connui_cellular_status_item_la_CFLAGS = \
	$(ICD2_CFLAGS) $(LIBHILDON_CFLAGS) $(HILDON_CFLAGS) $(CONNUI_CFLAGS) \
	$(OSSO_CFLAGS)
connui_cellular_status_item_la_LIBADD = \
	$(ICD2_LIBS) $(LIBHILDON_LIBS) $(HILDON_LIBS) $(CONNUI_LIBS) \
	$(OSSO_LIBS)

connui_cellular_status_item_la_LDFLAGS = -module -avoid-version

AM_CFLAGS = \
		$(ACCOUNTS_CFLAGS)
		-Wall -Werror \
		-DG_LOG_DOMAIN=\"$(PACKAGE)\"

AM_LIBS = $(ACCOUNTS_LIBS)

hildonstatusmenudesktopentry_DATA = connui-cellular.desktop
EXTRA_DIST = $(hildonstatusmenudesktopentry_DATA)
