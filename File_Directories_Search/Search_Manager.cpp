#define _CRT_SECURE_NO_WARNINGS
#include "Search_Manager.h"
#include <locale>
#include <codecvt>
#include <tuple>

Search_Manager* Search_Manager::ptr = nullptr;

template<class T>
void Search_Manager::FindFile(T)
{
	try
	{
		T it{ dir }, end;
		while (it != end)
		{
			SetValue(file_info(directory_entry(it->path())));
			it++;
			if (!bCheckThreadStatus)
			{
				UnblockButton();
				return;
			}
		}
		UnblockButton();
	}
	catch (exception ex)
	{
		MessageBox(hMain, ex.what(), TEXT("Error!"), MB_OK | MB_ICONINFORMATION);
	}
}


tuple<string, string, string, string> Search_Manager::file_info(directory_entry entry)
{
	string Date_mod;
	if (exists(entry.path()))
	{
		auto ftime = last_write_time(entry.path());
		std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
		Date_mod = ::asctime(std::localtime(&cftime));
	}
	const auto fs(status(entry));

	return tuple{	entry.path().string().substr(entry.path().string().find_last_of("/\\") + 1),
					entry.path().string(),
					to_string(is_regular_file(fs) ? file_size(entry.path()) : 0) + string(" bytes"),
					Date_mod
	};
}

Search_Manager::Search_Manager(void) noexcept : hMaskEdit(nullptr), hTextEdit(nullptr), hDiskCombo(nullptr), hListView(nullptr), hCount(nullptr)
{
	ptr = this;

	bCheckThreadStatus = bCheckBoxStatus = { false };

	memset(&LvCol, 0, sizeof(LvCol));
	memset(&LvItem, 0, sizeof(LvItem));

	LvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	LvItem.mask = LVIF_TEXT;
	LvItem.cchTextMax = 256;
}

Search_Manager::~Search_Manager() noexcept
{
	ThreadClose();
	file.close();
}

void Search_Manager::OnInitDialog(HWND h)
{
	try
	{
		hMain = h;
		hListView = GetDlgItem(h, IDC_LIST2);
		hMaskEdit = GetDlgItem(h, IDC_EDIT1);
		hTextEdit = GetDlgItem(h, IDC_EDIT2);
		hDiskCombo = GetDlgItem(h, IDC_COMBO1);
		hCheckBox = GetDlgItem(h, IDC_CHECK1);
		hCount = GetDlgItem(h, IDC_STATIC1);
		hSearch = GetDlgItem(h, IDC_STATIC2);
		hStart = GetDlgItem(hMain, IDC_BUTTON1);
		hStop = GetDlgItem(hMain, IDC_BUTTON2);

		FindDisks();
		SendMessage(hDiskCombo, CB_SETCURSEL, 0, 0L);

		AddColumn("Date modified", 145);
		AddColumn("Size", 100);
		AddColumn("Full name", 225);
		AddColumn("Name", 125);

		SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

		SetDir();
	}
	catch (exception ex)
	{
		MessageBox(h, ex.what(), TEXT("Error!"), MB_OK | MB_ICONINFORMATION);
	}
}

void Search_Manager::OnCommand(WPARAM wp, LPARAM lp)
{
	switch (LOWORD(wp))
	{
	case IDC_CHECK1:
		bCheckBoxStatus = (BOOL)SendMessage(hCheckBox, BM_GETCHECK, 0, 0);
		break;
	case IDC_BUTTON1:
		ThreadClose();
		UnblockButton();
		//CountClear = LvItem.iItem;
		ClearViewTable();
		bCheckThreadStatus = TRUE;
		EditMask();
		ChooseIterator();
		break;
	case IDC_BUTTON2:
		bCheckThreadStatus = FALSE;
		if (!IsWindowEnabled(hStop))
			UnblockButton();
		break;
	case IDC_COMBO1:
		if (HIWORD(wp) == CBN_SELCHANGE)
			SetDir();
		break;
	default:
		break;
	}
}

void Search_Manager::AddColumn(const char* Text, int size = 200) noexcept
{
	LvCol.cx = size;
	LvCol.pszText = (LPSTR)Text;
	SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);
}

void Search_Manager::FindDisks()
{
	auto Disks(GetLogicalDrives());
	for (char i[2]{ 'A' }; i[0] < 'Z'; i[0]++, Disks >>= 1)
		if (Disks & 1)
			SetDisks(i);
}

void Search_Manager::SetDisks(char* Disk) noexcept
{
	SendMessage(hDiskCombo, CB_ADDSTRING, 0, LPARAM((string(Disk) + TEXT(":\\")).c_str()));
}

void Search_Manager::SetValue(tuple<string, string, string, string> tValue)
{
	auto [Name, Full_Name, Size, Date_Modification] = tValue;
	vector<const char*> Value{ Name.c_str() , Full_Name.c_str(), Size.c_str(), Date_Modification.c_str() };
	try
	{
		SearchStatus(Full_Name.c_str());
		if (CheckResult(Name.c_str()) && CheckFileResult(Full_Name.c_str(), atoi(Size.c_str())))
		{
			SendMessage(hListView, LVM_INSERTITEM, LvItem.iSubItem = 0, (LPARAM)&LvItem);
			for (; LvItem.iSubItem < (int)Value.size(); LvItem.iSubItem++)
			{
				LvItem.pszText = (LPSTR)Value[LvItem.iSubItem];
				SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&LvItem);
			}
			CountStatus(++LvItem.iItem);
		}
	}
	catch (exception ex)
	{
		MessageBox(hMain, ex.what(), "Error!", MB_OK | MB_ICONINFORMATION);
	}
}

void Search_Manager::SetDir()
{
	char pathname[MAX_PATH];
	GetWindowText(hDiskCombo, pathname, MAX_PATH);
	dir = pathname;
}

void Search_Manager::ThreadClose() noexcept
{
	if (Result.joinable())
		Result.join();
}

void Search_Manager::ChooseIterator()
{
	if (!bCheckBoxStatus)
		Result = thread([this]() { FindFile(directory_iterator()); });
	else
		Result = thread([this]() { FindFile(recursive_directory_iterator()); });
}

void Search_Manager::EditMask()
{
	char pathname[MAX_PATH];
	GetWindowText(hMaskEdit, pathname, MAX_PATH);
	if (exists(pathname))
	{
		dir = pathname;
		SearchText = "";
	}
	else
	{
		SetDir();
		string Temp{};
		SearchText = pathname;

		vector<string> SearchSymbols;

		for (size_t i = 0; i < SearchText.length(); i++)
			SearchSymbols.push_back(string(1, SearchText[i]));

		SearchText = "";

		for (size_t i = 0; i < SearchSymbols.size(); i++)
			if (regex_search((SearchSymbols[i]), regex("\\\\|\\+|\\[|\\]|\\/|\\^|\\$|\\.|\\||\\(|\\)|\\{|\\}")))
				SearchText += "\\" + SearchSymbols[i];
			else if (SearchSymbols[i] == "?")
				SearchText += "+";
			else if (SearchSymbols[i] == "*")
				SearchText += ".*";
			else
				SearchText += SearchSymbols[i];
	}
	//MessageBox(hMain, Temp.c_str(), TEXT("Result!"), MB_OK | MB_ICONINFORMATION);
}

void Search_Manager::UnblockButton()
{
	if (IsWindowEnabled(hStart))
	{
		EnableWindow(hStart, FALSE);
		EnableWindow(hStop, TRUE);
	}
	else
	{
		EnableWindow(hStart, TRUE);
		EnableWindow(hStop, FALSE);
	}
}

void Search_Manager::ClearViewTable()
{
	for (int i = 0; i < LvItem.iItem; i++)
		ListView_DeleteItem(hListView, i);
	ListView_DeleteAllItems(hListView);// ~_(*.*)_~
	CountStatus(LvItem.iItem = 0);
}

void Search_Manager::CountStatus(int count)
{
	SetWindowText(hCount, string("Результаты поиска: " + to_string(count)).c_str());
}

void Search_Manager::SearchStatus(const char* filename)
{
	SetWindowText(hSearch, string("|\t" + string(filename)).c_str());
}

bool Search_Manager::CheckFileResult(const char* Check, int size)
{
	char pathname[MAX_PATH];
	GetWindowText(hTextEdit, pathname, MAX_PATH);
	if (!regex_search(SearchText, regex(".txt")) || string(pathname) == "")
		return true;
	if (size < 200)
	{
		SearchFile = "";
		file.open(Check);
		char c;
		while (!file.eof())
		{
			file.get(c);
			SearchFile.push_back(c);
		}
		file.close();
		//MessageBox(hMain, SearchFile.c_str(), TEXT("Error!"), MB_OK | MB_ICONINFORMATION);

		if (regex_search(SearchFile, regex(".*" + string(pathname) + ".*")))
			return true;
	}
	return false;
}

bool Search_Manager::CheckResult(const char* TextCheck) const noexcept
{
	return regex_match(string(TextCheck), regex(".*" + SearchText + ".*"));
}

BOOL CALLBACK Search_Manager::DlgProc(HWND hWnd, UINT mes, WPARAM wp, LPARAM lp)
{
	switch (mes)
	{
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		break;
	case WM_INITDIALOG:
		ptr->OnInitDialog(hWnd);
		break;
	case WM_COMMAND:
		ptr->OnCommand(wp, lp);
		break;
	}
	return 0;
}