target("target")
	set_kind("shared")
	
	add_cxflags("-fms-extensions")
	set_languages("c11", "cxx20")

	add_deps("common")
	add_deps("fastcorr")
	
	add_packages("minhook")
	add_packages("wil")

	add_files("*.cpp")
	