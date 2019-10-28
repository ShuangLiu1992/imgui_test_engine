// dear imgui
// (test engine, test context = end user automation api)

#pragma once

#include "imgui.h"
#include "imgui_te_core.h"

// Undo some of the damage done by <windows.h>
#ifdef Yield
#undef Yield
#endif

//-------------------------------------------------------------------------
// ImGuiTestContext
// This is the interface that most tests will interact with.
//-------------------------------------------------------------------------

// Note: keep in sync with GetActionName()
enum ImGuiTestAction
{
    ImGuiTestAction_Unknown = 0,
    ImGuiTestAction_Click,
    ImGuiTestAction_DoubleClick,
    ImGuiTestAction_Check,
    ImGuiTestAction_Uncheck,
    ImGuiTestAction_Open,
    ImGuiTestAction_Close,
    ImGuiTestAction_Input,
    ImGuiTestAction_NavActivate,
    ImGuiTestAction_COUNT
};

inline const char* GetActionName(ImGuiTestAction action)
{
    switch (action)
    {
    case ImGuiTestAction_Unknown:       return "Unknown";
    case ImGuiTestAction_Click:         return "Click";
    case ImGuiTestAction_DoubleClick:   return "DoubleClick";
    case ImGuiTestAction_Check:         return "Check";
    case ImGuiTestAction_Uncheck:       return "Uncheck";
    case ImGuiTestAction_Open:          return "Open";
    case ImGuiTestAction_Close:         return "Close";
    case ImGuiTestAction_Input:         return "Input";
    case ImGuiTestAction_NavActivate:   return "NavActivate";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

inline const char*  GetActionVerb(ImGuiTestAction action)
{
    switch (action)
    {
    case ImGuiTestAction_Unknown:       return "Unknown";
    case ImGuiTestAction_Click:         return "Clicked";
    case ImGuiTestAction_DoubleClick:   return "DoubleClicked";
    case ImGuiTestAction_Check:         return "Checked";
    case ImGuiTestAction_Uncheck:       return "Unchecked";
    case ImGuiTestAction_Open:          return "Opened";
    case ImGuiTestAction_Close:         return "Closed";
    case ImGuiTestAction_Input:         return "Input";
    case ImGuiTestAction_NavActivate:   return "NavActivate";
    case ImGuiTestAction_COUNT:
    default:                            return "N/A";
    }
}

// Helper struct to store various queryable state of an item.
// This facilitate interactions between GuiFunc <> TestFunc, since those state are frequently used.
struct ImGuiTestGenericStatus
{
    int     Ret;
    int     Hovered;
    int     Active;
    int     Focused;
    int     Clicked;
    int     Visible;
    int     Edited;
    int     Activated;
    int     Deactivated;
    int     DeactivatedAfterEdit;

    ImGuiTestGenericStatus()    { Clear(); }
    void Clear()                { memset(this, 0, sizeof(*this)); }
    void QuerySet(bool ret_val = false) { Clear(); QueryInc(ret_val); }
    void QueryInc(bool ret_val = false)
    { 
        Ret += ret_val;
        Hovered += ImGui::IsItemHovered();
        Active += ImGui::IsItemActive();;
        Focused += ImGui::IsItemFocused();
        Clicked += ImGui::IsItemClicked();
        Visible += ImGui::IsItemVisible();
        Edited += ImGui::IsItemEdited();
        Activated += ImGui::IsItemActivated();
        Deactivated += ImGui::IsItemDeactivated();
        DeactivatedAfterEdit += ImGui::IsItemDeactivatedAfterEdit();
    }
};

enum ImGuiTestActiveFunc
{
    ImGuiTestActiveFunc_None,
    ImGuiTestActiveFunc_GuiFunc,
    ImGuiTestActiveFunc_TestFunc
};

// Generic structure with varied data. This is useful for tests to quickly share data between the GUI functions and the Test function.
// This is however totally optional. Using SetUserDataType() it is possible to store custom data on the stack and read from it as UserData.
struct ImGuiTestGenericVars
{
    int             Int1;
    int             Int2;
    int             IntArray[10];
    float           Float1;
    float           Float2;
    float           FloatArray[10];
    bool            Bool1;
    bool            Bool2;
    bool            BoolArray[10];
    ImVec4          Vec4;
    ImVec4          Vec4Array[10];
    ImGuiID         Id;
    ImGuiID         IdArray[10];
    char            Str1[256];
    char            Str2[256];
    ImVector<char>  StrLarge;
    void*           Ptr1;
    void*           Ptr2;
    void*           PtrArray[10];
    ImGuiID         DockId;
    ImGuiTestGenericStatus  Status;

    ImGuiTestGenericVars()  { Clear(); }
    void Clear()            { StrLarge.clear(); memset(this, 0, sizeof(*this)); }
    void ClearInts()        { Int1 = Int2 = 0; memset(IntArray, 0, sizeof(IntArray)); }
    void ClearBools()       { Bool1 = Bool2 = false; memset(BoolArray, 0, sizeof(BoolArray)); }
};

struct ImGuiTestContext
{
    ImGuiTestEngine*        Engine = NULL;
    ImGuiTest*              Test = NULL;
    ImGuiTestEngineIO*      EngineIO = NULL;
    ImGuiContext*           UiContext = NULL;
    ImGuiTestInputs*        Inputs;
    ImGuiTestGatherTask*    GatherTask;
    ImGuiTestRunFlags       RunFlags = ImGuiTestRunFlags_None;
    ImGuiTestGenericVars    GenericVars;
    ImGuiTestActiveFunc     ActiveFunc = ImGuiTestActiveFunc_None;
    void*                   UserData = NULL;
    int                     UserCounter = 0;
    int                     FrameCount = 0;             // Test frame count (restarts from zero every time)
    int                     FirstFrameCount = 0;        // First frame where Test is running. After warm-up. This is generally -2 or 0 depending on whether we have warm up enabled
    int                     ActionDepth = 0;
    bool                    Abort = false;
    char                    RefStr[256] = { 0 };
    ImGuiID                 RefID = 0;
    ImGuiInputSource        InputMode = ImGuiInputSource_Mouse;

    double                  PerfRefDt = -1.0;
    int                     PerfStressAmount = 0;

    ImGuiTestContext()      {}

    // Main control
    void        Finish();
    bool        IsError() const             { return Test->Status == ImGuiTestStatus_Error || Abort; }
    bool        IsFirstFrame() const        { return FrameCount == FirstFrameCount; }
    void        SetGuiFuncEnabled(bool v)   { if (v) RunFlags &= ~ImGuiTestRunFlags_NoGuiFunc; else RunFlags |= ImGuiTestRunFlags_NoGuiFunc; }
    void        RecoverFromUiContextErrors();
    template <typename T> T& GetUserData()  { IM_ASSERT(UserData != NULL); return *(T*)(UserData); }

    // Logging
    void        LogEx(ImGuiTestLogFlags flags, const char* fmt, ...) IM_FMTARGS(3); // implicit 'this' arg
    void        LogExV(ImGuiTestLogFlags flags, const char* fmt, va_list args) IM_FMTLIST(3);
    void        Log(const char* fmt, ...) IM_FMTARGS(2);
    void        LogVerbose(const char* fmt, ...) IM_FMTARGS(2);
    void        LogDebug();

    // Yield, Timing
    void        Yield();
    void        YieldFrames(int count);
    void        YieldUntil(int frame_count);
    void        Sleep(float time);
    void        SleepNoSkip(float time, float frame_time_step);
    void        SleepShort();

    // Windows
    // FIXME-TESTS: Refactor this horrible mess... perhaps all functions should have a ImGuiTestRef defaulting to empty?
    void        WindowRef(ImGuiTestRef ref);
    void        WindowClose(ImGuiTestRef ref);
    void        WindowCollapse(ImGuiTestRef ref, bool collapsed);
    void        WindowFocus(ImGuiTestRef ref);
    void        WindowMove(ImGuiTestRef ref, ImVec2 pos, ImVec2 pivot = ImVec2(0.0f, 0.0f));
    void        WindowResize(ImGuiTestRef ref, ImVec2 sz);
    void        WindowMoveToMakePosVisible(ImGuiWindow* window, ImVec2 pos);
    bool        WindowBringToFront(ImGuiWindow* window, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        PopupClose();
    ImGuiWindow* GetWindowByRef(ImGuiTestRef ref);
    ImGuiTestRef GetFocusWindowRef();

    // ID 
    ImGuiID     GetID(ImGuiTestRef ref);
    ImGuiID     GetID(ImGuiTestRef seed_ref, ImGuiTestRef ref);

    // Misc
    ImVec2      GetMainViewportPos();

    // Mouse inputs
    void        MouseMove(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void        MouseMoveToPos(ImVec2 pos);
    void        MouseMoveToPosInsideWindow(ImVec2* pos, ImGuiWindow* window);
    void        MouseClick(int button = 0);
    void        MouseDoubleClick(int button = 0);
    void        MouseDown(int button = 0);
    void        MouseUp(int button = 0);
    void        MouseLiftDragThreshold(int button = 0);
    
    // Keyboard inputs
    void        KeyDownMap(ImGuiKey key, int mod_flags = 0);
    void        KeyUpMap(ImGuiKey key, int mod_flags = 0);
    void        KeyPressMap(ImGuiKey key, int mod_flags = 0, int count = 1);
    void        KeyChars(const char* chars);
    void        KeyCharsAppend(const char* chars);
    void        KeyCharsAppendEnter(const char* chars);

    // Navigation inputs
    void        SetInputMode(ImGuiInputSource input_mode);
    void        NavMove(ImGuiTestRef ref);
    void        NavActivate();
    void        NavInput();

    // Scrolling
    void        ScrollToY(ImGuiTestRef ref, float scroll_ratio_y = 0.5f);
    void        ScrollVerifyScrollMax(ImGuiWindow* window);

    // Low-level queries
    ImGuiTestItemInfo*  ItemLocate(ImGuiTestRef ref, ImGuiTestOpFlags flags = ImGuiTestOpFlags_None);
    void                GatherItems(ImGuiTestItemList* out_list, ImGuiTestRef parent, int depth = -1);

    // Item/Widgets manipulation
    void        ItemAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        ItemClick(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Click, ref); }
    void        ItemDoubleClick(ImGuiTestRef ref)   { ItemAction(ImGuiTestAction_DoubleClick, ref); }
    void        ItemCheck(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Check, ref); }
    void        ItemUncheck(ImGuiTestRef ref)       { ItemAction(ImGuiTestAction_Uncheck, ref); }
    void        ItemOpen(ImGuiTestRef ref)          { ItemAction(ImGuiTestAction_Open, ref); }
    void        ItemClose(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Close, ref); }
    void        ItemInput(ImGuiTestRef ref)         { ItemAction(ImGuiTestAction_Input, ref); }
    void        ItemNavActivate(ImGuiTestRef ref)   { ItemAction(ImGuiTestAction_NavActivate, ref); }

    void        ItemActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent, int depth = -1, int passes = -1);
    void        ItemOpenAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)    { ItemActionAll(ImGuiTestAction_Open, ref_parent, depth, passes); }
    void        ItemCloseAll(ImGuiTestRef ref_parent, int depth = -1, int passes = -1)   { ItemActionAll(ImGuiTestAction_Close, ref_parent, depth, passes); }

    void        ItemHold(ImGuiTestRef ref, float time);
    void        ItemHoldForFrames(ImGuiTestRef ref, int frames);
    void        ItemDragAndDrop(ImGuiTestRef ref_src, ImGuiTestRef ref_dst);
    void        ItemVerifyCheckedIfAlive(ImGuiTestRef ref, bool checked);

    // Menus
    void        MenuAction(ImGuiTestAction action, ImGuiTestRef ref);
    void        MenuActionAll(ImGuiTestAction action, ImGuiTestRef ref_parent);
    void        MenuClick(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Click, ref); }
    void        MenuCheck(ImGuiTestRef ref)                 { MenuAction(ImGuiTestAction_Check, ref); }
    void        MenuUncheck(ImGuiTestRef ref)               { MenuAction(ImGuiTestAction_Uncheck, ref); }
    void        MenuCheckAll(ImGuiTestRef ref_parent)       { MenuActionAll(ImGuiTestAction_Check, ref_parent); }
    void        MenuUncheckAll(ImGuiTestRef ref_parent)     { MenuActionAll(ImGuiTestAction_Uncheck, ref_parent); }

    // Docking
#ifdef IMGUI_HAS_DOCK
    void        DockWindowInto(const char* window_src, const char* window_dst, ImGuiDir split_dir = ImGuiDir_None);
    void        DockMultiClear(const char* window_name, ...);
    void        DockMultiSet(ImGuiID dock_id, const char* window_name, ...);
    ImGuiID     DockMultiSetupBasic(ImGuiID dock_id, const char* window_name, ...);
    bool        DockIdIsUndockedOrStandalone(ImGuiID dock_id);
    void        UndockNode(ImGuiID dock_id);
#endif

    // Performances
    void        PerfCalcRef();
    void        PerfCapture();
};

#define IM_TOKENPASTE(x, y)     x ## y
#define IM_TOKENPASTE2(x, y)    IM_TOKENPASTE(x, y)
#define IMGUI_TEST_CONTEXT_REGISTER_DEPTH(_THIS)            ImGuiTestContextDepthRegister IM_TOKENPASTE2(depth_register, __LINE__)(_THIS)

struct ImGuiTestContextDepthRegister
{
    ImGuiTestContext*       TestContext;
    ImGuiTestContextDepthRegister(ImGuiTestContext* ctx)    { TestContext = ctx; TestContext->ActionDepth++; }
    ~ImGuiTestContextDepthRegister()                        { TestContext->ActionDepth--; }
};