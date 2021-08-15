#pragma once
#include <Windows.h>

template <typename T>
size_t stringLen(T* const string) {

	if (!string)
		return 0;

	for (size_t i = 0; true; ++i) {
		if (string[i] == '\0')
			return i;
	}
	
}

template <typename T>
size_t stringSizeBytes(T* const string) {
	return stringLen(string) * sizeof(T);
}

template <typename T>
bool compareString(T* const str1, T* const str2){
	bool status = false;

	auto sizeFirst = stringLen(str1);
	auto sizeSecond = stringLen(str2);

	if (sizeFirst == sizeSecond) {
		for (decltype(sizeFirst) i = 0; i < sizeFirst; ++i) {
			if (str1[i] != str2[i]) {
				return false;
			}
		}
		status = true;
	}

	return status;
}