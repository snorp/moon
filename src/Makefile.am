
# we need to define this so that explicit dependencies on source files in subdirectories work.
# e.g. look at the config-dialog-gtk.lo dep on icon128.h at the bottom of the file
VPATH=pal/gtk


libmoon_includedir=$(includedir)/libmoon

lib_LTLIBRARIES = libmoon.la
noinst_LTLIBRARIES = libmoonxpi.la libmoon-static.la
noinst_SCRIPTS = xaml-to-header.exe

INCLUDES = $(MOON_CFLAGS) $(GALLIUM_CFLAGS) $(LLVM_CFLAGS) -Wall -DUSE_OPT_RGB24=1 -DG_LOG_DOMAIN=\"Moonlight\" -I$(top_srcdir)/plugin -I$(srcdir)/pal -I$(srcdir)/pal/@PAL@ -fPIC -fvisibility=hidden -fvisibility-inlines-hidden -DBUILDING_LIBMOON=1

libmoon_include_headers = 	\
	animation.h		\
	application.h		\
	applier.h		\
	asxparser.h		\
	audio.h			\
	authors.h		\
	bitmapcache.h		\
	bitmapimage.h		\
	bitmapsource.h		\
	border.h		\
	brush.h			\
	canvas.h		\
	capture.h		\
	cbinding.h		\
	clock.h			\
	collection.h		\
	color.h			\
	contentcontrol.h	\
	contentpresenter.h	\
	context.h		\
	context-cairo.h		\
	control.h		\
	cornerradius.h		\
	consent.h		\
	debug.h			\
	dependencyobject.h	\
	dependencyproperty.h	\
	deployment.h		\
	deepzoomimagetilesource.h	\
	designmode.h		\
	dirty.h			\
	downloader.h		\
	easing.h		\
	enums.h			\
	effect.h		\
	error.h			\
	eventargs.h		\
	fontmanager.h		\
	fonts.h			\
	fontfamily.h		\
	fontsource.h		\
	fontstretch.h		\
	fontstyle.h		\
	fontweight.h		\
	frameworkelement.h 	\
	gchandle.h		\
	geometry.h		\
	glyphs.h		\
	grid.h			\
	http-streaming.h	\
	icon128.h		\
	imagesource.h		\
	inputmethod.h		\
	inputscope.h		\
	keyboard.h		\
	layoutinformation.h	\
	libmoon.h		\
	list.h			\
	managedtypeinfo.h	\
	media.h			\
	mediaelement.h		\
	mediaplayer.h		\
	messaging.h		\
	moon-curves.h		\
	moonlightconfiguration.h\
	moon-path.h		\
	mp3.h			\
	multiscaleimage.h	\
	multiscalesubimage.h	\
	mutex.h			\
	notificationwindow.h	\
	namescope.h		\
	network.h		\
	openfile.h		\
	pal/pal.h		\
	pal/window.h		\
	pal/capture/pal-linux-capture.h	\
	pal/capture/v4l2/pal-v4l2-video-capture.h	\
	pal/gtk/clipboard-gtk.h	\
	pal/gtk/config-dialog-gtk.h\
	pal/gtk/im-gtk.h	\
	pal/gtk/install-dialog-gtk.h\
	pal/gtk/pal-gtk.h	\
	pal/gtk/pixbuf-gtk.h	\
	pal/gtk/window-gtk.h	\
	pal/gtk/nocodec-ui-gtk.h	\
	pal/gtk/debug-ui-gtk.h	\
	pal/gtk/nocodec-ui-gtk.h		\
	pal/cocoa/MLEvent.h	\
	pal/cocoa/MLTimer.h	\
	pal/cocoa/MLView.h		\
	pal/cocoa/clipboard-cocoa.h	\
	pal/cocoa/config-dialog-cocoa.h\
	pal/cocoa/im-cocoa.h	\
	pal/cocoa/install-dialog-cocoa.h\
	pal/cocoa/pal-cocoa.h	\
	pal/cocoa/pixbuf-cocoa.h	\
	pal/cocoa/window-cocoa.h	\
	pal/cocoa/debug-ui-cocoa.h	\
	pal/messaging/glib/pal-glib-msg.h	\
	pal/network/dbus/pal-dbus-network.h	\
	pal/network/pal-linux-network.h	\
	panel.h			\
	pipeline.h		\
	pipeline-asf.h		\
	pipeline-mp4.h		\
	playlist.h		\
	point.h			\
	popup.h			\
	printing.h		\
	projection.h		\
	propertypath.h		\
	provider.h		\
	ptr.h			\
	rect.h			\
	region.h		\
	resources.h		\
	richtextbox.h		\
	richtextlayout.h	\
	runtime.h		\
	security.h		\
	shape.h			\
	size.h			\
	style.h			\
	stylus.h		\
	surface.h		\
	surface-cairo.h		\
	tabnavigationwalker.h	\
	template.h		\
	textblock.h		\
	textbox.h		\
	textelement.h		\
	textlayout.h		\
	textoptions.h		\
	textpointer.h		\
	textselection.h		\
	thickness.h		\
	tilesource.h		\
	timeline.h		\
	timemanager.h		\
	timesource.h		\
	transform.h		\
	trigger.h		\
	type.h			\
	uielement.h		\
	uri.h			\
	usercontrol.h		\
	utils.h			\
	validators.h		\
	value.h			\
	weakrefmanager.h	\
	webbrowser.h		\
	writeablebitmap.h	\
	xaml.h			\
	xap.h			\
	yuv-converter.h

libmoon_la_SOURCES = 		\
	animation.cpp		\
	application.cpp		\
	applier.cpp		\
	asxparser.cpp		\
	audio.cpp		\
	bitmapcache.cpp		\
	bitmapimage.cpp		\
	bitmapsource.cpp	\
	border.cpp		\
	brush.cpp		\
	canvas.cpp		\
	capture.cpp		\
	cbinding.cpp		\
	clock.cpp		\
	collection.cpp		\
	color.cpp		\
	contentcontrol.cpp	\
	contentpresenter.cpp	\
	context.cpp		\
	context-cairo.cpp	\
	control.cpp		\
	cornerradius.cpp	\
	consent.cpp		\
	debug.cpp		\
	deepzoomimagetilesource.cpp \
	dependencyobject.cpp	\
	dependencyproperty.cpp	\
	dependencyproperty.g.cpp\
	deployment.cpp		\
	dirty.cpp		\
	dirty.h			\
	downloader.cpp		\
	easing.cpp		\
	effect.cpp		\
	enums.cpp		\
	error.cpp		\
	eventargs.cpp		\
	fontmanager.cpp		\
	fonts.cpp		\
	frameworkelement.cpp	\
	gchandle.cpp		\
	geometry.cpp		\
	glyphs.cpp		\
	grid.cpp		\
	http-streaming.cpp	\
	imagesource.cpp		\
	keyboard.cpp		\
	layoutinformation.cpp	\
	list.cpp		\
	managedtypeinfo.cpp	\
	media.cpp		\
	mediaelement.cpp	\
	mediaplayer.cpp		\
	messaging.cpp		\
	moon-curves.c		\
	moonlightconfiguration.cpp	\
	moon-path.c		\
	mp3.cpp			\
	multiscaleimage.cpp	\
	multiscalesubimage.cpp	\
	network.cpp				\
	notificationwindow.cpp	\
	namescope.cpp		\
	openfile.cpp		\
	pal/pal.cpp	\
	pal/window.cpp		\
	panel.cpp		\
	pipeline-asf.cpp	\
	pipeline.cpp		\
	pipeline-logo.cpp	\
	pipeline-mp4.cpp	\
	playlist.cpp		\
	point.cpp		\
	popup.cpp		\
	printing.cpp		\
	projection.cpp		\
	provider.cpp		\
	rect.cpp		\
	region.cpp		\
	resources.cpp		\
	richtextbox.cpp		\
	richtextlayout.cpp	\
	runtime.cpp		\
	security.cpp		\
	shape.cpp		\
	size.cpp		\
	style.cpp		\
	stylus.cpp		\
	surface.cpp		\
	surface-cairo.cpp	\
	tabnavigationwalker.cpp	\
	template.cpp		\
	textblock.cpp		\
	textbox.cpp		\
	textelement.cpp		\
	textlayout.cpp		\
	textpointer.cpp		\
	textselection.cpp	\
	thickness.cpp		\
	tilesource.cpp		\
	timeline.cpp		\
	timemanager.cpp		\
	timesource.cpp		\
	transform.cpp		\
	trigger.cpp		\
	type.cpp		\
	type-generated.cpp	\
	uielement.cpp		\
	uri.cpp			\
	usercontrol.cpp 	\
	utils.cpp		\
	validators.cpp		\
	value.cpp		\
	weakrefmanager.cpp	\
	webbrowser.cpp		\
	writeablebitmap.cpp	\
	xaml.cpp		\
	xap.cpp			\
	yuv-converter.cpp	\
	zip/crypt.h		\
	zip/ioapi.c		\
	zip/ioapi.h		\
	zip/unzip.c		\
	zip/unzip.h


if GTK_PAL
libmoon_la_SOURCES += \
	pal/gtk/clipboard-gtk.cpp	\
	pal/gtk/config-dialog-gtk.cpp\
	pal/gtk/im-gtk.cpp	\
	pal/gtk/install-dialog-gtk.cpp\
	pal/gtk/pal-gtk.cpp	\
	pal/gtk/pixbuf-gtk.cpp	\
	pal/gtk/window-gtk.cpp	\
	pal/gtk/nocodec-ui-gtk.cpp	\
	pal/gtk/debug-ui-gtk.cpp

# FIXME: This should be individually guarded ot the pal backends, not lumped with GTK
libmoon_la_SOURCES += \
	pal/capture/pal-linux-capture.cpp	\
	pal/capture/v4l2/pal-v4l2-video-capture.cpp	\
	pal/messaging/glib/pal-glib-msg.cpp	\
	pal/network/dbus/pal-dbus-network.cpp	\
	pal/network/pal-linux-network.cpp
endif

if COCOA_PAL
libmoon_la_OBJCFLAGS = $(AM_OBJCFLAGS)
libmoon_la_OBJCXXFLAGS = $(AM_OBJCXXFLAGS)

libmoon_la_SOURCES += \
	pal/cocoa/MLView.m		\
	pal/cocoa/MLEvent.m	\
	pal/cocoa/MLTimer.m	\
	pal/cocoa/clipboard-cocoa.m	\
	pal/cocoa/config-dialog-cocoa.m\
	pal/cocoa/im-cocoa.m	\
	pal/cocoa/install-dialog-cocoa.m\
	pal/cocoa/pal-cocoa.m	\
	pal/cocoa/pixbuf-cocoa.m	\
	pal/cocoa/window-cocoa.m	\
	pal/cocoa/debug-ui-cocoa.m
endif

if CODECS_SUPPORTED
INCLUDES += -I$(CODECS_PATH)/libmoon
endif

if INCLUDE_FFMPEG
libmoon_la_SOURCES += pipeline-ffmpeg.cpp pipeline-ffmpeg.h
endif

if INCLUDE_ALSA
libmoon_la_SOURCES += audio-alsa.cpp audio-alsa.h
endif

if INCLUDE_PULSEAUDIO
libmoon_la_SOURCES += audio-pulse.cpp audio-pulse.h
endif

if HAVE_CURL
libmoon_la_SOURCES += network-curl.cpp network-curl.h
endif

if HAVE_GALLIUM
libmoon_la_SOURCES += 		\
	surface-gallium.cpp	\
	surface-gallium.h	\
	surface-gallium-egl.cpp	\
	surface-gallium-egl.h	\
	context-gallium.cpp	\
	context-gallium.h
endif

if HAVE_GLX
libmoon_la_SOURCES += 	\
	surface-gl.cpp	\
	surface-gl.h	\
	context-gl.cpp	\
	context-gl.h    \
	surface-glx.cpp	\
	surface-glx.h	\
	context-glx.cpp	\
	context-glx.h
endif

EXTRA_DIST = \
	type.h.in \
	value.h.in \
	alpha-premul-table.inc \
	morton-layout-table.inc \
	fullscreen.xaml \
	incomplete-support.xaml \
	framerate-display.xaml \
	drm.xaml \
	xaml-to-header.cs \
	png-to-header.cs \
	zip/README \
	zip/ChangeLogUnzip \
	$(libmoon_include_headers)

libmoon_la_LIBADD = $(MOON_LIBS) $(MONO_LIBS) $(GALLIUM_LIBS) $(LLVM_LIBS)

libmoonxpi_la_LDFLAGS = -rpath /nowhere
libmoonxpi_la_LIBADD = $(MOON_XPI_LIBS) $(GALLIUM_LIBS) $(LLVM_LIBS)
libmoonxpi_la_SOURCES = $(libmoon_la_SOURCES)

# we do not link with gallium libs here, they are static libs, and whoever links with libmoon-static 
# must also lik with the gallium libs.
libmoon_static_la_SOURCES = $(libmoon_la_SOURCES)
libmoon_static_la_LDFLAGS = -static 
libmoon_static_la_LIBADD = $(MOON_LIBS)

runtime.lo: fullscreen.h incomplete-support.h drm.h framerate-display.h

fullscreen.h: fullscreen.xaml xaml-to-header.exe
	$(SYSTEM_MONO) xaml-to-header.exe fullscreen.xaml FULLSCREEN_MESSAGE

incomplete-support.h: incomplete-support.xaml xaml-to-header.exe
	$(SYSTEM_MONO) xaml-to-header.exe incomplete-support.xaml INCOMPLETE_SUPPORT_MESSAGE

framerate-display.h: framerate-display.xaml xaml-to-header.exe
	$(SYSTEM_MONO) xaml-to-header.exe framerate-display.xaml FRAMERATE_COUNTER_DISPLAY

drm.h: drm.xaml xaml-to-header.exe
	$(SYSTEM_MONO) xaml-to-header.exe drm.xaml DRM_MESSAGE

config-dialog-gtk.lo: icon128.h

icon128.h: $(srcdir)/../icons/icon128.png png-to-header.exe
	$(SYSTEM_MONO) png-to-header.exe $(srcdir)/../icons/icon128.png icon128.h ICON_128

xaml-to-header.exe: xaml-to-header.cs
	gmcs xaml-to-header.cs

png-to-header.exe: png-to-header.cs
	gmcs png-to-header.cs
