#pragma once

#include "CmDefinition.h"

class CmSetting
{
public:
	CmSetting(string name, string section);
	~CmSetting(void);

	static void Demo(CStr name = "set.ini");

public:
	void LoadSetting(string section);

	// Get a string indicating the whole setting data
	CStr Txt(void);

	// Get const char* value
	const char* Val(const char* key);

	// Get int setting value
	int operator [](const char* key);

	// Get double setting value
	double operator ()(const char* key);

	// Add new expression to the setting
	void Add(string expr);

	// Apply Expressions in the setting files
	void ApplyExpression(void);

	// Save setting files (The format is destroyed)
	void Save(const char* fileName = NULL);

	bool IsTrue(const char* key);

	bool LoadVector(const char* key, vecD &vd);
	bool LoadVector(const char* key, vecI &vi);
	bool LoadVector(const char* key, vecS &vs);

	string GetSettingFileName(){ return _fileName;}
private:
	map<string, string> _Setting;
	vecS _Keys;
	string _fileName;

	vecS Split(const char* key);
	bool Split(CStr str , string& key, string& value);
	string Calc(string expression);
	int Split(string expression, int a, int b, int& value);
	double Calc(CStr expression, int a, int b);
};

