// sqlt-launcher.cpp : Defines the entry point for the application.
//
#include "stdafx.h"

#pragma comment(lib, "comctl32.lib") // For TaskDialog

#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include <Commctrl.h>
#include <string>
#include <vector>
#include "shlwapi.h"
#include "resource.h"
#include "tinyxml\tinyxml.h"

using namespace std;

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    //{
    //    HRESULT hr;
    //    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    //    int nClickedBtn;

    //    TASKDIALOG_BUTTON aCustomButtons[] = {
    //      { 1000, L"Heck &Yeah!" },
    //      { 1001, L"N&o Way Dude" },
    //      { 1002, L"Heck &Yeah!" },
    //      { 1003, L"N&o Way Dude" },
    //      { 1004, L"Heck &Yeah!" },
    //      { 1005, L"N&o Way Dude" },
    //      { 1006, L"Heck &Yeah!" },
    //      { 1007, L"N&o Way Dude" },
    //    };
 
    //      tdc.hwndParent = NULL;
    //      tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    //      tdc.dwFlags = TDF_USE_COMMAND_LINKS|TDF_USE_HICON_MAIN;
    //      tdc.pButtons = aCustomButtons;
    //      tdc.cButtons = _countof(aCustomButtons);
    //      tdc.pszWindowTitle = L"SQLTools Launcher";
    //      tdc.hMainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    //      tdc.pszMainInstruction = L"Select the configuration to execute:";
    //      tdc.pszFooter = L"Configurations are stored in sqlt-launcher.xml";
 
    //      hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);

    //      return 0;
    //}
    //WCHAR title[128];
    //size_t title_size = sizeof(title)/sizeof(title[0]);
    //mbstowcs_s(&title_size, title, "SQLTools Launcher", _TRUNCATE);

    //int choice = 0;
    ////TASKDIALOG_COMMON_BUTTON_FLAGS

    //HRESULT hResult = TaskDialog(
    //  NULL,                                 //_In_  HWND                           hWndParent,
    //  hInstance,                            //_In_  HINSTANCE                      hInstance,
    //  L"SQLTools Launcher",                 //_In_  PCWSTR                         pszWindowTitle,
    //  L"Main instructions",                 //_In_  PCWSTR                         pszMainInstruction,
    //  L"Content",                           //_In_  PCWSTR                         pszContent,
    //  TDCBF_OK_BUTTON|TDCBF_CANCEL_BUTTON,  //_In_  TASKDIALOG_COMMON_BUTTON_FLAGS dwCommonButtons,
    //  (PCWSTR)MAKEINTRESOURCE(IDI_MAIN),    //_In_  PCWSTR                         pszIcon,
    //  &choice                               //_Out_ int                            *pnButton
    //);

    const char* config = "sqlt-launcher.xml";

    bool found = false;
    char full_path[MAX_PATH];
    {
        char curr_dir[MAX_PATH];

        if (GetCurrentDirectory(sizeof(curr_dir)-1, curr_dir))
        {

            ::PathCombine(full_path, curr_dir, config);

            if (::PathFileExists(full_path))
                found = true;
        }
    }

    if (!found)
    {
        char program_folder[_MAX_PATH];
        if (::GetModuleFileName(hInstance, program_folder, _MAX_PATH))
        {
            ::PathRemoveFileSpec(program_folder);

            ::PathCombine(full_path, program_folder, config);

            if (::PathFileExists(full_path))
                found = true;
        }
    }

    if (!found)
    {
        MessageBox(NULL, "Config file not found!", "sql-launcher", MB_OK|MB_ICONERROR);
        return -1;
    }

    TiXmlDocument doc(full_path);

	if (!doc.LoadFile())
	{
        char message[1024];
		_snprintf_s(message, sizeof(message), "Could not config file. Error='%s'.", doc.ErrorDesc());
        MessageBox(NULL, message, "sql-launcher", MB_OK|MB_ICONERROR);
		return -1;
	}

	TiXmlNode* root = doc.FirstChild("programs");
    if (!root)
    {
        MessageBox(NULL, "Cannot find root node 'programs'", "sql-launcher", MB_OK|MB_ICONERROR);
		return -1;
    }

    TiXmlElement* rootE = root->ToElement();
    
    int default_idx = -1;
    const char* default_id = rootE->Attribute("default");
    bool detailed_labels = false;
    if (const char* detailed_labels_str = rootE->Attribute("detailed_labels"))
        detailed_labels = !strcmp(detailed_labels_str, "1") ? true : false;

	TiXmlNode* first = root->FirstChild("program");
    TiXmlElement* program = first ? first->ToElement() : 0;
    if (!first || !program)
    {
        MessageBox(NULL, "Cannot find node 'programs\\program'", "sql-launcher", MB_OK|MB_ICONERROR);
		return -1;
    }

    typedef basic_string<wchar_t> string_w;
    vector<string_w> program_list;
    for (int i = 0; program; ++i, program = program->NextSiblingElement("program"))
    {
        string buffer =  "Unnamed";

        if (const char* name = program->Attribute("name"))
            buffer = name;

        if (const char* id = program->Attribute("id"))
            if (default_idx == -1 && !strcmp(default_id, id))
                default_idx = i;

        if (detailed_labels)
        {
            if (const char* oracle_home = program->Attribute("oracle_home"))
            {
                buffer += "\nORACLE_HOME=";
                buffer += oracle_home;
            }

            if (const char* tns_admin = program->Attribute("tns_admin"))
            {
                buffer += "\nTNS_ADMIN=";
                buffer += tns_admin;
            }

            if (const char* oracle_bin_path = program->Attribute("oracle_bin_path"))
            {
                buffer += "\nORACLE_BIN_PATH=";
                buffer += oracle_bin_path;
            }
        }

        WCHAR buff[128];
        size_t buff_size = sizeof(buff)/sizeof(buff[0]);
        mbstowcs_s(&buff_size, buff, buffer.c_str(), _TRUNCATE);
        program_list.push_back(buff);
    }

    if (program_list.empty())
    {
        MessageBox(NULL, "Configuartion list is empty!", "sql-launcher", MB_OK|MB_ICONERROR);
	    return -1;
    }

    if (default_idx == -1)
    {
        typedef basic_string<wchar_t> string_w;
        auto_ptr<TASKDIALOG_BUTTON> buttons(new TASKDIALOG_BUTTON[program_list.size()]);

        vector<string_w>::const_iterator it = program_list.begin();
        for (int i = 0; it != program_list.end(); ++it, ++i)
        {
            buttons.get()[i].nButtonID = 1000 + i;
            buttons.get()[i].pszButtonText = it->c_str();
        }

        HRESULT hr;
        TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
        int clicked_btn = -1;

        tdc.hwndParent = NULL;
        //tdc.dwCommonButtons = TDCBF_CANCEL_BUTTON;
        tdc.dwFlags = TDF_USE_COMMAND_LINKS|TDF_USE_HICON_MAIN|TDF_ALLOW_DIALOG_CANCELLATION;
        tdc.pButtons = buttons.get();
        tdc.cButtons = program_list.size();
        tdc.pszWindowTitle = L"SQLTools Launcher";
        tdc.hMainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
        tdc.pszMainInstruction = L"Select the configuration to execute:";
        tdc.pszFooter = L"Configurations are stored in sqlt-launcher.xml";
 
        hr = TaskDialogIndirect(&tdc, &clicked_btn, NULL, NULL);

        if (clicked_btn >= 1000 && clicked_btn < 1000 + (int)program_list.size())
            default_idx = clicked_btn - 1000;
        else
            return 0;
    }
    
    program = first ? first->ToElement() : 0;
    for (int i = 0; program; ++i, program = program->NextSiblingElement("program"))
    {
        if (i == default_idx)
        {
            const char* name         = program->Attribute("name");
            const char* executable   = program->Attribute("executable");
            const char* oracle_bin   = program->Attribute("oracle_bin_path");
            const char* oracle_home  = program->Attribute("oracle_home");
            const char* tns_admin    = program->Attribute("tns_admin");

            if (!executable)
            {
                char message[1024];
		        _snprintf_s(message, sizeof(message), "programs\\program with name='%s' does not have the required 'path' attribute!", (name ? name : ""));
                MessageBox(NULL, message, "sql-launcher", MB_OK|MB_ICONERROR);
                return -1;
            }

            LPTCH env = 0;

            if (oracle_home || oracle_bin || tns_admin)
            {
                if (oracle_home)
                    SetEnvironmentVariable("ORACLE_HOME", oracle_home);
  
                if (oracle_bin)
                {
                    char org_path[32767]; 
                    
                    string new_path(oracle_bin);
                    new_path += ';';
                    if (GetEnvironmentVariable("PATH", org_path, sizeof(org_path))) 
                        new_path += org_path;

                    SetEnvironmentVariable("PATH", new_path.c_str());
                }

                if (tns_admin)
                    SetEnvironmentVariable("TNS_ADMIN", tns_admin);

                env = GetEnvironmentStrings();
            }

            STARTUPINFO startInfo;
            memset(&startInfo, 0, sizeof startInfo);

            PROCESS_INFORMATION procInfo;
            memset(&procInfo, 0, sizeof procInfo);

            char _executable[32768];
            strncpy_s(_executable, executable, sizeof(_executable));

            BOOL status = CreateProcess(
                0,                      //_In_opt_     LPCTSTR lpApplicationName,
                _executable,            //_Inout_opt_  LPTSTR lpCommandLine,
                0,                      //_In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                0,                      //_In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                FALSE,                  //_In_         BOOL bInheritHandles,
                0,                      //_In_         DWORD dwCreationFlags,
                env,                    //_In_opt_     LPVOID lpEnvironment,
                0,                      //_In_opt_     LPCTSTR lpCurrentDirectory,
                &startInfo,             //_In_         LPSTARTUPINFO lpStartupInfo,
                &procInfo               //_Out_        LPPROCESS_INFORMATION lpProcessInformation
            );
  
            if (!status)
            {
                MessageBox(NULL, "Failed to launch!", "sql-launcher", MB_OK|MB_ICONERROR);
                return -1;
            }

            return 1;
        }
    }

    MessageBox(NULL, "Failed to find the correct config entry!", "sql-launcher", MB_OK|MB_ICONERROR);
	return -1;
}
