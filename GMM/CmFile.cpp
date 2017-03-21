//#include <shlobj.h>
//#include <Commdlg.h>
#include "CmFile.h"
#include "CmLog.h"
#include <exception>

BOOL CmFile::MkDir(CStr&  _path)
{
    throw "not implement";
    /*
	if(_path.size() == 0) {
		CmLog::LogWarning("Creating empty path\n");
		return false;
	}

	static char buffer[1024];
	strcpy(buffer, _S(_path));
	for (int i = 0; buffer[i] != 0; i ++) {
		if (buffer[i] == '\\' || buffer[i] == '/') {
			buffer[i] = '\0';
			CreateDirectoryA(buffer, 0);
			buffer[i] = '/';
		}
	}
	return CreateDirectoryA(_S(_path), 0);
    */
}

// Test whether a file exist
bool CmFile::FileExist(CStr& filePath)
{
    throw "not implement";
    /*
	if (filePath.size() == 0)
		return false;

	return  GetFileAttributesA(_S(filePath)) != INVALID_FILE_ATTRIBUTES; // ||  GetLastError() != ERROR_FILE_NOT_FOUND;
    */
}

bool CmFile::FilesExist(CStr& fileW)
{
	vecS names;
	int fNum = GetNames(fileW, names);
	return fNum > 0;
}

string CmFile::GetWkDir()
{	
    throw "not implement";
    /*
	string wd;
	wd.resize(1024);
	DWORD len = GetCurrentDirectoryA(1024, &wd[0]);
	wd.resize(len);
	return wd;
    */
}

string CmFile::BrowseFolder()   
{
    throw "not implement";
    /*
	static char Buffer[MAX_PATH];
	BROWSEINFOA bi;//Initial bi 	
	bi.hwndOwner = NULL; 
	bi.pidlRoot = NULL;
	bi.pszDisplayName = Buffer; // Dialog can't be shown if it's NULL
	bi.lpszTitle = "BrowseFolder";
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.iImage = NULL;


	LPITEMIDLIST pIDList = SHBrowseForFolderA(&bi); // Show dialog
	if(pIDList)	{	
		SHGetPathFromIDListA(pIDList, Buffer);
		if (Buffer[strlen(Buffer) - 1]  == '\\')
			Buffer[strlen(Buffer) - 1] = 0;

		return string(Buffer);
	}
	return string();   
    */
}

string CmFile::BrowseFile(bool isOpen /* = true */)
{
    throw "not implement";
    /*
	static char Buffer[MAX_PATH];
	OPENFILENAMEA   ofn;  
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Buffer;
	ofn.lpstrFile[0] = '\0';   
	ofn.nMaxFile = MAX_PATH;   
	ofn.lpstrFilter = "Images (*.bmp;*.jpg;*.png;*.gif)\0*.bmp;*.jpg;*.png;*.gif\0All (*.*)\0*.*\0\0";   
	ofn.nFilterIndex = 1;    
	ofn.Flags = OFN_PATHMUSTEXIST;   

	if (isOpen) {
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		GetOpenFileNameA(&ofn);
		return Buffer;
	}

	GetSaveFileNameA(&ofn);
	return string(Buffer);
    */
}

int CmFile::Rename(CStr& _srcNames, CStr& _dstDir, const char *nameCommon, const char *nameExt)
{
    throw "not implement";
    /*
	vecS names;
	string inDir;
	int fNum = GetNames(_srcNames, names, inDir);
	for (int i = 0; i < fNum; i++) {
		string dstName = format("%s\\%.4d%s.%s", _S(_dstDir), i, nameCommon, nameExt);
		string srcName = inDir + names[i];
		::CopyFileA(srcName.c_str(), dstName.c_str(), false);
	}
	return fNum;
    */
}

void CmFile::RmFolder(CStr& dir)
{
    throw "not implement";
    /*
	CleanFolder(dir);
	if (FolderExist(dir))
		CmComand::RunProgram("Cmd.exe", format("/c rmdir /s /q \"%s\"", _S(dir)), true, false);
      */
}

void CmFile::CleanFolder(CStr& dir, bool subFolder)
{
	vecS names;
	int fNum = CmFile::GetNames(dir + "/*.*", names);
	for (int i = 0; i < fNum; i++)
		RmFile(dir + "/" + names[i]);

	vecS subFolders;
	int subNum = GetSubFolders(dir, subFolders);
	if (subFolder)
		for (int i = 0; i < subNum; i++)
			CleanFolder(dir + "/" + subFolders[i], true);
}

bool CmFile::FolderExist(CStr& strPath)
{
    throw "not implement";
    /*
	int i = (int)strPath.size() - 1;
	for (; i >= 0 && (strPath[i] == '\\' || strPath[i] == '/'); i--)
		;
	string str = strPath.substr(0, i+1);

	WIN32_FIND_DATAA  wfd;
	HANDLE hFind = FindFirstFileA(_S(str), &wfd);
	bool rValue = (hFind != INVALID_HANDLE_VALUE) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);   
	FindClose(hFind);
	return rValue;
    */
}


int CmFile::GetSubFolders(CStr& folder, vecS& subFolders)
{
    throw "not implement";
    /*
	subFolders.clear();
	WIN32_FIND_DATAA fileFindData;
	string nameWC = folder + "\\*";
	HANDLE hFind = ::FindFirstFileA(nameWC.c_str(), &fileFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do {
		if (fileFindData.cFileName[0] == '.')
			continue; // filter the '..' and '.' in the path
		if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			subFolders.push_back(fileFindData.cFileName);
	} while (::FindNextFileA(hFind, &fileFindData));
	FindClose(hFind);
	return (int)subFolders.size();
    */
}

// Get image names from a wildcard. Eg: GetNames("D:\\*.jpg", imgNames);
int CmFile::GetNames(CStr &nameW, vecS &names, string dir)
{
    throw "not implement";
    /*
	dir = GetFolder(nameW);
	names.clear();
	WIN32_FIND_DATAA fileFindData;
	HANDLE hFind = ::FindFirstFileA(_S(nameW), &fileFindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do{
		if (fileFindData.cFileName[0] == '.')
			continue; // filter the '..' and '.' in the path
		if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue; // Ignore sub-folders
		names.push_back(fileFindData.cFileName);
	} while (::FindNextFileA(hFind, &fileFindData));
	FindClose(hFind);
	return (int)names.size();
    */
}

int CmFile::GetNamesNE(CStr& nameWC, vecS &names, string dir, string ext)
{
	int fNum = GetNames(nameWC, names, dir);
	ext = GetExtention(nameWC);
	for (int i = 0; i < fNum; i++)
		names[i] = GetNameNE(names[i]);
	return fNum;
}

// Load mask image and threshold thus noisy by compression can be removed
Mat CmFile::LoadMask(CStr& fileName)
{
	Mat mask = imread(fileName, CV_LOAD_IMAGE_GRAYSCALE);
	CV_Assert_(mask.data != NULL, ("Can't find mask image: %s", _S(fileName)));
	compare(mask, 128, mask, CV_CMP_GT);
	return mask;
}

BOOL CmFile::Move2Dir(CStr &srcW, CStr dstDir)
{
	vecS names;
	string inDir;
	int fNum = CmFile::GetNames(srcW, names, inDir);
	BOOL r = true;
	for (int i = 0; i < fNum; i++)	
		if (Move(inDir + names[i], dstDir + names[i]) == false)
			r = false;
	return r;
}

BOOL CmFile::Copy2Dir(CStr &srcW, CStr dstDir)
{
	vecS names;
	string inDir;
	int fNum = CmFile::GetNames(srcW, names, inDir);
	BOOL r = true;
	for (int i = 0; i < fNum; i++)	
		if (Copy(inDir + names[i], dstDir + names[i]) == false)
			r = false;
	return r;
}
