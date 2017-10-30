# -*- Makefile -*-
# Eugene Skepner 2017
# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

N=_n

TARGETS = \
    $(ACMACS_CHART_LIB)

SOURCES = chart.cc

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/makefiles/Makefile.g++
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.vars

ACMACS_CHART_LIB = $(DIST)/libacmacschart$(N).so

CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WEVERYTHING) $(WARNINGS) -Icc -I$(AD_INCLUDE) $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)
LDLIBS = -L$(AD_LIB) -lacmacsbase -llocationdb $$(pkg-config --libs liblzma) $(CXX_LIB)

PKG_INCLUDES = $(shell pkg-config --cflags liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root install-headers $(ACMACS_CHART_LIB) $(BACKEND)

install: check-acmacsd-root install-headers $(ACMACS_CHART_LIB) $(BACKEND)
	$(call install_lib,$(ACMACS_CHART_LIB))
	@#ln -sf $(abspath bin)/acmacs-chart-* $(AD_BIN)

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(ACMACS_CHART_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) | $(DIST) $(LOCATION_DB_LIB)
	@echo "SHARED     " $@ # '<--' $^
	@$(CXX) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(ACMACS_MAP_DRAW_LIB)
	@echo "LINK       " $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(ACMACS_MAP_DRAW_LIB) $(LDLIBS)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
