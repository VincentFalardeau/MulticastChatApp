#pragma once
#define DLL_ALGO_ROTATION __declspec(dllexport)
extern "C" DLL_ALGO_ROTATION const int ALPHA_COUNT = 26;
extern "C" DLL_ALGO_ROTATION void chiffrer(char* buffer, int decalage);
extern "C" DLL_ALGO_ROTATION void dechiffrer(char* buffer, int decalage);
