#include "CmSetting.h"
#include <cstdarg>
#include <fstream>
#include "CmLog.h"
#include <cstdio>

#define BUFFER_SIZE 1024
char gTmpStrBuf[BUFFER_SIZE];
const char* CmSprintf(const char* format, ...)
{
	va_list cur_arg;
	va_start(cur_arg, format); {
		vsnprintf(gTmpStrBuf, BUFFER_SIZE, format, cur_arg);
	}
	va_end(cur_arg);
	return gTmpStrBuf;
}


CmSetting::CmSetting(string name, string section)
{
	_fileName = name;
	LoadSetting(section);
}

CmSetting::~CmSetting(void)
{
}

// Splite a string to key and value
bool CmSetting::Split(CStr str, string& key, string& value)
{
	const char* comment = "#\n\r";
	const char* space = " \t";
	const char* equ = "=:";
	int cc = (int)str.find_first_of(comment);
	int a = (int)str.find_first_not_of(space);
	int b = (int)str.find_first_of(equ);
	int c = (int)str.find_last_not_of(space, cc - 1);
	if (a < b && b <= c && cc != 0) {
		int b1 = (int)str.find_last_not_of(space, b - 1);
		int b2 = (int)str.find_first_not_of(space, b + 1);
		key = str.substr(a, b1 - a + 1);
		value = b == c ? "" : str.substr(b2, c - b2 + 1);
		return true;
	}
	return false;
}

// Calculate expressions
string CmSetting::Calc(string expression)
{
	string out;
	for (size_t i = 0; i < expression.length(); i ++)	{
		switch (expression[i])	{
		case '\\':
			i ++;
			out += (i < expression.length()) ? expression[i] : '\\';
			break;
		case '$':{
				size_t j = i + 1;
				while (j < expression.length() && expression[j] != '$') j ++;
				if (j < expression.length() && j > i + 1)	{
					string key = expression.substr(i + 1, j - i - 1);
					if (_Setting.find(key) != _Setting.end())
						out += _Setting[key];
					i = j;
					break;
				}
			}
		default:
			out += expression[i];
		}
	}
	return out;
}

// Split expressions
int CmSetting::Split(string expression, int a, int b, int& value)
{
	int r = - 1, d = 0;
	value = INT_MAX - 1;
	for (int i = a; i <= b; i ++) {
		int p = INT_MAX;
		switch (expression[i])	{
		case '(': d += 3; break;
		case ')': d -= 3; break;
		case '+':
		case '-': p = d + 1; break;
		case '*':
		case '/': p = d + 2; break;
		}
		if (p <= value)	{
			value = p;
			r = i;
		}
	}
	return r;
}

// Calculate numerical expressions
double CmSetting::Calc(CStr expression, int a, int b)
{
	int value;
	int c = Split(expression, a, b, value);
	if (c == - 1) {
		a = (int)expression.find_first_not_of("(", a);
		b = (int)expression.find_last_not_of(")", b);
		string key = expression.substr(a, b - a + 1);
		if (_Setting.find(key) != _Setting.end()) key = _Setting[key];
		return atof(key.c_str());
	}
	a += (value / 3);
	b -= (value / 3);
	double x = Calc(expression, a, c - 1);
	double y = Calc(expression, c + 1, b);
	double r = 0;
	switch (expression[c])	{
	case '+': r = x + y; break;
	case '-': r = x - y; break;
	case '*': r = x * y; break;
	case '/': if (y != 0) r = x / y; break;
	}
	return r;
}

// Load setting with section name and file name
void CmSetting::LoadSetting(string section)
{
	_Setting.clear();
	_Keys.clear();

	ifstream inp(_fileName.c_str());
	if (!inp.is_open())
		CmLog::LogError("Can't load file: \"%s\" in %d: %s\n", _fileName.c_str(), __LINE__, __FILE__);

	string line, key, value;
	bool inside = true;
	while (getline(inp, line))	{
		int a = (int)line.find_first_not_of(" \t");
		if ((a >= 0) && (line[a] == '[')) {
			int b = line.find_last_not_of(" \t");
			inside = (line.substr(a, b - a + 1) == "[" + section + "]");
			continue;
		}

		if (inside && Split(line, key, value)) {
			_Keys.push_back(key);
			_Setting[key] = value;
			if (key == "section") section = value;
		}
	}
	inp.close();
	_Setting["section"] = section;
	ApplyExpression();
}


// Apply Expressions in the setting files
void CmSetting::ApplyExpression(void)
{
	for (vecS::iterator it = _Keys.begin(); it != _Keys.end(); it ++) {
		string key = *it, mapvalue = _Setting[key];
		if (key[0] == '$')	{
			key = key.substr(1);
			if (_Setting.find(key) == _Setting.end()){
				string value = Calc(mapvalue);
				_Setting[key] = value;
			}
		}
		if (key[0] == '@')	{
			key = key.substr(1);
			if (_Setting.find(key) == _Setting.end()){
				string expression;
				for (size_t i = 0; i < mapvalue.length(); i ++){
					if (mapvalue[i] != ' ' )
						expression += mapvalue[i];
				}
				double value = Calc(expression, 0, (int)expression.length() - 1);
				ostringstream oss;
				oss << value;
				_Setting[key] = oss.str();
			}
		}
	}
}

// Add new expression to the setting
void CmSetting::Add(string expr)
{
	string key, value;
	if (Split(expr, key, value)) {
		_Keys.push_back(key);
		_Setting[key] = value;
		ApplyExpression();
	}
}

// Get a string indicating the whole setting data
CStr CmSetting::Txt(void)
{
	ostringstream o;
	for (map<string, string>::const_iterator it = _Setting.begin(); it != _Setting.end(); it ++)
		o << it->first << " = " << it->second << endl;
	return o.str();
}

// Get const char* value
const char* CmSetting::Val(const char* key)
{
	map<string, string>::const_iterator it = _Setting.find(key);
	if (it != _Setting.end())
		return it->second.c_str();
	CmLog::LogWarning("Invalidate parameter with key = \"%s\" in setting file \"%s\".\n", key, _S(_fileName));
	return NULL;
}

// Get int setting value
int CmSetting::operator [](const char* key)
{
	const char* str = Val(key);
	if (str == NULL){
		fprintf(stderr, "No validate setting parameter with key = %s\n", key);
		return -1;
	} 
	else
		return atoi(Val(key));
}

// Get double setting value
double CmSetting::operator ()(const char* key)
{
	const char* str = Val(key);
	if (str == NULL){
		fprintf(stderr, "No validate setting parameter with key = %s\n", key);
		return -1;
	} 
	else
		return atof(Val(key));
}

bool CmSetting::LoadVector(const char* key, vecS &vs)
{
	string keyS(key);
	int num = (*this)[CmSprintf("%sNum", keyS.c_str())];
	vs.resize(num);
	for (int i = 0; i < num; i++)
		vs[i] = Val(CmSprintf("%s%d", keyS.c_str(), i));
	return num > 0;
}

bool CmSetting::LoadVector(const char *key, vecD &vd)
{
	string keyS(key);
	int num = (*this)[CmSprintf("%sNum", keyS.c_str())];
	vd.resize(num);
	for (int i = 0; i < num; i++)
		vd[i] = (*this)(CmSprintf("%s%d", keyS.c_str(), i));
	return num > 0;
}

bool CmSetting::LoadVector(const char *key, vecI &vi)
{
	string keyS(key);
	int num = (*this)[CmSprintf("%sNum", keyS.c_str())];
	vi.resize(num);
	for (int i = 0; i < num; i++)
		vi[i] = (*this)[CmSprintf("%s%d", keyS.c_str(), i)];
	return num > 0;
}

// Split a string to several ones
vecS CmSetting::Split(const char* key)
{
	istringstream iss(Val(key));
	string value;
	vecS ret;
	while (iss >> value) ret.push_back(value);
	return ret;
}

// Save setting files (The format is destroyed)
void CmSetting::Save(const char* fileName /* = NULL */)
{
	ofstream* pOut;
	if (fileName == NULL)
		pOut = new ofstream(_Setting["iniFileName"].c_str());
	else
		pOut = new ofstream(fileName);
	if (pOut->is_open()) {
		ofstream& out = *pOut;
		out << Txt() <<endl;
		out.close();
	}
}

// Demonstrate how to use this class
void CmSetting::Demo(CStr name /* = "set.ini" */)
{
	CmSetting varSet(name, "default");

	int x0 = varSet["x0"];
	double x1 = varSet("x1");
	double x3 = varSet("x 3");

	printf("x0 = %d\n", x0);
	printf("x1 = %lf\n", x1);
	printf("x3 = %lf\n", x3);


	printf("Setting string:****************************************\n");
	printf("%s\n", varSet.Txt().c_str());
	printf("Setting string:****************************************\n");
	printf("Input any thing, type exit to quit\n");

	char input[100];
    while(scanf("%s", input, 100) == 1 && strcmp(input, "exit"))	{
		printf("%s = %s\n", input, varSet.Val(input));
		printf("%s = %d\n", input, varSet[input]);
		printf("%s = %lf\n", input, varSet(input));
	}

	varSet.Add("$new = $direct$new");
	varSet.Save("set.set.ini");
}

bool CmSetting::IsTrue(const char* key)
{
	return (Val(key) != NULL) && (strcmp(Val(key), "true") == 0);
}
