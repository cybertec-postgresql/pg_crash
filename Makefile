MODULE_big = pg_crash
OBJS = pg_crash.o $(WIN32RES)
PGFILEDESC = "pg_crash"

EXTENSION = pg_crash

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
