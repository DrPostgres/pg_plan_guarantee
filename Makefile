# contrib/pg_plan_guarantee/Makefile

MODULE_big = pg_plan_guarantee
OBJS = \
	$(WIN32RES) \
	pg_plan_guarantee.o

EXTENSION = pg_plan_guarantee
DATA = pg_plan_guarantee--1.4.sql pg_plan_guarantee--1.8--1.9.sql \
	pg_plan_guarantee--1.7--1.8.sql pg_plan_guarantee--1.6--1.7.sql \
	pg_plan_guarantee--1.5--1.6.sql pg_plan_guarantee--1.4--1.5.sql \
	pg_plan_guarantee--1.3--1.4.sql pg_plan_guarantee--1.2--1.3.sql \
	pg_plan_guarantee--1.1--1.2.sql pg_plan_guarantee--1.0--1.1.sql
PGFILEDESC = "pg_plan_guarantee - Guarantee that your plans will never change"

LDFLAGS_SL += $(filter -lm, $(LIBS))

REGRESS_OPTS = --temp-config $(top_srcdir)/contrib/pg_plan_guarantee/pg_plan_guarantee.conf
REGRESS = pg_plan_guarantee oldextversions
# Disabled because these tests require "shared_preload_libraries=pg_plan_guarantee",
# which typical installcheck users do not have (e.g. buildfarm clients).
NO_INSTALLCHECK = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_plan_guarantee
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
