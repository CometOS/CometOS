local_gmock_build_directory := $(project_build_directory)/support/gtest
gmock_library := $(local_gmock_build_directory)/gmock.a
gmock_main_library := $(local_gmock_build_directory)/gmock_main.a
gmock_main := $(local_gmock_build_directory)/gmock_main.o
gmock_all := $(local_gmock_build_directory)/gmock-all.o
gtest_all := $(local_gmock_build_directory)/gtest-all.o

# Gmock realted variables. Taken from the makefile
# bundled with gmock
CPPFLAGS += -isystem $(GTEST_DIR)/include -isystem $(GMOCK_DIR)/include
CXXFLAGS += -pthread -fpermissive -std=c++11 -g

GTEST_HEADERS = $(wildcard $(GTEST_DIR)/include/gtest/*.h) \
                $(wildcard $(GTEST_DIR)/include/gtest/internal/*.h)
GMOCK_HEADERS = $(wildcard $(GMOCK_DIR)/include/gmock/*.h) \
                $(wildcard $(GMOCK_DIR)/include/gmock/internal/*.h) \
                $(wildcard $(GTEST_HEADERS))
                
GTEST_SRCS_ = $(wildcard $(GTEST_DIR)/src/*.cc) $(wildcard $(GTEST_DIR)/src/*.h) $(GTEST_HEADERS)
GMOCK_SRCS_ = $(wildcard $(GMOCK_DIR)/src/*.cc) $(wildcard $(GMOCK_HEADERS))

gmock: $(gmock_library)

.PHONY: gmock

# gmock.a related targets
$(gmock_library): $(gmock_all) $(gtest_all)
	$(AR) $@ $^

# order-only prereq  | $(local_gmock_build_directory) is repeated as
# boiler plate code down below. TODO: refactor to a better solution

$(gtest_all): $(GTEST_SRCS_) | $(local_gmock_build_directory)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) $(CXXFLAGS) \
            -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(gmock_all): $(GMOCK_SRCS_) | $(local_gmock_build_directory)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) $(CXXFLAGS) \
            -c $(GMOCK_DIR)/src/gmock-all.cc -o $@ 


# gmock_main.a related targets
$(gmock_main_library) : $(gmock_all_new) $(gtest_all_new) $(gmock_main_new) | $(local_gmock_build_directory)
	$(AR) $(local_ARFLAGS) $@ $^

$(gmock_main): $(GMOCK_SRCS_) | $(local_gmock_build_directory)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) $(CXXFLAGS) \
            -c $(GMOCK_DIR)/src/gmock_main.cc -o $@


$(local_gmock_build_directory):
	@echo "Creating $(local_gmock_build_directory) dir"
	-@mkdir -p $(local_gmock_build_directory) 