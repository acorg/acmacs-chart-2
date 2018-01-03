# -*- Makefile -*-
# Eugene Skepner 2017
# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

TARGETS = \
    $(ACMACS_CHART_LIB) \
    $(DIST)/chart-info \
    $(DIST)/chart-names \
    $(DIST)/chart-plot-spec \
    $(DIST)/chart-layout \
    $(DIST)/chart-convert \
    $(DIST)/chart-modify-projection \
    $(DIST)/chart-modify-plot-spec \
    $(DIST)/chart-titer-merging-report \
    $(DIST)/chart-stress

SOURCES = chart.cc titers.cc column-bases.cc bounding-ball.cc stress.cc \
    rjson-import.cc \
    factory-import.cc ace-import.cc acd1-import.cc lispmds-import.cc lispmds-token.cc \
    factory-export.cc ace-export.cc lispmds-export.cc lispmds-encode.cc chart-modify.cc

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/makefiles/Makefile.g++
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.vars

ACMACS_CHART_LIB_MAJOR = 2
ACMACS_CHART_LIB_MINOR = 0
ACMACS_CHART_LIB = $(DIST)/$(call shared_lib_name,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR))

CXXFLAGS += -Icc $(PKG_INCLUDES)
# ifneq ($(wildcard $(AD_INCLUDE)/acmacs-base/pch.$(shell uname).pch),)
#   CXXFLAGS += -include $(AD_INCLUDE)/acmacs-base/pch.$(shell uname).hh -verify-pch
# endif
# CXX := gtime $(CXX)

LDLIBS = \
	$(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
	$(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
	$$(pkg-config --libs liblzma) -lbz2 $(CXX_LIB)

PKG_INCLUDES = $(shell pkg-config --cflags liblzma)

# ----------------------------------------------------------------------

all: check-acmacsd-root $(TARGETS)

install: check-acmacsd-root install-headers $(TARGETS)
	$(call install_lib,$(ACMACS_CHART_LIB))
	ln -sf $(abspath dist)/chart-* $(AD_BIN)

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(ACMACS_CHART_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) | $(DIST) $(LOCATION_DB_LIB)
	@printf "%-16s %s\n" "SHARED" $@
	@$(call make_shared,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(ACMACS_CHART_LIB)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(ACMACS_CHART_LIB) $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
