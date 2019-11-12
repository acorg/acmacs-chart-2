# -*- Makefile -*-
# ----------------------------------------------------------------------

TARGETS = \
  $(ACMACS_CHART_LIB) \
  $(DIST)/chart-info \
  $(DIST)/chart-names \
  $(DIST)/chart-table \
  $(DIST)/chart-plot-spec \
  $(DIST)/chart-layout \
  $(DIST)/chart-convert \
  $(DIST)/chart-txt-to-ace \
  $(DIST)/chart-modify-projection \
  $(DIST)/chart-modify-plot-spec \
  $(DIST)/chart-titer-merging-report \
  $(DIST)/chart-titer-merging-from-layers \
  $(DIST)/chart-titer-merging-for-serum \
  $(DIST)/chart-stress \
  $(DIST)/chart-stresses \
  $(DIST)/chart-transformation \
  $(DIST)/chart-relax-test \
  $(DIST)/chart-relax \
  $(DIST)/chart-relax-existing \
  $(DIST)/chart-relax-disconnected \
  $(DIST)/chart-relax-incremental \
  $(DIST)/chart-relax-save-intermediate-layouts \
  $(DIST)/chart-grid-test \
  $(DIST)/chart-relax-grid \
  $(DIST)/chart-error-lines \
  $(DIST)/chart-common \
  $(DIST)/chart-procrustes \
  $(DIST)/chart-reorient \
  $(DIST)/chart-reorient-projections \
  $(DIST)/chart-degradation-resolver \
  $(DIST)/chart-html \
  $(DIST)/chart-serum-circles \
  $(DIST)/chart-update \
  $(DIST)/chart-join \
  $(DIST)/chart-remove-antigens-sera \
  $(DIST)/chart-remove-antigens-by-full-name \
  $(DIST)/chart-keep-antigens-sera \
  $(DIST)/chart-keep-antigens-titrated-against-sera \
  $(DIST)/chart-serum-titers-check \
  $(DIST)/chart-column-bases \
  $(DIST)/chart-list-antigens-without-titers \
  $(DIST)/chart-remove-layers \
  $(DIST)/chart-remove-projections \
  $(DIST)/chart-merge \
  $(DIST)/chart-combine-projections \
  $(DIST)/chart-export \
  $(DIST)/chart-homologous-pairs \
  $(DIST)/chart-map-resolution-test \
  $(DIST)/chart-antigens-without-passages \
  $(DIST)/chart-find-chart-with-antigens \
  $(DIST)/chart-time-series-for-country \
  $(DIST)/test-titer-iterator \
  $(DIST)/test-chart-modify \
  $(DIST)/test-chart-create-from-scratch \
  $(DIST)/test-chart-from-text \
  $(DIST)/test-chart-merge \
  $(DIST)/test-get-titers \
  $(DIST)/test-clone-projection \
  $(DIST)/test-chart-clone \
  $(DIST)/test-chart-proportion-to-dontcare \
  $(DIST)/test-chart-relax

SOURCES = \
  chart.cc titers.cc column-bases.cc bounding-ball.cc stress.cc optimize.cc randomizer.cc serum-circle.cc \
  merge.cc common.cc \
  rjson-import.cc \
  factory-import.cc ace-import.cc acd1-import.cc lispmds-import.cc lispmds-token.cc \
  factory-export.cc ace-export.cc lispmds-export.cc lispmds-encode.cc chart-modify.cc \
  procrustes.cc grid-test.cc serum-line.cc blobs.cc map-resolution-test.cc

ALGLIB = alglib-3.13.0
ALGLIB_SOURCES = optimization.cpp ap.cpp alglibinternal.cpp linalg.cpp alglibmisc.cpp \
  dataanalysis.cpp statistics.cpp specialfunctions.cpp solvers.cpp
ALGLIB_CXXFLAGS = -DAE_COMPILE_MINLBFGS -DAE_COMPILE_PCA -DAE_COMPILE_MINCG -g -MMD -O3 -mfpmath=sse -mtune=intel -fPIC -std=c++11 -Icc -Wall
ifeq ($(CXX_TYPE),GCC8)
ALGLIB_CXXFLAGS += -Wno-maybe-uninitialized
endif

all: install

include $(ACMACSD_ROOT)/share/Makefile.config

# ----------------------------------------------------------------------

ACMACS_CHART_LIB_MAJOR = 2
ACMACS_CHART_LIB_MINOR = 0
ACMACS_CHART_LIB = $(DIST)/$(call shared_lib_name,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR))

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(XZ_LIBS) $(BZ2_LIBS) $(CXX_LIBS)

# ----------------------------------------------------------------------

install: install-headers $(TARGETS)
	$(call install_lib,$(ACMACS_CHART_LIB))
	$(call symbolic_link_wildcard,$(DIST)/chart-*,$(AD_BIN))
	$(call symbolic_link_wildcard,$(abspath bin)/chart-*,$(AD_BIN))

test: install
	test/test
.PHONY: test

# ----------------------------------------------------------------------

$(ACMACS_CHART_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) $(patsubst %.cpp,$(BUILD)/%.o,$(ALGLIB_SOURCES)) | $(DIST)
	$(call echo_shared_lib,$@)
	$(call make_shared_lib,libacmacschart,$(ACMACS_CHART_LIB_MAJOR),$(ACMACS_CHART_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(ACMACS_CHART_LIB)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(ACMACS_CHART_LIB) $(LDLIBS) $(AD_RPATH)

$(BUILD)/%.o: cc/$(ALGLIB)/%.cpp | $(BUILD) install-headers
	$(call echo_compile,$<)
	$(CXX) $(ALGLIB_CXXFLAGS) -c -o $@ $(abspath $<)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
