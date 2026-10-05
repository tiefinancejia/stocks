// stdafx.cpp : 只包括标准包含文件的源文件
// HTSPerfTest.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

// TODO:  在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用

void split_string_by_char(const char* str, char c, vector<string> &arVec)
{
    vector<string>().swap(arVec);
    char *p = (char *)str;
    while (*p != '\0')
    {
        string s = "";
        while (*p != c && *p != '\0')
            s.push_back(*p++);
        if ("" == s)
            ++p;
        else
            arVec.push_back(s);
    }
    return;
}