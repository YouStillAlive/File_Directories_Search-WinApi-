#pragma once
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "resource.h"
#include <windows.h>
#include <thread>
#include <Tchar.h>
//#include <filesystem>// bugs with access)
#include <experimental/filesystem>// no bugs!
#include <sstream> 
#include <regex>
#include <commctrl.h>
#include <exception>
#include <fstream>

using namespace std;
//using namespace filesystem;
using namespace experimental::filesystem;

class Search_Manager
{
	///
	thread Result;
	///
	path dir;
	ifstream file;
	string SearchText;
	string SearchFile;

	HWND hMaskEdit;
	HWND hTextEdit;
	HWND hDiskCombo;
	HWND hMain;
	HWND hCount;
	HWND hSearch;

	HWND hListView;
	LVCOLUMN LvCol;
	LVITEM LvItem;

	HWND hCheckBox;
	BOOL bCheckBoxStatus;

	HWND hStart;
	HWND hStop;
	BOOL bCheckThreadStatus;

	void OnInitDialog(HWND h);
	void OnCommand(WPARAM wp, LPARAM lp);
	void AddColumn(const char* Text, int size) noexcept;
	void FindDisks();
	void SetDisks(char* Disk) noexcept;
	void SetValue(tuple<string, string, string, string> Value);
	void SetDir();
	void ThreadClose() noexcept;
	void ChooseIterator();
	void EditMask();
	void UnblockButton();
	void ClearViewTable();
	void CountStatus(int count);
	void SearchStatus(const char* filename);
	bool CheckFileResult(const char *Check, int size);
	bool CheckResult(const char* Check) const noexcept;
	template<class T>
	void FindFile(T);
	tuple<string, string, string, string> file_info(directory_entry entry);
	static Search_Manager* ptr;
public:
	Search_Manager() noexcept;
	~Search_Manager() noexcept;
	static BOOL CALLBACK DlgProc(HWND hWnd, UINT mes, WPARAM wp, LPARAM lp);
};