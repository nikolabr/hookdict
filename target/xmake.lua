target("target")
	set_kind("shared")
	
	add_cxflags("-fms-extensions")
	add_cxxflags("-stdlib=libc++")

	-- Needs to be linked statically, because there is no guarantee that the target exe can find the GCC DLLs
	add_shflags("-static-libgcc -static-libstdc++", {force = true})
	
	set_languages("c11", "cxx20")

	add_deps("common")
	add_deps("fastcorr")
	
	add_packages("minhook")
	add_packages("wil")

	add_files("*.cpp")
	