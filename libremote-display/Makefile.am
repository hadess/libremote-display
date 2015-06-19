include $(top_srcdir)/Makefile.decl

EXTRA_DIST = remote-display.symbols

BUILT_GIRSOURCES =

lib_LTLIBRARIES = libremote-display.la

libremote_display_la_PUBLICSOURCES =			\
	remote-display-error.c				\
	remote-display-enum-types.c			\
	remote-display-manager.c			\
	remote-display-device.c

libremote_display_la_SOURCES =				\
	$(libremote_display_la_PUBLICSOURCES)		\
	remote-display-private.h			\
	remote-display-device-private.h			\
	remote-display-device-airplay.c			\
	remote-display-device-airplay.h

libremote_display_la_LIBADD = $(REMOTE_DISPLAY_LIBS) $(LIBS)

libremote_display_la_LDFLAGS =				\
	-version-info $(GCLIB_LT_VERSION)		\
	-no-undefined					\
	$(AM_LDFLAGS)

#FIXME
#	-export-symbols $(srcdir)/remote-display.symbols

REMOTE_DISPLAY_HEADER_FILES =				\
	remote-display.h				\
	remote-display-error.h				\
	remote-display-manager.h			\
	remote-display-device.h				\
	remote-display-enum-types.h

remote_displaydir = $(includedir)/$(PACKAGE)-$(REMOTE_DISPLAY_API_VERSION)/$(PACKAGE)
remote_display_HEADERS =					\
	$(REMOTE_DISPLAY_HEADER_FILES)				\
	remote-display-enum-types.h

AM_CPPFLAGS =						\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-DREMOTE_DISPLAY_LOCALEDIR=\"$(localedir)\"

AM_CFLAGS = $(REMOTE_DISPLAY_CFLAGS) $(COMMON_CFLAGS) $(WARN_CFLAGS) $(DISABLE_DEPRECATED)

BUILT_SOURCES = \
               remote-display-enum-types.c \
               remote-display-enum-types.h

remote-display-enum-types.h: $(REMOTE_DISPLAY_HEADER_FILES)
	$(AM_V_GEN) ($(GLIB_MKENUMS) \
		--fhead "#ifndef __REMOTE_DISPLAY_ENUM_TYPES_H__\n#define __REMOTE_DISPLAY_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define REMOTE_DISPLAY_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n\n/**\n * SECTION:remote-display-enum-types\n * @short_description: RemoteDisplay enumerated types\n * @include: libremote-display/remote-display.h\n *\n * The enumerated types defined and used by remote-display.\n **/" \
		--ftail "G_END_DECLS\n\n#endif /* __REMOTE_DISPLAY_ENUM_TYPES_H__ */" $^ > xgen-$(@F) \
		&& mv -f xgen-$(@F) $@)

remote-display-enum-types.c: $(REMOTE_DISPLAY_HEADER_FILES) remote-display-enum-types.h
	$(AM_V_GEN) ($(GLIB_MKENUMS) \
		--fhead "#include <libremote-display/remote-display.h>\n#include <libremote-display/remote-display-enum-types.h>" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" $^ > xgen-$(@F) \
		&& mv -f xgen-$(@F) $@)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(top_srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_srcdir)
CLEANFILES =

if HAVE_INTROSPECTION
introspection_files = $(libremote_display_la_PUBLICSOURCES) $(remote_display_HEADERS)

RemoteDisplay-1.0.gir: libremote-display.la
RemoteDisplay_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0 Soup-2.4
RemoteDisplay_1_0_gir_PACKAGES = gobject-2.0 gmodule-2.0 glib-2.0 gio-2.0 libsoup-2.4
RemoteDisplay_1_0_gir_CFLAGS = -I$(top_srcdir) -I$(top_builddir)
RemoteDisplay_1_0_gir_LIBS = libremote-display.la
RemoteDisplay_1_0_gir_SCANNERFLAGS = \
	--symbol-prefix=remote_display \
	--identifier-prefix=RemoteDisplay \
	--pkg-export=remote-display-1.0 \
	--c-include="remote-display/remote-display.h"
RemoteDisplay_1_0_gir_FILES = $(introspection_files)
INTROSPECTION_GIRS += RemoteDisplay-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

endif # HAVE_INTROSPECTION

TEST_PROGS += test-remote-display
noinst_PROGRAMS = $(TEST_PROGS)

test_remote_display_LDADD = libremote-display.la $(REMOTE_DISPLAY_LIBS)

MAINTAINERCLEANFILES = Makefile.in

-include $(top_srcdir)/git.mk
