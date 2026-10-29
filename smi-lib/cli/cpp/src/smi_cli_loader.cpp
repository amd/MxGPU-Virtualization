/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "smi_cli_loader.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef _WIN64
	#include <windows.h>
#elif defined(__linux__)
	#include <dlfcn.h>
#endif

SmiDynamicLoader::SmiDynamicLoader(const char* win_dll_name, const char* linux_so_name,
				   bool global_scope)
{
#ifdef _WIN64
	(void)linux_so_name;
	(void)global_scope;
	name_ = win_dll_name;

	char systemPath[MAX_PATH];
	UINT systemDirLength = GetSystemDirectoryA(systemPath, MAX_PATH);
	if (systemDirLength == 0) {
		std::cout << "Error GetSystemDirectoryA" << std::endl;
		exit(1);
	}
	strncat_s(systemPath, sizeof(systemPath), win_dll_name, MAX_PATH - strlen(systemPath) - 1);
	handle_ = static_cast<void*>(LoadLibraryA(systemPath));
#elif defined(__linux__)
	(void)win_dll_name;
	name_	  = linux_so_name;
	int flags = RTLD_NOW;
	if (global_scope)
		flags |= RTLD_GLOBAL;
	handle_ = dlopen(linux_so_name, flags);
#else
	(void)win_dll_name;
	(void)linux_so_name;
	(void)global_scope;
	handle_ = nullptr;
#endif
}

SmiDynamicLoader::~SmiDynamicLoader()
{
	if (handle_ == nullptr)
		return;
#ifdef _WIN64
	FreeLibrary(static_cast<HMODULE>(handle_));
#elif defined(__linux__)
	dlclose(handle_);
#endif
	handle_ = nullptr;
}

void* SmiDynamicLoader::get_symbol(const char* name) const
{
	if (handle_ == nullptr)
		return nullptr;
#ifdef _WIN64
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#elif defined(__linux__)
	return dlsym(handle_, name);
#else
	(void)name;
	return nullptr;
#endif
}

bool SmiDynamicLoader::is_loaded() const
{
	return handle_ != nullptr;
}

const char* SmiDynamicLoader::library_name() const
{
	return name_;
}
