target("fastcorr")
	set_kind("library")
	add_files("*.cpp")
	add_includedirs(".", {public = true})