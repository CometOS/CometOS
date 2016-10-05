test_module_executable :=  $(project_build_directory)/$(unit_test_executable_name)
test_module_source_files := $(wildcard $(project_directory)/test/*.cc) \
                            $(wildcard $(project_directory)/test/*/*.cc) 

test_module_build_prerequisites := $(wildcard $(project_directory)/test/*/*) \
                                   $(wildcard $(project_directory)/test/*.*) \
                                   $(OBJS) \
                                   $(gmock_library) 

test_module_library_dependencies :=  -L"$(OMNETPP_LIB_SUBDIR)" \
                         -L"$(OMNETPP_LIB_DIR)" \
                         -L$(COMETOS_PROJ) \
                         -lpthread -loppsim$D     


$(test_module_executable): $(test_module_build_prerequisites) 
	@echo "Creating the unit test executable"
	@echo $(LDFLAGS)
	$(CXX) $(CXXFLAGS) $(COPTS) -DOMNETPP $(test_module_source_files) $(OBJS) \
	$(test_module_library_dependencies) $(INCLUDE_PATH) \
	$(gmock_library) -o $@ \

#-l$(cometos_libraries)
	
