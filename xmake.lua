add_requires("conan::wil/1.0.240803.1",
	{
		alias = "wil",
		configs = {settings = {"compiler=clang"}}
	})

add_requires("minhook")

add_syslinks("kernel32", "user32", "gdi32", "winspool", "shell32", "ole32", "oleaut32", "uuid", "comdlg32", "advapi32")

includes("common")
includes("fastcorr")
includes("injector")
includes("target")