MODULE_big = pg_crash
OBJS = pg_crash.o $(WIN32RES)
PGFILEDESC = "pg_crash"

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
