target("injector")
	add_cxxflags("-stdlib=libc++")
	
	set_languages("c11", "cxx20")
	add_ldflags("-static-libgcc -static-libstdc++", {force = true})

	add_deps("common")

	add_packages("rpclib")
	add_packages("boost")
	
	set_kind("binary")
	add_files("*.cpp")

	after_install("mingw", function (target)
			    print("Copying libc++ DLLs...")
			    local p = format("%s/bin/", target:installdir())
			    os.cp("$(mingw)/i686-w64-mingw32/bin/*.dll", p)
	end)
