target("injector")
	add_cxflags("-fms-extensions")
	set_languages("c11", "cxx20")

	add_packages("wil")
	
	add_deps("common")
	
	set_kind("binary")
	add_files("*.cpp")