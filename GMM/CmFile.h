#pragma once

#include "CmDefinition.h"

struct CmFile
{
	static string BrowseFile(bool isOpen = true);
	static string BrowseFolder(); 

	static inline string GetFolder(CStr& path);
	static inline string GetName(CStr& path);
	static inline string GetNameNE(CStr& path);

	// Get file names from a wildcard. Eg: GetNames("D:\\*.jpg", imgNames);
    static int GetNames(CStr &nameW, vecS &names, string dir = string());
    static int GetNamesNE(CStr& nameWC, vecS &names, string dir = string(), string ext = string());
	static inline string GetExtention(CStr name);

	static bool FileExist(CStr& filePath);
	static bool FilesExist(CStr& fileW);
	static bool FolderExist(CStr& strPath);

	static string GetWkDir();

	static BOOL MkDir(CStr&  path);

	// Eg: RenameImages("D:/DogImages/*.jpg", "F:/Images", "dog", ".jpg");
	static int Rename(CStr& srcNames, CStr& dstDir, const char* nameCommon, const char* nameExt);

	static inline void RmFile(CStr& fileW);
	static void RmFolder(CStr& dir);
	static void CleanFolder(CStr& dir, bool subFolder = false);

	static int GetSubFolders(CStr& folder, vecS& subFolders);

    inline static BOOL Copy(CStr &src, CStr &dst, BOOL failIfExist = false);
    inline static BOOL Move(CStr &src, CStr &dst);
	static BOOL Move2Dir(CStr &srcW, CStr dstDir);
    static BOOL Copy2Dir(CStr &srcW, CStr dstDir);

	//Load mask image and threshold thus noisy by compression can be removed
	static Mat LoadMask(CStr& fileName);

	static void WriteNullFile(CStr& fileName) {FILE *f = fopen(_S(fileName), "w"); fclose(f);}
};

/************************************************************************/
/* Implementation of inline functions                                   */
/************************************************************************/
string CmFile::GetFolder(CStr& path)
{
	return path.substr(0, path.find_last_of("\\/")+1);
}

string CmFile::GetName(CStr& path)
{
	int start = path.find_last_of("\\/")+1;
	int end = path.find_last_not_of(' ')+1;
	return path.substr(start, end - start);
}

string CmFile::GetNameNE(CStr& path)
{
	int start = path.find_last_of("\\/")+1;
	int end = path.find_last_of('.');
	if (end >= 0)
		return path.substr(start, end - start);
	else
		return path.substr(start,  path.find_last_not_of(' ')+1 - start);
}

string CmFile::GetExtention(CStr name)
{
	return name.substr(name.find_last_of('.'));
}

BOOL CmFile::Copy(CStr &src, CStr &dst, BOOL failIfExist)
{
    //return ::CopyFileA(src.c_str(), dst.c_str(), failIfExist);
    // TODO: fix me
    return false;
}

BOOL CmFile::Move(CStr &src, CStr &dst)
{
    //return MoveFileExA(src.c_str(), dst.c_str(), dwFlags);
    // TODO : fix me
    return false;
}

void CmFile::RmFile(CStr& fileW)
{ 
    // TODO: fix me
    /*
	vecS names;
	string dir;
	int fNum = CmFile::GetNames(fileW, names, dir);
	for (int i = 0; i < fNum; i++)
		::DeleteFileA(_S(dir + names[i]));
    */
}
