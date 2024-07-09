#pragma once

#include <Windows.h>

/*
* Functions for allowing hooking to elevated processes
*/

HANDLE get_elevated_handle(unsigned long pid);