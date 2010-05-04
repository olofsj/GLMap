AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin 

bin_PROGRAMS = mapgenerator

mapgenerator_SOURCES = mapgenerator.c mapgenerator_utils.c
mapgenerator_LDADD = -lexpat -lproj -ltriangle
mapgenerator_LDFLAGS =

