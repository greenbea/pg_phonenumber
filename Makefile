MODULES = pg_phonenumber
EXTENSION = pg_phonenumber
DATA = $(EXTENSION)--*.sql
MODULES = pg_phonenumber

PG_CPPFLAGS := -I/usr/include/phonenumbers -fPIC
PG_LDFLAGS := -lphonenumber
SHLIB_LINK  := -lphonenumber


PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)