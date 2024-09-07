add_rules("mode.debug", "mode.release")

add_requires("minhook")
add_requires("rpclib")

add_requires("boost", {configs = {outcome = true}})

add_syslinks("kernel32", "user32", "gdi32", "winspool", "shell32", "ole32", "oleaut32", "uuid", "comdlg32", "advapi32")

includes("common")
includes("fastcorr")
includes("target")
includes("injector")

