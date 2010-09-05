/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CArchMiscWindows.h"
#include "CArchDaemonWindows.h"

#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED  ((DWORD)0x00000001)
#endif
#ifndef ES_DISPLAY_REQUIRED
#define ES_DISPLAY_REQUIRED ((DWORD)0x00000002)
#endif
#ifndef ES_CONTINUOUS
#define ES_CONTINUOUS       ((DWORD)0x80000000)
#endif
typedef DWORD EXECUTION_STATE;

//
// CArchMiscWindows
//

CArchMiscWindows::CDialogs* CArchMiscWindows::s_dialogs   = NULL;
DWORD						CArchMiscWindows::s_busyState = 0;
CArchMiscWindows::STES_t	CArchMiscWindows::s_stes      = NULL;

void
CArchMiscWindows::init()
{
	s_dialogs = new CDialogs;
	isWindows95Family();
}

bool
CArchMiscWindows::isWindows95Family()
{
	static bool init   = false;
	static bool result = false;

	if (!init) {
		OSVERSIONINFO version;
		version.dwOSVersionInfoSize = sizeof(version);
		if (GetVersionEx(&version) == 0) {
			// cannot determine OS;  assume windows 95 family
			result = true;
		}
		else {
			result = (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
		}
		init = true;
	}
	return result;
}

bool
CArchMiscWindows::isWindowsModern()
{
	static bool init   = false;
	static bool result = false;

	if (!init) {
		OSVERSIONINFO version;
		version.dwOSVersionInfoSize = sizeof(version);
		if (GetVersionEx(&version) == 0) {
			// cannot determine OS;  assume not modern
			result = false;
		}
		else {
			result = ((version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
						version.dwMajorVersion == 4 &&
						version.dwMinorVersion > 0) ||
						(version.dwPlatformId == VER_PLATFORM_WIN32_NT &&
						version.dwMajorVersion > 4));
		}
		init = true;
	}
	return result;
}

int
CArchMiscWindows::runDaemon(RunFunc runFunc)
{
	return CArchDaemonWindows::runDaemon(runFunc);
}

void
CArchMiscWindows::daemonRunning(bool running)
{
	CArchDaemonWindows::daemonRunning(running);
}

void
CArchMiscWindows::daemonFailed(int result)
{
	CArchDaemonWindows::daemonFailed(result);
}

UINT
CArchMiscWindows::getDaemonQuitMessage()
{
	return CArchDaemonWindows::getDaemonQuitMessage();
}

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* keyName)
{
	// ignore if parent is NULL
	if (key == NULL) {
		return NULL;
	}

	// open next key
	HKEY newKey;
	LONG result = RegOpenKeyEx(key, keyName, 0,
								KEY_WRITE | KEY_QUERY_VALUE, &newKey);
	if (result != ERROR_SUCCESS) {
		DWORD disp;
		result = RegCreateKeyEx(key, keyName, 0, TEXT(""),
								0, KEY_WRITE | KEY_QUERY_VALUE,
								NULL, &newKey, &disp);
	}
	if (result != ERROR_SUCCESS) {
		RegCloseKey(key);
		return NULL;
	}

	// switch to new key
	RegCloseKey(key);
	return newKey;
}

HKEY
CArchMiscWindows::openKey(HKEY key, const TCHAR* const* keyNames)
{
	for (size_t i = 0; key != NULL && keyNames[i] != NULL; ++i) {
		// open next key
		key = openKey(key, keyNames[i]);
	}
	return key;
}

void
CArchMiscWindows::closeKey(HKEY key)
{
	assert(key  != NULL);
	RegCloseKey(key);
}

void
CArchMiscWindows::deleteKey(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegDeleteKey(key, name);
}

void
CArchMiscWindows::deleteValue(HKEY key, const TCHAR* name)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegDeleteValue(key, name);
}

bool
CArchMiscWindows::hasValue(HKEY key, const TCHAR* name)
{
	DWORD type;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
	return (result == ERROR_SUCCESS &&
			(type == REG_DWORD || type == REG_SZ));
}

CArchMiscWindows::EValueType
CArchMiscWindows::typeOfValue(HKEY key, const TCHAR* name)
{
	DWORD type;
	LONG result = RegQueryValueEx(key, name, 0, &type, NULL, NULL);
	if (result != ERROR_SUCCESS) {
		return kNO_VALUE;
	}
	switch (type) {
	case REG_DWORD:
		return kUINT;

	case REG_SZ:
		return kSTRING;

	case REG_BINARY:
		return kBINARY;

	default:
		return kUNKNOWN;
	}
}

void
CArchMiscWindows::setValue(HKEY key,
				const TCHAR* name, const std::string& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_SZ,
								reinterpret_cast<const BYTE*>(value.c_str()),
								value.size() + 1);
}

void
CArchMiscWindows::setValue(HKEY key, const TCHAR* name, DWORD value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_DWORD,
								reinterpret_cast<CONST BYTE*>(&value),
								sizeof(DWORD));
}

void
CArchMiscWindows::setValueBinary(HKEY key,
				const TCHAR* name, const std::string& value)
{
	assert(key  != NULL);
	assert(name != NULL);
	RegSetValueEx(key, name, 0, REG_BINARY,
								reinterpret_cast<const BYTE*>(value.data()),
								value.size());
}

std::string
CArchMiscWindows::readBinaryOrString(HKEY key, const TCHAR* name, DWORD type)
{
	// get the size of the string
	DWORD actualType;
	DWORD size = 0;
	LONG result = RegQueryValueEx(key, name, 0, &actualType, NULL, &size);
	if (result != ERROR_SUCCESS || actualType != type) {
		return std::string();
	}

	// if zero size then return empty string
	if (size == 0) {
		return std::string();
	}

	// allocate space
	char* buffer = new char[size];

	// read it
	result = RegQueryValueEx(key, name, 0, &actualType,
								reinterpret_cast<BYTE*>(buffer), &size);
	if (result != ERROR_SUCCESS || actualType != type) {
		delete[] buffer;
		return std::string();
	}

	// clean up and return value
	if (type == REG_SZ && buffer[size - 1] == '\0') {
		// don't include terminating nul;  std::string will add one.
		--size;
	}
	std::string value(buffer, size);
	delete[] buffer;
	return value;
}

std::string
CArchMiscWindows::readValueString(HKEY key, const TCHAR* name)
{
	return readBinaryOrString(key, name, REG_SZ);
}

std::string
CArchMiscWindows::readValueBinary(HKEY key, const TCHAR* name)
{
	return readBinaryOrString(key, name, REG_BINARY);
}

DWORD
CArchMiscWindows::readValueInt(HKEY key, const TCHAR* name)
{
	DWORD type;
	DWORD value;
	DWORD size = sizeof(value);
	LONG result = RegQueryValueEx(key, name, 0, &type,
								reinterpret_cast<BYTE*>(&value), &size);
	if (result != ERROR_SUCCESS || type != REG_DWORD) {
		return 0;
	}
	return value;
}

void
CArchMiscWindows::addDialog(HWND hwnd)
{
	s_dialogs->insert(hwnd);
}

void
CArchMiscWindows::removeDialog(HWND hwnd)
{
	s_dialogs->erase(hwnd);
}

bool
CArchMiscWindows::processDialog(MSG* msg)
{
	for (CDialogs::const_iterator index = s_dialogs->begin();
							index != s_dialogs->end(); ++index) {
		if (IsDialogMessage(*index, msg)) {
			return true;
		}
	}
	return false;
}

void
CArchMiscWindows::addBusyState(DWORD busyModes)
{
	s_busyState |= busyModes;
	setThreadExecutionState(s_busyState);
}

void
CArchMiscWindows::removeBusyState(DWORD busyModes)
{
	s_busyState &= ~busyModes;
	setThreadExecutionState(s_busyState);
}

void
CArchMiscWindows::setThreadExecutionState(DWORD busyModes)
{
	// look up function dynamically so we work on older systems
	if (s_stes == NULL) {
		HINSTANCE kernel = LoadLibrary("kernel32.dll");
		if (kernel != NULL) {
			s_stes = reinterpret_cast<STES_t>(GetProcAddress(kernel,
							"SetThreadExecutionState"));
		}
		if (s_stes == NULL) {
			s_stes = &CArchMiscWindows::dummySetThreadExecutionState;
		}
	}

	// convert to STES form
	EXECUTION_STATE state = 0;
	if ((busyModes & kSYSTEM) != 0) {
		state |= ES_SYSTEM_REQUIRED;
	}
	if ((busyModes & kDISPLAY) != 0) {
		state |= ES_DISPLAY_REQUIRED;
	}
	if (state != 0) {
		state |= ES_CONTINUOUS;
	}

	// do it
	s_stes(state);
}

DWORD
CArchMiscWindows::dummySetThreadExecutionState(DWORD)
{
	// do nothing
	return 0;
}
