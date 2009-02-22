// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>


extern "C"
{
void __declspec(dllexport) Test();
}

int _tmain(int argc, _TCHAR* argv[])
{

	Test();
	char c;
	std::cin >> c;
	return 0;
}

