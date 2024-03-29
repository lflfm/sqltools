 History
~~~~~~~~~

Version 2.0 build 14 (2021-03-03)
    Bug fixes:
      - the program crashes on opening a workspace after closing another one

Version 2.0 build 12 (2021-02-21)
    Improvements:
    - The program partially supports Unicode/UTF now. The editor works with European languages w/o any issues 
      but eastern characters are not supported mainly because the Windows monospaced fonts are actually not 
      fixed-width for those code pages. The data grid, on other hand, should handle any texts.
    - New docking window framework
      The control panels like "File Panel", "History", "Open Files", "Object Viewer" can be docked more easily, 
      as well as stacked in tabs. Additionally panels can "Auto hide" in the docking mode. The window tab control 
      workbook is more functional and nicer than in the version 1.9.
    - The external grep program was replaced with the internal implementation which is faster and more functional. 
    Bug fixes for earlier 2.0 version:
    - Autocompletion for table columns and procedure fails with the timeout error.
    - Tortoise Git menu does not appear in the file context menu 
    - Hanging on attempt to send email

Version 1.9 build 18 (2018-12-06)
    Bug fixes:
    - when a line is having only spaces, 'Trim Trailing Space / Ctrl + K, R" is not trimming any space.
      same issue for "Trim Leading Spaces / Ctrl+K, O"

Version 1.9 build 17 (2018-11-30)
    Improvements:
    -- adde two new commands to move between open files in the order of woorkbook tabs
        Window.NextWindow             Ctrl+PgDown
        Window.PrevWindow             Ctrl+PgUp
    - added Copy "Text" column to the Output context menu
    - added the toolbar button to enable/disable readonly session
    Bug fixes:
    - stop using hint RULE with unknown Oracle versions
    - The "Copy all" command formated "plain text" is missing a end-of-line in the last line.
      probably other copy commands have to be revised but I'll do that in the unicode version

Version 1.9 build 16 (2018-03-30)
    Bug fixes:
    - compatibility with WinXp
    - compatibility with non enterprise version of 8i

Version 1.9 build 15 (2018-02-25)
    Improvements:
    - Object Viewer "Query" and "Count" commands work with partitions and subpartitions 
    - Added help for Data grid and Object Viewer
    - Data grid "Save" command was renamed to "Export"
    Bug fixes:
    - Drage and drop with <Ctrl> from Object Viewer did not work as expected

Version 1.9 build 14 (2018-02-07)
    Bug fixes:
    - Broken individual refresh of constraints, triggers and views in Objects List (introduced in build 11)
    - Data grid copy does not handle NULLs properly (at least in case of  varchar2 and date columns)
    - Data grid copy fails after "Select All"

Version 1.9 build 12 (2018-02-04)
    Improvements:
    - added query excution / fetch time to the fetch message displayed on the status bar
    - implemented "Select All" command for the data grid
    Bug fixes:
    - left/right mouse click on unused space of the data grid should not change the selection
    - broken XML highlighting after "<!-- anything-->"

Version 1.9 build 11 (2018-01-01)
    Improvements:
    - DDL reverse engineering handles 12c indentity columns
    - Added "Generated", "Created" and "Modified" to the "Sequence" tab of "Objects List"
    - Dbms_Output supports unlimited size (enter 0), contributed by Tomas
    - RULE hint is used for databases prior to 10g only

Version 1.9 build 10 (2017-12-28)
    Bug fixes:
    - Deleted Templates reappears after program restart. Template saving procedure reimplemented 
      based on uuid in order to merge deleted and renames entries properly.
    - Auto-complete/Template window does not open on the left secondary monitor
    Improvements:
    - The program automatically reloads the template file if it was changed externally
    - Added a quicker way to create a new Template from the editor using the context menu.
      The select text will be used as Template body.
    - Added ability to modify the selected Template from Autocomplete/Template list 
      using the context menu
    - Added name uniqueness validation for new and modified Templates
    - Data grid gets INSERT copy/save format, XML format modified slightly
    - Data grid copy/save setting page split into two

Version 1.9 build 9 (2017-12-17)
    Improvements:
    - Find Match (Ctrl+]) works with CASE in SELECT/INSERT/UPDATE/DELETE statements
    - added support of SYSDBA and SYSOPER to the script CONNECT command
    - Save from the data grid gets a file name template
    Bug fixes:
    - table column autocomplete is not supposed to have datatype

Version 1.9 build 7 (2017-12-13)
    Improvements:
    - Added data grid popup viewer with two modes Text and Html.
      Html mode should display xml data as well with a condition
      Xml should begin with "<?xml..." declaration
    - Copy from the data grid in Html format has been improved.
      The clipboard gets extra information so Microsof Office products
      can recognize the data as Html.
    Bug fixes:
    - File context menu is supposed to show only TotoiseSVN extensions
      and File Properties but any items with bitmap are visible too.
    - Pressing keys <Shift> and <Ctrl> should not affect the scrolling position
      of the the data grid

Version 1.9 build 5 (2017-12-03)
    Improvements:
    - saving the order of workbook tabs in workspace
    - added support for 12c WITH FUNCTION/PROCEDURE queries
    - added option to ignore end-line commments after the final semicolon,
      for example: SELECT * FROM DUAL;--this comment does not cause an error anymore
    Bug fixes:
    - If you want to replace a string in a selected text with a replacement string bigger
      than the search string, it will not replace until the end of the original selected text.

Version 1.9 build 4 (2017-11-28)
    Improvements:
    - Added new data grid export formats to menus and settings
    - Data grid "Open in Editor" / "Save" commands work with rectangular selection
    - changed context menus of "Query" (data grid), "Plan" and "Output"
    Bug fixes:
    - indentation does not work with a single line columnar selection

Version 1.9 build 3 (2017-11-26)
    Improvements:
    - Implemented rectangular selection in the data grid,
      drag & drop in multiple formats (controlled with <Shift> and <Ctrl>)
    Bug fixes:
    - DDL reverse engineering incrorrectly handles function-base indexes
      with quoted columns
    - FATAL: Invoking Copy command (Ctrl+C) from "Properties" controlbar crashes the program
    - The program cannot connect to a database that is mount but not open.
    - added cleanup in case if non-essential config files (history.xml, favorites.xml) got corrupted

Version 1.8 build 38 (2016-09-16)
    Improvements:
    - Updated the program setup and created a new one that does not require admin
      rights. The new setup uses a user profile folder (instead of C:\Program Files)
      as a default destination for the program files.
    - Work started on help system.
    Bug fixes:
    - "Find Object" <F12> fails with EXCEPTION_ACCESS_VIOLATION
      when Object Viewer "Use Cache" is ON while session "Object Cache" is OFF

Version 1.8 build 37 (2016-09-01)
    Improvements:
    - Implemented "Rename / Move" command (internally it calls SaveAs
      and then deletes the original file)
    - the last choice of "Create Quick Snapshot" on exit is preserved now
    - added two options "Show only name in recent files menu" and
      "Show only name in recent workspace menu" (Editor / File Manger / History settings)
    - added two options "Show workspace name in the application title bar" and
      "Show workspace name in the application status bar" (Editor / Workspace settings)
    Bug fixes:
    - added "ON DELETE SET NULL" for FK reverse-engineering
    - remove an inexistent file from the recent menu is not working in some cases
    - File Save fails if file backup does not succeed. The backup failure can be
      ignored now.
    - Exit can ignore an modified workspace. "Close Workspace" is called before exit now.
    - Raw error handling on losing connection in the background keep alive procedure

Version 1.8 build 36 (2016-08-28)
    Improvements:
    - Added the option to retain the focus in the editor on "Find Object" F12
    - Added "Find Selected" [or current word] with the default shortcut Ctrl+F3 & Ctrl+Shift+F3
    Workspace-related improvements:
    - The active workspace name is included in the application title.
      The format is "CONNECTION - SQLTools 1.8bXX - {workspace} - [filename]"
    - Added dedicated "Workspace" settings page
    - Added option to delete Autosave snapshot on normal exit (see "Workspace" settings)
    - Implemented support for relative path for files stored in workspaces
      (there is an option in "Workspace" settings)
    - Workspaces and backup snapshots have been separated and use 2 different
      extensions.
    - Name templates for AS and QS snapshots have been unified besides the case
      when there is an active workspace the workspace name will be used as a prefix for
      Autosave snapshot.
    - Processing system shutdown in a proper way
    - The command "Save all" saves the active workspace as well
    - "Open Workspace" calls "Close Workspace" first
    - "Close Workspace" asks to save (cancel is possible) and then closes all documents
    - "Close All Files" calls "Close Workspace" if there is the active workspace
    Bug fixes:
     - Search / replace history is not preserved correctly if INI file is chosen
     to keep supplemental settings. Search history is stored in binary format now.
     - flickering on attempt to continue fetching data in the grid after disconnect/connect
     - "Go to active document location" not working on the first attempt
       if FilePanel was not open before
     - fixed skipped <> in tooltip "comments" of "Object Viewer"

Version 1.8 build 35 (2016-08-07)
    Improvements:
    - Simplified internal idle processing in order to reduce CPU usage
    - Added ability to turn on/off Syntax Gutter
    Bug fixes:
    - mismatch between toolbar images and commands introduced in the previous build
    - quick query/count could not be cancelled

Version 1.8 build 34 (2016-08-01)
    Improvements:
    - Added workspace commands to the main toolbar
    - Added shortcuts for workspace operations:
        Workspace.Copy                Ctrl+Shift+W,C
        Workspace.Paste               Ctrl+Shift+W,V
        Workspace.Save                Ctrl+Shift+W,S
        Workspace.SaveAs              Ctrl+Shift+W,A
        Workspace.SaveQuick           Ctrl+Shift+W,Q
        Workspace.Open                Ctrl+Shift+W,O
        Workspace.OpenQuick           Ctrl+Shift+W,I
        Workspace.OpenAutosaved       Ctrl+Shift+W,T
        Workspace.OpenClose           Ctrl+Shift+W,L
    - added quicksave workspaces - created by single click on toolbar.
      QS workspace disappears from menu after opening. Those files are stored
      with autosaved workspaces and their names are generated
      by "QS-YYYY-MM-DD-HH24-MI-#N" format mask.
    - changed autosave filename format to "AS-YYYY-MM-DD-HH24-MI-SS_pid###"
    - added setting to control size of recent files/workspaces menus
    - saving all changes in workspace instead of in individual files
    - detecting if file changed after opening modified version from workspace

Version 1.8 build 33 (2016-07-14)
    Improvements:
    - Reimplemented recent file list. It keeps hundreds of files
      with the last cursor position/selection/bookmarks.
    - Added "History" tab to "File Panel"

Version 1.8 build 32 (2016-07-13)
    Bug fixes:
    - autorestore happens for no reason under certain circumstances
    - "User requested cancel" error might completly block script execution
      after prolonged period from the program start (days)

Version 1.8 build 31 (2016-07-04)
    Bug fixes:
    - the cursor does not return to original position after succesuful script
      execution (w/o errors) if "Buffered Output" is on
    - a workspace file saved w/o a default extension if user changes the file name
      in SaveAs dialog
    - Undo for columnar block inserted beyond EOF leaves garbage on screen
      in some cases
    - CSV issue with multiline fields

Version 1.8 build 30 (2016-06-29)
    Improvements:
    - Introduced workspace and workspace autosaving. Workspace is edit session
      but called differently to avoid confusion with Oracle session.
      The unique feature is keeping layout (positions/selections/bookmarks/...)
      and text changes in the same file.
    - Added File format (Windows/Unix/Mac) to Text/Format menu
      it was buried deeply in settings

Version 1.8 build 29 (2016-06-16)
    Improvements:
    - When selecting a text with your mouse, double-click on the first word,
      hold down the mouse on the second click and then select your text.
      It will now select text by words, not characters.
      The same can be initiated by single-click with <Ctrl> pressed.
    - <Alt><Shift>+Arrow or mouse click starts Alternative Columnar selection mode,
      the difference from "Classic" Columnar selection is it automatically returns
      to "stream" selection as soon as the current edit operation is done
    - Highlighting for changed lines (yellow - changed, green - changed & saved,
      gold - changed, saved & reverted)
    - made Template dialog resizable
    Bug fixes:
    - added programmatically missing templates
    - downgraded user cancel error

Version 1.8 build 27 (2016-06-09)
    Improvements:
    - Pressing <Shift> while dropping an individual table/view column
      from "Object Viewer" inserts the column datatype as well
    - Currently unpopulated explain plan columns will be automatically reduced
      to the minimum width
    - Added the build number to the main window title
    - Added "Find Object"/"Query table"/"Count query" to the editor context menu
    Bug fixes:
    - Data grid, "Copy All" locks the clipboard if the cursor is still
      open but fetch is not possible because the connection is busy
    - Data grid, fixed "Remember manually changed column width"
    - Data grid, fixed some issues with multi-line text export in HTML and CVS
    - Editor: the cursor does not change position on right mouse click

Version 1.8 build 26 (2016-06-06)
    Improvements:
    - The currently executed script is marked with the exclamation sign <!> at the title
    Bug fixes:
    - Cancel does not stop a script with multiple fast sql statements
    - a new file saved w/o a default extension if user changes the file name in SaveAs dialog

Version 1.8 build 25 (2016-06-05)
    Improvements:
    - Completed migraing all database operatins into background thread.

Version 1.8 build 18 (2016-05-09)
    Bug fixes:
    - Count query (Ctrl+F11) fails from Object Viewer if table/view selected w/o columns
    - Object List does not show indexes for schemas different from the current one

Version 1.8 build 17 (2016-05-07)
    Improvements:
    - "Tables" page in "Objects List" gets 2 additional columns (rows and blocks).
    - "Indexes" and "Constraints" pages show the list of index/constraint columns.
    - "Indexes" and "Constraints" pages support "Show in Object Viewer" F12
    Bug fixes:

Version 1.8 build 16 (2016-05-02)
    Improvements:
    - Added two background activity indicators. The first indicator is global
      on the connection toolbar. The second one shows task activity initiated
      by "Objects List" on its status bar.
    - "Tables" page in "Objects List" gets updated with more relevant information.
    - operations on group of selected objects (compile, enable/disable) processed
      by independent background tasks in order to get responsive list item refresh
    Bug fixes:
    - right click on empty Object Viewer

Version 1.8 build 13 (2016-04-10)
    Improvement:
    - added dbms_application_info.set_client_info into background connection
    Bug fixes:
    - keep foreground/backgrount connection read-only attribute in sync

Version 1.8 build 12 (2015-11-08)
    Improvements:
    - Highlighting all occurrences of selected word
    - Displaying full path in the status bar when an open recent file menu item is selected
    - Changed the way the mouse position is mapped to text postion.
      The cursor position will always move to the left of the character selected by mouse,
      previously the closest boundary was choosen.
    Bug fixes:
    - incorrect highlighting in lines stated with 's'

Version 1.8 build 10 (2015-10-04)
    Improvements:
    - JAVA SOURCE added to "Object List"
    - JAVA SOURCE compilation errors are parsed to extract line and position

Version 1.8 build 9 (2015-09-29)
    Improvements:
    - can compile JAVA SOURCE

Version 1.8 build 5 (2015-09-27)
    Improvements:
    - all but user queries have been moved in background thread (second connection)
    Bug fixes:
    - QuickQuery (F11) / QuickQCount (Ctrl+F11) do not work with multiline selection

Version 1.8 build 5 (2015-09-16)
    Improvements:
    - execution of all "Object View" commands moved in background thread (second connection)
    - implemented "Keep (connection) alive period" with default 10 min
    - added new editor command "Duplicate current line or selection" with default shortcut Ctrl+K,Q

Version 1.8 build 3 (2015-09-10)
    Improvements:
    - execution of all "Objects List" commands moved in background thread (second connection)

Version 1.7 build 53 (2015-07-26)
    Bug fixes:
    - Save File: added FlushViewOfFile & FlushFileBuffers to force Windows to flush data to disk
    - QuickQuery (F11) ignores "NULL representation" setting in Data Grid 2

Version 1.7 build 52 (2015-01-27)
    Other changes:
    - removed the trailing <CR> from copy in tab-delimited mode.
      Hopefully nobody will complain about that.

Version 1.7 build 51 (2015-01-25)
    Bug fixes:
    - bug fix, explain plan view small navigation/focus issues

Version 1.7 build 50 (2015-01-21)
    Bug fixes:
    - bug fix, ObjectViewer compatibility issue with 10g

Version 1.7 build 49 (2014-10-15)
    Improvements:
    - new explain plan view
    - property view for explain plan view

Version 1.7 build 45 (2014-10-02)
    Improvements:
    - added a sql text to the error reporting
    Bug fixes:
    - bug fix, DDL reverse engineering compatibility issue with 11.1

Version 1.7 build 42 (2014-08-05)
    Bug fixes:
    - bug fix, "Compile" fails on "Types"/"Type Bodies" from "ObjetsList"

Version 1.7 build 41 (2014-06-30)
    Improvements:
    - added support for dbms_xplan. Taken from sqltools++ but it was altered.
    - added two properties to the Grid setting: "Headers in lowercase"
      and "Wrap around cursor" (sqltools++)
    - "Count" from ObjectViewer is possible with selected columns as well:
      select <selected columns>, count(*) from <table> group by <selected columns> order by count(*) desc

Version 1.7 build 39 (2014-06-23)
    Improvements:
    - added partitions/subpartitions to ObjectViewer (subject to change)
    Bug fixes:
    - bug fix, when Double-Click to close file with changes, file seems closed, but filename remain in workbook

Version 1.7 build 37 (2014-06-19)
    Improvements:
    - added "Quick query" and "Quick count(*)"
    - added "Query"/"Count" to ObjectViewer (it can show only selected columns)
    - The result of "Query"/"Count" from ObjectViewer / ObjetsList can be shown either
      in a new document or in already open one (change in settings)
    - F11 key was re-assigned to run "Query"/"Count",
      that is not final unless there is strong voice against that
    Not impleneted yet:
    - add an optional where clause to "query"/"count(*)"
    - add an alternative "SQL" context menu and let switch default context menu

Version 1.7 build 35 (2014-06-15)
    Improvements:
    - DDL reverse engineering handles sub-partitioning
    Bug fixes:
    - PROMPT with ' is not handled properly
    - DDL reverse engineering is not compatible with 8i

Version 1.7 build 27 (2014-05-29)
    Improvements:
    - DDL reverse engineering recognizes virtual columns and range-interval partitioning
    Bug fixes:
    - Keyboard shortcuts in "Objects Viewer" have been re-implementd because if their erratic behavior.
    - DB object highlighting is still active after disconnect.

Version 1.7 build 26 (2014-05-27)
    Bug fixes:
    - compile in "Invalid Objects" does not refresh object status

Version 1.7 build 25 (2014-05-26)
    Improvements:
    - Added "Invalid Objects" Tab to "Objects List"
    - Added duration to SQL history
      ATTENTION: all persistent history files will be converted to the new format
      and will be ignored by SQLTools 1.6!
    Bug fixes:
    - All Alt key + (0, 1, 3 & 4) do not work whenever Object Viewer is in focus.
      I want to see how this fix works. if it does not cause any problem,
      I will make similar changes in "Objects List" and "File Manger".
    - Object Viewer does not work when 'Use Cache' option is selected with slow network option.

Version 1.7 build 24 (2014-05-19)
    Improvements:
    Bug fixes:
    -Autocomplete for package procedures does not work

Version 1.7 build 23 (2014-05-18)
    Improvements:
    - ObjectViewer is finalized,
      hopefully I will need only to add new stuff as partition/subpartition info
    - Addded "Load DDL with Dbms_Metadata", the shortcut is Alt+F12
    Bug fixes:

Version 1.7 build 21 (2014-05-12)
    Improvements:
    - New ObjectViewer look, tree lines have been sacrificed
      in favor of colunm formatted datatypes.
    Bug fixes:
    - cannot load index DDL form ObjectViewr

Version 1.7 build 20 (2014-05-12)
    Improvements:
    - Removed RULE hint from DDL reverse engineering for server versions >= 10g
    Bug fixes:

Version 1.7 build 19 (2014-05-11)
    Improvements:
    - Autocomplete (objects, table column / package procedures)
    Bug fixes:
    - template changes are not available for already open files
    - read-only connection does not allow explain plan
    - commit command is not disabled in read-only mode
    - read-only connection asks about commit/rollback on close

Version 1.7 build 16 (2014-04-27)
    Improvements:
    - Implemented REFCURSOR and DATE bind variables.
      The last one is not supported by Oracle SQL*PLus,
    Bug fixes:

Version 1.7 build 15 (2014-04-20)
    Improvements:
    - ObjectViewer has been re-implemented (drag-drop has been improved)
    Bug fixes:
    - suppressed unnecessary object name cache reload (for editor highlighting)
      on any setting change anymore.
    - Win7 compatibility issue with multi item selection in ObjectViewer

Version 1.6 build 23 (2014-03-03)
    Improvements:
    - added otion to disable query cancle confirmation
    Bug fixes:
    - revisited the issue related to drag and drop in columnar mode

Version 1.6 build 22 (2014-03-03)
    Bug fixes:
    - multiline drag and drop in columnar mode does nothing

Version 1.6 build 21 (2014-02-09)
    Bug fixes:
    - Window shell commands removed from the editor contect menu,
      alternatively they are available on <Shift> + right click.

Version 1.6 build 20 (2014-02-04)
    Improvements:
    - "File Panel/Open Files" can be a drag & drop source
      so any file can be dragged and dropped on another application
      such as SQLTools.
    - File Shell menu items added to the editor context menu.
    Bug fixes:
        - selection stays on cursor move by Ctrl+[
        - paste on multi-line columnar selection uses the cursor position
          instead of the beginning of selection

Version 1.6 build 19 (2013-10-31)
    Improvements:
        - Object Viewer gets "Do not use RULE hint" option

Version 1.6 build 18 (2013-10-XX)
    Improvements:
        - Implemented:
            Edit.RemoveBlankLines         Ctrl+K,G
            Edit.RemoveExcessiveBlankLines Ctrl+K,H
            Edit.TrimLeadingSpaces        Ctrl+K,O
            Edit.TrimExcessiveSpaces      Ctrl+K,P
            Edit.TrimTrailingSpaces       Ctrl+K,R
    Bug fixes:
        - ORA-28001 (the password has expired) is hadled on connect

Version 1.6 build 17 (2013-10-01)
    Improvements:
    - command line supports user/password@host:port/service format
      addiionally to user/password@tnsname and user/password@host:port:sid
    - Commit/Rollback called from menu logged in "Output" pane
    - Cancel requires a confirmation

    Bug fixes:
    - substitution fails sometimes on text pasted from clipboard
    - "invalid QuickArray<T> subscript" exception on "Execute Current" F10
      if the cursor is beyond of the End Of single-line File

Version 1.6 build 16 (2011-12-25)
    Improvements:
    - included the last changes from OpenEditor project
        - Added a set of operations on a stream selection:
            Edit.JoinLines                Ctrl+K,J
            Edit.SplitLine                Ctrl+K,K
        - Added a set of operations on a stream columnar selection:
            Edit.ColumnInsert             Ctrl+K,B
            Edit.ColumnInsertNumber       Ctrl+K,M
            Edit.ColumnLeftJustify        Ctrl+K,Y
            Edit.ColumnCenterJustify      Ctrl+K,V
            Edit.ColumnRightJustify       Ctrl+K,W
        - Under development:
            Edit.RemoveBlankLines         Ctrl+K,G
            Edit.RemoveExcessiveBlankLines Ctrl+K,H
            Edit.TrimLeadingSpaces        Ctrl+K,O
            Edit.TrimExcessiveSpaces      Ctrl+K,P
            Edit.TrimTrailingSpaces       Ctrl+K,R

    - compatibility with VC++ 2010 (SQLTools.sln-2010.sln/SQLTools.vcxproj)

Version 1.6 build 15 (2011-12-25)
    Bug fixes:
    - Find Brace (Ctrl+[) and brace highlighting cause an exception under some conditions
    - DDL reengineering and Object Viewer show zero column size for RAW type
    - Fixed some typos in messages

Version 1.6 build 14 (2011-11-27)
    Bug fixes:
    - brace highlighting is wrong if the context is horizontally scrolled.
    - define/undefine can be erroneously executed while "Execute Current"
      is searching for the beginning of the current statement.
      The substitution variables can be changed as the result.

Version 1.6 build 12 (2011-11-20)
    Improvements:
    - Added highlighting for matching brace, two color properties "Highlighting"
      and "Error Highlighting". Taken from sqltools++ but it was altered.
    - Two environment variables ORACLE_HOME and TNS_ADMIN were added to "About Dialog"
      and bug reporting
    - Table and view columns can be sorted alphabetically. "Object List" gets its own
      property page in Settings.
    Bug fixes:
    - current schema synonyms are not highlighted when current_schema <> user account

Version 1.6 build 11 (2011-11-15)
    Improvements:
    - "Object Viewer" is able to show
        package/type functions and procedures,
        parameters for stand alone functions and procedures,
        "references" tables for tables (the old "referenced tables" node was renamed
        to "referenced by")
    - "Object Viewer" behavior was changed slightly. If a found object is a package or type
      then its specification is automatically displayed (the old implementation wants user
      to select one of them).
    - Highlighting for user objects. Objects are selected by the current schema
      rather then user. Quoted identifiers are not supported.
      "Refresh object cache" command was added under "Session" menu.
    - "Safety" combo box was replaced with "Read Only" checkbox in Connection dialog.
      "Read Only" toggled item was added under "Session" menu.
    - "Slow network" checkbox was added in Connection dialog.
      if this option is checked then "Object List" and "Object cache" are not refreshed
      automatically on connect.
    - The program can connect to a database that is mount but not open.
      The same should apply to ASM instance. The feature was taken from SQLTools++.
    - Text selection by mouse should be smoother on older computers
    Bug fixes:
    - "Object Viewer" is incompatible with Oracle 8.0.X (broken in 1.5 build 21/22)
    - DDL reengineering returns wrong column size for CHAR types in AL32UTF8 database
    - DDL reengineering prints Zero for INITIAL, MINEXTENTS, MAXEXTENTS in 11g database
    - Connection dialog opens too slow if the number of stored connection is above a hundred
     (the problem was introduced in 1.6)

Version 1.6 build 6 (2011-11-03)
    Improvements:
    - "File Manager" has basic integration with Windows context menu
       that allows access to TortoiseSVN / CVS commands using the application menu
    - "File Manager" allows to execute/rename/delete files and folders
    - "File Manager" file type filter is finally implemented
    - "File Manager" shows file/folder information in tooltips
    - "File Manager" settings page has been added to control new features

Version 1.6 build 4 (2011-10-20)
    Improvements:
    - "Execute Current" is smarter now, you do not need to place the cursor
      on the first line of a statement before execution. It finds the beginning
      of the the statement based script syntax rules (statements have to be
      separated by ';' and '/' for PL/SQL stuff). If you like old TOAD style execution
      then it is supported too (check "Execute Current Alt"). The new settings page
      allows change default behavior for these 2 commands.
    - Bulk execution commands "Execute" and "Execute from "Cursor position" have
      "Halt Execution On Error" option. That works like in SQLTools++ but it first shows
      an error and then asks if you want to cancel or continue script execution.
    - Additionally single statement execution commands can highlight and automatically
      show the beginning and end of the current statement.
    - the editor does not have 64K line limitation, now it can handle 2GB lines in theory.
    - "File Panel" restores the last selected tab when application restarts
    - improved stability of XML settings storage and recovery encrypted connection
      information.
    Bug fixes:
    - file custom.keymap is missing
    - IOT table has extra PK definition if "No storage for PK,UK" is checked
    - Find Object F12 does not recognize an object name if it is sql*plus keyword
    - Load DDL Script Ctrl+F12 throws the exception "Invalid object type"
      on a synonym that is not longer valid

Version 1.5 build 22 (2011-10-03)
    improvement:
    - Added an option to use INI file for supplemental settings instead of the registry
    bug fixes:
    DDL reengineering fails on source lines longer than 128 chars (inroduced in build 21)

Version 1.5 build 21 (2011-10-02)
    Improvements:
    - execute in SQL*Plus taken from sqltools++ with minor changes
    - added "Test and Reconnect" from sqltools++
    - DDL reengineering supports CHAR/BYTE column length symatntecs
    bug fixes:
    - FindReplace dialog can go beyond the display screen and stay there
    - DDL reengineering fails on altered Oracle OBJECTS/TYPES
    - "Object List" can show dropped objects after reapplying new filter conditions

Version 1.5 build 20 (2011-09-28)

    Improvements:
    - refresh on synonyms tab "Object List (Alt+3)" has been speeded up for 10g and above
    - added JOIN, INNER, OUTER to sql keywords (remove old language.xml)
    - added "Open Settings Folder" in "Tools" menu
    - a small fix for CASE expression in PLS/SQL language support
    - "Find Object" <F12> and "Load DDL script" <Ctrl>+<F12> do substitution
      before searching for an object
    - improved cursor position / selection indicator has been borrowed from sqltools++
    - commit/rollback dialog on exit allows cancel exit
    - block indent keeps a selection anchored to a text
    - DDL reengineering supports compound triggers for 11g
    - DDL reengineering supports constraints on views
    - DDL reengineering supports NOVALIDATE and RELY for constraints
    bug fixes:
    - deleting selection extended beyond EOF causes EXCEPTION_ACCESS_VIOLATION
    - the application hangs if user sets either Indent or Tab size to 0
    - /**/ comments after END of a procedure cause a compilation error
    - DDL reengineering fails on wrapped pl/sql procedures
      because all_source.text might contain multiple code lines
    - block undent does not work correctly if Tab size > Indent size
    - editor gutter might not show syntax nesting correctly during incremental parsing
    - PL/SQL Analyzer fails on auth/wrapped packages
    - PL/SQL Analyzer fails on packages with startup/shuldtown/run procedures
      (those are script keywords)
    - PL/SQl Analyzer fails on grant/revoke
    - some bug fixes taken from sqltools++ by Randolf Geist
    - DDL reengineering fails on IOT tables w/o overflow

Version 1.5 build 14 (2011-09-01)

    SQLTools has been released under GNU GPL v3 (updated from v2 to v3).

    "Find Object" <F12> and "Load DDL script" <Ctrl>+<F12> use an updated object name resolver
    that can handle ALTER SESSION SET CURREN_SCHEMA = <Schema>

    Improvements in DDL reverse engineering:
    - implemented PARALLEL clause for tables, indexes and clusters
    - implemented event triggers

    Improvements in "Object List (Alt+3)":
    - Schema list control got new addition to the context menu.
    It allows quick switch between previously selected schemas (session live scope)
    - Changed a default action for the table list. <DblClick> will load DDL
    instead of switching to "Object Viewer". <Alt>+<DblClick> will show the
    current selection in "Object Viewer"
    - <Del> accelerator is not linked with "Drop" action anymore for safety reason.
    You should use <Ctrl>+<Del> as a short cut for "Drop"

    A minor bug fix for Drag&Drop from Grid component

Version 1.5 build 10 (2009-05-06)
    bug fix - execution of a part line selection does not support bind variables
    bug fix - dbms_output line limit increased from 255 to 4000 chars
    bug fix - "SQL Quick Reference guide" search/index tabs miss some keywords (sqlqkref.chm)
    some improvements taken from sqltools++ by Randolf Geist (2007 version)
    some small fixes in "Find Match" Ctrl+]

Version 1.5 build 9 (2007-10-1)
    Improvements (some taken from sqltools++ by Randolf Geist):
    - redesigned PL/SQL syntax analyzer ("Find Match" Ctrl+] works better)
    - redesigned Connect Dialog, connection information is stored encrypted in XML file
    - the program settings are stored as XML in %APPDATA%/GNU/SQLTools (private)
      or <Program Folder>/SharedData (shared) depending on user choice
    - implemented Filter for "Object List"
    - improved keyboard navigation in "Object List"
    - added Recyclebin to "Object List"
    - improved error handling for "Extract Schema DDL"
    - implemented DDL reverse engineering for partitioned tables and indexes
    Some minor bugs have been fixed.

Version 1.4.1 build 66 (2005-04-18)

    R#1111224 - OE Enable/disable "smart" home key behavior (tMk).
    R#1092510 - OE Drag/Reorder Tab (tMk).
    R#1084220 - OE Close Tab on double click (tMk).
    R#1081930 - OE Open file under cursor (tMk).
    R#unknown - OE New Netscape-like property sheet interface & new tabs (tMk).
    R#unknown - OE Highlighting for a dragged workbook tab (tMk).
    R#unknown - OE Introduced "IndentGuide" (ak).
    R#unknown - OE Added ColourPicket for Font/Color property page (ak).
    R#unknown - OE Added auto-scrolling for dragged workbook tabs (ak).

    B#1185035 - OE "Show message on EOF" doesn't work (ak).
    B#1181324 - Trigger reverse-engineering fails if  description contains comments (ak).
    B#1165795 - OE Hanging during replace in the selection (ak).
    B#1093790 - OE Show message on EOF (tMk).
    B#1086407 - OE Infinite locking directory (tMk).
    B#unknown - OE Wrong message if file or folder does not exist (ak).
    B#unknown - OE Missing shortcut labels for double key (ak).
    B#unknown - OE If file are locked, an unrecoverable error occurs on "Reload" (ak).
    B#unknown - OE Sorting does not completely remove duplicate rows if a number of dups is even (ak).


Version 1.4.1 build 64 (2005-03-21)

    R#unknown - Bind variables (SQL*Plus syntax for VARCHAR2, NUMBER, CLOB) (clubok).

    B#1159674 - Hanging with 'invalid string position' (tMk).
    B#1123762 - Hanging when ;; at the end of statement (ak).
    B#unknown - OE An exception on "Find Object", if some name component is null (ak).


Version 1.4.1 build 63 (2005-01-10)

    R#unknown - SQL*Plus CONNECT/DISCONNECT commands
    R#unknown - Added support for BINARY_FLOAT and BINARY_DOUBLE.
    R#unknown - TIMESTAMP, INTERVAL support.
    R#unknown - Grid format for number columns.
    R#unknown - XMLTYPE support only for Oracle client 10g, but it's slow as hell.

    B#1095462 - Empty bug report on disconnect (ak).
    B#1092516 - Copy column from datagrid (clubok).
    B#1087088 - TIMESTAMP object in Oracle 9i is not recognised (ak).
    B#1086239 - CSV export (clubok).
    B#1078884 - XML Export (clubok).
    B#unknown - Substitution "&var." does not work.
    B#unknown - Cannot execute "exec null" w/o ending ';'.
    B#unknown - Compatibility with Oracle client 8.0.X.


Versions before SF.net (up to 1.4.1 build 58-9)

1.4.1 build 58-9
    Improvements:
    - An input field for Find/Describe object window has been added
    Following bugs have been fixed:
    - '/' might be recognized as a statement separator at any position

1.4.1 build 58-8
    The latest OpenEditor code has been included
    Following bugs have been fixed:
    - all compilation errors are ignored after "alter session set current schema"
    - all compilation errors are ignored for Oracle7
    - some servers may return very long strings due to oracle bug
    - external authentication does not work
    - trigger reverse-engineering fails in OF clause if column name contains "ON_"/"_ON"/"_ON_"

1.4.1 build 58-3
    bug fix, compatibility with Oracle client 8.0.X

1.4.1 build 58-2
    bug fix, sqltools crashes if a connection was broken

1.4.1 build 58
    Improvements:
    - Action on disconnect: rollback, commit or user choice
    Following bugs have been fixed:
    - Memory corruption on a query with blob columns.
    It's a very serious bug, which has been introduced since 1.4.1 (I guess).

1.4.1 build 57
    Improvements:
    - sort in editor
    Following bugs have been fixed:
    - Autocommit does not work
    - unknown exception on "save as", if a user doesn't have permissions
        to rewrite the destination
    - exception "FixedString is too long (>=64K)" on undo after save

1.4.1 build 53
    Improvements:
    - Bug-report memory consumption has been decreased
    Following bugs have been fixed:
    - DDL reengineering: domain index bug (introduced in 141b46)
    - CreareAs file property is ignored for "Open In Editor", "Query", etc (always in Unix format)
    - Output: only the first line of multiline error/message is visible (introduced in 141b50)

1.4.1 build 52
    Improvements:
    - global sql history is done
    - history files are merged if concurrent sqltools instances save history for same files
    - setting and history can be moved to another location.
        You can do it only manually, see example:
        REGEDIT4
        [HKEY_CURRENT_USER\Software\KochWare\SQL Tools\Roots]
        "History"="e:\\_TEST\\SQLTools.settings\\sql-history"
        "Settings"="e:\\_TEST\\SQLTools.settings\\data"

    Following bugs have been fixed:
    - export DDL fails if an object name contains on of \/:*?"<>|
    - command line parser fails on sysdba/sysoper
    - sql history memory limit

1.4.1 build 50
    Small improvements:
    - global sql history (shared between all sql worksheet)
    - displaying current record# and number of records in the status bar

1.4.1 build 49
    Bug fix release:
    - exception on scrolling if grid contains more then 32K rows
    - temporary tables reengineering fails on 8.1.X (since build 47)
    - editor: exception on PgDn if "Cursor beyond end of file" is unchecked
    - a connection info might not be displayed properly after changing an existing connection profile
    Small improvements:
    - a connection info in the main window title (pls let me know if it looks ugly)
    - a server version in the connection toolbar and in the status bar

1.4.1 build 48
    Bug fix release:
    - "Object List"/"Object Tree" small issues with cancellation of ddl loading
    - DDL reengineering: trigger/views group operation bug (introduced in build 47)

1.4.1 build 47
    Bug fix release:
    - DDL reengineering: 64K limit for trigger increased to 512K
    - DDL reengineering: 64K limit for view increased to 512K
    - DDL reengineering: Oracle Server 8.0.5 support (tables, indexes)
    - suppressed bug-reporting for oracle errors on grid fetching
    - some fixes in general exception handling

1.4.1 build 46
    Bug fix release:
    - some fixes in diagnostics and bug-reporting, partial support for Win98/WinME
    - "Object List" error handling changed to avoid paranoiac bug-reporting
    - Lock/open/save file error handling changed to avoid paranoiac bug-reporting

1.4.1 build 45
    Improvements:
    - Diagnostics and bug-reporting
    Following bugs have been fixed:
    - Ctrl+End causes unknown exception in data grid if query result is empty
    - Explain plan result may contain duplicated records if you press F9 too fast
    - Explain plan table is not cleared after fetching result
    - CreareAs property is ignored for ddl scripts (always in Unix format)

1.4.1 build 44
    Improvements:
    - SQL History persistence (it should be enabled in History settings)
    - PERL support (language.dat and settings.dat has to be replaced for activation)
    Following bugs have been fixed:
    - Table transformer helper generates unique name only once
    - Substitution error and Cancel window problem
    - When using Ctrl +F12 (Load DDL Script) on a table name,
        only index script is loaded, not table creation script.
        It happens only when index name is identical to table name.
    - handling unregular eol, e.c. single '\n' for MSDOS files

1.4.1 build 41
    - Following bugs have been fixed:
    - shortcut F1 has been disabled until sqltools help can be worked out
    - missing public keyword for public synonyms
    - Unexpected exception: "OCI8::BlobVar::GetString(): String buffer overflow!"

1.4.1 build 40
    Following bugs have been fixed:
    - continuous exception "Invalid vector<t> subscript" on query which returns no records,
        it happens only if grid inplace editor is activated

1.4.1 build 39
    Following bugs have been fixed:
    -"Invalid vector<t> subscript" happened again, some diagnostics
        improvement and protection against infinite exception loop
    - missing new line characters for procedure/function/package code

1.4.1 build 38
    Improvements:
    - type and uniqueness have been added to index description in "Object Viewer"
    Following bugs have been fixed:
    - wrong order of html tags for html export
    - "Invalid vector<T> subscript" happens continually
        if a query returns error during fetching the first portion of records
    - file extension is not recognized properly if it's shorter then 3 chars
    - quote character in comments for table/view DDL
    - Cancel window does not work properly on Load DDL
    - missing public keyword for public database links

1.4.1 build 37
    Improvements:
    - a new option for cancel query window - it can be topmost
    Following bugs have been fixed:
    - the serious bug since the previous build, a procedure/function/package code line can be truncated if its length > 128
    - multiple occurences of PUBLIC in "Schema" list ("Object List" window)
    - any sql statement which is executed after cancelation will be also canceled
    - multiple typos in menus, hints and dialogs
    - workaround for oracle 8.1.6, removed trailing '0' for long columns and trigger text
    - html export does not handle empty strings (with spaces) properly
    - export settings do not affect on quick html/csv viewer launch

1.4.1 build 36
    Improvements:
    - open grid data with default html and csv viewers (see gred toolbar)
    Following bugs have been fixed:
    - goto the last record does not fetch all rows
    - clob fields have extra 0 character

1.4.1 build 35
    It's the first Beta build for 1.4.1 release.
    Major improvements since 1.4 release:
    - OCI8 instead of OCI7 and as result some related features
        which were not be available for OCI7 (for example connect as sysdba)
    - SQL Window redesign and SQL History pane

1.4.1 build 34
    8.0.X compatibility problem resolved

1.4.1 build 33
    A lot of fixes and small improvements

1.4.1 build 32
    Improvements:
    - Cancel query is available after reimplementation
    Several bugs have been fixed

1.4.1 build 31
    Improvements:
    - sql history

1.4.1 build 29
    Following bugs have been fixed:
    - no error offset for sql statemnet
    - oracle types support

1.4.1 build 21
    Improvements:
    - safety is a new conection property (none, confirmation required (not implemented yet), read-only)
    - connection control bar
    - connection string in application title while application is minimized

1.4.1 build 11
    Improvements:
    - output/error postions is based on internal line IDs and more stable on editing text above the original line
    - thousand delemiters for cost,cardinality and bytes in Explain Plan viewer

1.4.1 build 3
    Improvements:
    - Added two new connect options SYSDBA and SYSOPER.

1.4 build 28
    Improvements:
    - SQR language support has been improved (match for begin-setup/end-setup, ...)
    - C++ language support has been improved (match for #if/#else/#endif)

1.4     Full list is not available yet. Some latest changes:

    23.03.2003 improvement, save files dialog appears when the program is closing
    23.03.2003 improvement, added a new command - mdi last window - default shortcut is Ctrl+Tab
    23.03.2003 improvement, added a new editor option - keep selection after drag and drop operation
    23.03.2003 improvement, added a new mouse word selectiton on left butten click with pressed ctrl
    24.03.2003 improvement, MouseSelectionDelimiters has been added (hidden) which influences on double click selection behavior
    26.03.2003 improvement, added delay for scrolling in drag & drop mode
    30.03.2003 optimization, removed images from Object Browser for Win95/Win98/WinMe because of resource cost
    30.03.2003 improvement, added command line support, try sqltools /h to get help
    30.03.2003 improvement, added SQR language support (no match for control constructions yet)

1.3.5.19 Improvements:
    - Plan table option
    - Dbms Output size option
    Following bugs have been fixed:
    - wrong declaration length of character type column for fixed charset
    - wrong aligmnet for long type column in data grid

1.3.5.18 Improvements:
    "Object List" supports new commands:
    - Query (it's not good enough yet)
    - Delete
    - Truncate

1.3.5.17 Following bugs have been fixed:
    - bug fix, plsql parser loses all empty lines
        (any created stored procedure/trigger/view does not have empty lines)
    - bug fix, a last column with right alignment looks ugly
        if the colomn is expanded to the right window boundary
    - numeric column alignment has been changed to right in statistics grid

1.3.5.16 Following bugs have been fixed:
    - comment processing before define/undefine and after sql statements
    - wrong error positioning on EXEC (raised in pl/sql procedure)
    - wrong error positioning on execution of selected text
        if selection does not starts at start of line
    - grid painting defect on scrolling to the right
    Improvements:
    - Data grid supports
        - a new format for saving - XML (element based)
        - auto resizing and remembering column sizes
        - new copy commands and drag & drop modes
    NOTE: If you upgrade SQLTools you should increase "Max Column Length"
        on "Settings"/"Grig" Page to get the benefit of new auto column resizing.


1.3.5.15 Following bugs have been fixed:
    - Data grid does not use persistent settings until grid property page is called.
    - Spin control behavior in editor/grid property pages.
    - Cluster reengineering fails
    - Better SQLPlus compatibility
    Improvements:
    - String representation for NULL value and Date format for data grid
        look Grid page in Settings to change it

1.3.5.14 Following bugs have been fixed:
    - debug privilege for 9i
    - small fixes in data grid export

1.3.5.13 Major changes:
    - Data grid supports a new format for saving - comma delimited (CSV)
        and a new command "Save and Open with a Default Editor"
        (for CSV files it can be MS Excel)

1.3.5.12 Following serios bug has been fixed:
    - SQLTools does not reload compilation errors
        on Execute/Compile command if an invalid package/procedure
        has full qualified name (schema name + object name).

1.3.5.11 Following serios bug has been fixed:
    - Table DDL reengineering fails on integer columns.
        Integer column is replaced with number one.
        The same bug still exists in "Quick viewer"
        because of some limitations in implementation.

1.3.5.10 Following bugs have been fixed:
    - SQLTools hangs on a context-sensitive help call
    if there is more than one topic, which is found by keyword

1.3.5.9 Following bugs have been fixed:
    - OLEACC.DLL is missing for Win95/Win98/NT4
    - OCIEnvCreate is not found in OCI.DLL for Net 8.0.X
    - Statistics is always empty because sesstat.dat is missing
    - minor fixes

1.3.5.8 Following bugs have been fixed:
    - "Load DDL script" fails on table with trigger
    which body is bigger then 4K
    - Command "Find Object" fails on an TYPE object.
    There is an open issue. Type & package bodies nodes don't have
    "Dependent On" & "Dependent Objects" folders. It's not possible
    in the current implementation.

1.3.5.7 Following bugs have been fixed:
    - empty prompt(s) and rem(s) fail on execute
    Improvements:
    - Variable Substitution (& &&) became global persistent option
        and uses a shared (between multiple windows) substitution map.
    - Skipping unsupported sqlplus commands (set ..., show ...)
    - HTML SQL Quick Reference has been updated

1.3.5.6 Following bugs have been fixed:
    - the program crashes on connect the first time after the first installation
    - DDL reengineering loses PK & UK storage when it's invoked from PK & UK pages
    Improvements:
    - HTML SQL Quick Reference has been added (English version)

1.3.5.5 Following bugs have been fixed:
    - the program crashes after F6 or F7 (bug since 1.3.0.10)
    Improvements:
    - the first step to sqltools help - connection dialog help

1.3.5.4 Following bugs have been fixed:
    - End line comment bug (';' was not ignored)
    - "define variable=value" does not work,
        space was required "define variable = value"

1.3.5.3 Bug fixes (connection dialog, "Load DDL script" command).

1.3.5.1 Major changes:
    - Compatibility with Oracle 7.3.x and 8.5.x has been dropped because
    it's impossible to test it and it's significant overhead for coding.
    - "Object List" window includes 4 new tabs (indexes and constraints).
    - DDL reengineering supports index-based tables and reverse indexes.
    - Connection dialog has been re implemented for supporting non-tnsnames
    connection strings.

1.3.0.10 Following minor bugs have been fixed:
    - A MDI window splitter flickers after restore or maximize operations
    - Activation of MDI window fails if a mouse moves out of the current tab
    - Some name resolving problems in "Find object" command
    Following functionality have been added:
    - "Load DDL script" resolves an object name and loads DDL scripts.
    Shortcut keys are Ctrl+F12 and Shift+Ctrl+F12.

1.3.0.9  Following bugs have been fixed:
    - OCI7 initialization bug for 8.1.7 client
    - A package body text is corrupted if a package body header has
    more then one space between "package" and "body".

1.3.0.8  Sqlplus-like EXECUTE command has been added. DEFINE supports sqlplus syntax.
    Comments processing bug has been fixed (';' was not skipped in comments).

1.3.0.7  Stable version. Fixed a small bugs in trigger source reengineering.
    Fixed a bug in function-based index source reengineering. Fixed
    crash in some case when a editor window is closed after connection
    losing. Changed focus processing in "Object list" window.

1.3.0.6  Editor bookmark failure has been fixed. Load options for "Load as one"
    have been fixed. MS Wheel support has been  added.
