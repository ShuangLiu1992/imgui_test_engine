// dear imgui
// (tests: viewports)

#define _CRT_SECURE_NO_WARNINGS
#include <limits.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_test_engine/imgui_te_engine.h"      // IM_REGISTER_TEST()
#include "imgui_test_engine/imgui_te_context.h"

// Warnings
#ifdef _MSC_VER
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// Operators
static inline bool operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }    // for IM_CHECK_EQ()
static inline bool operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }    // for IM_CHECK_NE()

//-------------------------------------------------------------------------
// Ideas/Specs for future tests
// It is important we take the habit to write those down.
// - Even if we don't implement the test right away: they allow us to remember edge cases and interesting things to test.
// - Even if they will be hard to actually implement/automate, they still allow us to manually check things.
//-------------------------------------------------------------------------
// TODO: Tests: Viewport: host A, viewport B over A, make new Modal appears where geometry overlap A -> SHOULD APPEAR OVER B
// TODO: Tests: Viewport: host A, viewport B over A, window C with TopMost, move over host A (may be merged) then drag over B -> SHOULD APPEAR OVER B AKA NOT MERGE IN A, OR, UNMERGE FROM A
//              Actually same test is meaningful even without TopMost flag, since clicking C supposedly should bring it to front, C merged in A and dragging around B should appear over B
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Tests: Viewports
//-------------------------------------------------------------------------

void RegisterTests_Viewports(ImGuiTestEngine* e)
{
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiTest* t = NULL;

    // Require -viewport to register those tests
#if 1 // Set to 0 to verify that a given tests actually fails without viewport enabled
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
        return;
    if ((ImGui::GetIO().BackendFlags & ImGuiBackendFlags_PlatformHasViewports) == 0)
        return;
    if ((ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasViewports) == 0)
        return;
#endif

    // ## Basic viewport test
    t = IM_REGISTER_TEST(e, "viewport", "viewport_basic_1");
    //t->Flags |= ImGuiTestFlags_RequireViewports;
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();

        if (vars.Step == 0)
        {
            ImGui::SetNextWindowSize(ImVec2(200, 200));
            ImGui::SetNextWindowPos(main_viewport->WorkPos + ImVec2(20, 20));
        }

        if (vars.Step == 2)
        {
            ImGuiWindow* window = ImGui::FindWindowByName("Test Window");
            if (ctx->UiContext->MovingWindow == window)
                vars.Count++;
            if (vars.Count >= 1)
                return;
        }

        ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus); // _NoBringToFrontOnFocus to avoid stray in-between viewport preventing merge. FIXME-TESTS: should test both
        ImGui::Text("hello!");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ctx->SetRef("Test Window");

        // Inside main viewport
        ImGuiWindow* window = ctx->GetWindowByRef("");
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGuiTestGenericVars& vars = ctx->GenericVars;
        vars.Step = 0;
        ctx->Yield();
        IM_CHECK(window->Viewport == main_viewport);

        // Outside, create its own viewport
        vars.Step = 1;
        ctx->WindowMove("", main_viewport->Pos - ImVec2(20.0f, 20.0f));
        IM_CHECK(window->Viewport != main_viewport);
        IM_CHECK(window->ViewportOwned);
        IM_CHECK(window->Viewport->Window == window);

        // Back inside
        ctx->WindowMove("", main_viewport->WorkPos + ImVec2(20.0f, 20.0f));
        IM_CHECK(window->Viewport == main_viewport);
        IM_CHECK(window->Viewport->Window == NULL);
        IM_CHECK_EQ(window->Pos, main_viewport->WorkPos + ImVec2(20.0f, 20.0f));

        // Test viewport disappearing while moving
        ctx->WindowMove("", main_viewport->Pos - ImVec2(20.0f, 20.0f));
        vars.Step = 2;
        vars.Count = 0;
        ctx->WindowMove("", main_viewport->WorkPos + ImVec2(100.0f, 100.0f));
        IM_CHECK_NE(window->Pos, main_viewport->WorkPos + ImVec2(100.0f, 100.0f));
    };

    // ## Test value of ParentViewportID (include bug #4756)
    t = IM_REGISTER_TEST(e, "viewport", "viewport_parent_id");
    struct DefaultParentIdVars { ImGuiWindowClass WindowClass; bool SetWindowClass = false; };
    t->SetVarsDataType<DefaultParentIdVars>();
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        DefaultParentIdVars& vars = ctx->GetVars<DefaultParentIdVars>();
        if (vars.SetWindowClass)
            ImGui::SetNextWindowClass(&vars.WindowClass);
        ImGui::SetNextWindowSize(ImVec2(50, 50));
        ImGui::Begin("Test Window with Class", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        DefaultParentIdVars& vars = ctx->GetVars<DefaultParentIdVars>();
        ctx->SetRef("Test Window with Class");
        ImGuiWindow* window = ctx->GetWindowByRef("");

        // Reset class at it currently persist if not set
        vars.WindowClass = ImGuiWindowClass();
        vars.SetWindowClass = true;
        ctx->Yield();

        // Default value.
        ctx->WindowMove("", ImGui::GetMainViewport()->Pos + ImVec2(100, 100));
        IM_CHECK_EQ(window->ViewportOwned, false);
        IM_CHECK_EQ(window->WindowClass.ParentViewportId, (ImGuiID)-1);  // Default value

        // Unset. Result depends on ConfigViewportsNoDefaultParent value.
        vars.WindowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
        vars.SetWindowClass = true;

        g.IO.ConfigViewportsNoDefaultParent = true;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0);

        g.IO.ConfigViewportsNoDefaultParent = false;
        ctx->Yield();
        const ImGuiID IMGUI_VIEWPORT_DEFAULT_ID = ImGui::GetMainViewport()->ID;
        IM_CHECK_EQ(window->Viewport->ParentViewportId, IMGUI_VIEWPORT_DEFAULT_ID);

        // Explicitly set parent viewport id. 0 may or may not be a special value. Currently it isn't.
        vars.WindowClass.ParentViewportId = 0;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0);

        // This is definitely a non-special value.
        vars.WindowClass.ParentViewportId = 0x12345678;
        ctx->Yield();
        IM_CHECK_EQ(window->Viewport->ParentViewportId, (ImGuiID)0x12345678);
    };

    // Test Platform Focus leading to Dear ImGui focus (#6299)
    t = IM_REGISTER_TEST(e, "viewport", "viewport_platform_focus");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(200, 200)); // Assume fitting in host viewport
        ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button A");
        ImGui::BeginChild("Child A", ImVec2(100, 100), true);
        ImGui::Button("Button Child A");
        ImGui::EndChild();
        ImGui::End();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos + ImVec2(ImGui::GetMainViewport()->Size.x, 10));
        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button B");
        ImGui::BeginChild("Child B", ImVec2(100, 100), true);
        ImGui::Button("Button Child B");
        ImGui::EndChild();
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiWindow* window_a = ctx->WindowInfo("//Window A")->Window;
        ImGuiWindow* window_b = ctx->WindowInfo("//Window B")->Window;
        ImGuiWindow* window_child_a = ctx->WindowInfo("//Window A/Child A")->Window;
        ImGuiWindow* window_child_b = ctx->WindowInfo("//Window B/Child B")->Window;
        IM_ASSERT(window_a && window_b && window_child_a && window_child_b);
        ctx->Yield();

        ImGuiViewportP* viewport_a = window_a->Viewport;
        ImGuiViewportP* viewport_b = window_b->Viewport; IM_UNUSED(viewport_b);
        IM_CHECK(window_a->Viewport != NULL);
        IM_CHECK(window_a->ViewportId == ImGui::GetMainViewport()->ID);
        IM_CHECK(window_b->Viewport != NULL);
        IM_CHECK(window_b->ViewportId != ImGui::GetMainViewport()->ID);

        ctx->ItemClick("//Window A/Button A");
        IM_CHECK_EQ(g.NavWindow, window_a);
        ctx->ViewportPlatform_SetWindowFocus(viewport_a);   // No-op
        IM_CHECK_EQ(g.NavWindow, window_a);

#if IMGUI_VERSION_NUM >= 18948
        ctx->ViewportPlatform_SetWindowFocus(viewport_b);
        IM_CHECK_EQ(g.NavWindow, window_b);

        ctx->ViewportPlatform_SetWindowFocus(viewport_a);
        IM_CHECK_EQ(g.NavWindow, window_a);
        ctx->SetRef(window_child_a);
        ctx->ItemClick("Button Child A");
        IM_CHECK_EQ(g.NavWindow, window_child_a);
        ctx->ViewportPlatform_SetWindowFocus(viewport_b);
        IM_CHECK_EQ(g.NavWindow, window_b);
        ctx->ViewportPlatform_SetWindowFocus(viewport_a);
        IM_CHECK_EQ(g.NavWindow, window_child_a);

        ctx->ViewportPlatform_SetWindowFocus(viewport_b);
        ctx->SetRef(window_child_b);
        ctx->ItemClick("Button Child B");
        IM_CHECK_EQ(g.NavWindow, window_child_b);
        ctx->ViewportPlatform_SetWindowFocus(viewport_a);
        IM_CHECK_EQ(g.NavWindow, window_child_a);
        ctx->ViewportPlatform_SetWindowFocus(viewport_b);
        IM_CHECK_EQ(g.NavWindow, window_child_b);

        ctx->MouseClickOnVoid(ImGuiMouseButton_Left, viewport_a);
        IM_CHECK(g.NavWindow == NULL);

        // Verify that TestEngine mouse actions are propagated to focus.
        IM_CHECK((viewport_a->Flags & ImGuiViewportFlags_IsFocused) != 0);
        IM_CHECK((viewport_b->Flags & ImGuiViewportFlags_IsFocused) == 0);
        IM_CHECK_GT(viewport_a->LastFocusedStampCount, viewport_b->LastFocusedStampCount);

        // Verify that NULL focus is preserved after refocusing Viewport A which didn't have focus.
        ctx->SetRef("");
        ctx->ItemClick("//Window B/Button B");
        IM_CHECK((viewport_b->Flags & ImGuiViewportFlags_IsFocused) != 0);
        IM_CHECK_EQ(g.NavWindow, window_b);
        ctx->ViewportPlatform_SetWindowFocus(viewport_a);
        IM_CHECK(g.NavWindow == NULL);
#endif
    };

    // More tests Platform Focus leading to Dear ImGui focus (#6299)
#if IMGUI_VERSION_NUM >= 18956
    t = IM_REGISTER_TEST(e, "viewport", "viewport_platform_focus_2");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        ImGui::Begin("Window A", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button A");
        ImGui::End();

        ImGui::Begin("Window B", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::Button("Button B");
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiContext& g = *ctx->UiContext;
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGuiWindow* window_a = ctx->WindowInfo("//Window A")->Window;
        ImGuiWindow* window_b = ctx->WindowInfo("//Window B")->Window;

        // Move all other windows which could be occluding to main viewport
        // FIXME-TESTS: Could add dedicated/shared helpers?
        for (ImGuiWindow* window : g.WindowsFocusOrder)
            if (window != window_a && window != window_b && window->Viewport != main_viewport && window->WasActive)
            {
                ctx->WindowResize(window->ID, ImVec2(100, 100));
                ctx->WindowMove(window->ID, main_viewport->Pos + ImVec2(10, 10));
                IM_CHECK(window->Viewport == main_viewport);
            }

        ctx->WindowResize("//Window A", ImVec2(100, 100));
        ctx->WindowMove("//Window A", main_viewport->Pos + ImVec2(10,10));
        IM_CHECK(window_a->Viewport == main_viewport);

        ctx->WindowResize("//Window B", ImVec2(100, 100));
        ctx->WindowMove("//Window B", main_viewport->Pos + ImVec2(30, 30));
        IM_CHECK(window_b->Viewport == main_viewport);

        ctx->WindowMove("//Window B", main_viewport->Pos + ImVec2(main_viewport->Size.x, 0.0f));
        IM_CHECK(window_b->Viewport != main_viewport);
        IM_CHECK(g.NavWindow == window_b);

        ctx->WindowMove("//Window B", main_viewport->Pos + ImVec2(30, 30));
        IM_CHECK(window_b->Viewport == main_viewport);
        IM_CHECK(g.NavWindow == window_b);
    };
#endif

    // More tests Platform Focus leading to Dear ImGui focus (#6462)
    // Test closing a popup in viewport and opening another one also in viewport, temporary focus of parent viewport should not interfere.
#if IMGUI_VERSION_NUM >= 18958
    t = IM_REGISTER_TEST(e, "viewport", "viewport_platform_focus_3");
    t->GuiFunc = [](ImGuiTestContext* ctx)
    {
        if (ImGui::Begin("Test Window", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) // will make small
        {
            if (ImGui::BeginPopup("second"))
            {
                ImGui::Text("success message!");
                ImGui::EndPopup();
            }

            bool open_second_popup = false;
            if (ImGui::BeginPopup("first"))
            {
                if (ImGui::Button("open second popup"))
                    open_second_popup = true;
                ImGui::EndPopup();
            }
            if (open_second_popup)
                ImGui::OpenPopup("second");

            if (ImGui::Button("open first popup"))
                ImGui::OpenPopup("first");
        }
        ImGui::End();
    };
    t->TestFunc = [](ImGuiTestContext* ctx)
    {
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGuiWindow* window = ctx->WindowInfo("//Test Window")->Window;
        IM_CHECK(window != NULL);
        ctx->WindowMove(window->ID, main_viewport->Pos + ImVec2(main_viewport->Size.x - window->Size.x - 2.0f, 10.0f));
        IM_CHECK(window->Viewport == main_viewport);

        ctx->ItemClick("//Test Window/open first popup");
        ImGuiWindow* popup_1 = ctx->WindowInfo("//$FOCUSED")->Window;
        IM_CHECK(popup_1 && (popup_1->Flags & ImGuiWindowFlags_Popup));
        IM_CHECK(popup_1->Viewport != main_viewport);

        // Important: as per https://github.com/ocornut/imgui/issues/6462, prior to our fix this depends on backend handling
        // the ImGuiViewportFlags_NoFocusOnClick flag or not. Behavior may be different depending on backend (e.g. mock viewport support vs interactive win32/glfw backend).
        ctx->ItemClick("//$FOCUSED/open second popup");
        ImGuiWindow* popup_2 = ctx->WindowInfo("//$FOCUSED")->Window;
        IM_CHECK(popup_2 && (popup_2->Flags & ImGuiWindowFlags_Popup));
        IM_CHECK(popup_2 != popup_1);
        IM_CHECK(popup_2->Viewport != main_viewport);
    };
#endif

#else
    IM_UNUSED(e);
#endif
}
