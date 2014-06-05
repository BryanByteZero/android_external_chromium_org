// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/constrained_window_views.h"

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/tab_modal_confirm_dialog_views.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace {

class TestDialog : public views::DialogDelegateView {
 public:
  TestDialog() { SetFocusable(true); }
  virtual ~TestDialog() {}

  virtual views::View* GetInitiallyFocusedView() OVERRIDE { return this; }
  // Don't delete the delegate yet. Keep it around for inspection later.
  virtual void DeleteDelegate() OVERRIDE {}

  virtual ui::ModalType GetModalType() const OVERRIDE {
#if defined(USE_ASH)
    return ui::MODAL_TYPE_CHILD;
#else
    return views::WidgetDelegate::GetModalType();
#endif
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestDialog);
};

// A helper function to create and show a web contents modal dialog.
scoped_ptr<TestDialog> ShowModalDialog(content::WebContents* web_contents) {
  scoped_ptr<TestDialog> dialog(new TestDialog());
  ShowWebModalDialogViews(dialog.get(), web_contents);
  return dialog.Pass();
}

} // namespace

typedef InProcessBrowserTest ConstrainedWindowViewTest;

// Tests the intial focus of tab-modal dialogs, the restoration of focus to the
// browser when they close, and that queued dialogs don't register themselves as
// accelerator targets until they are displayed.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, FocusTest) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  scoped_ptr<TestDialog> dialog1 = ShowModalDialog(web_contents);

  // |dialog1| should be active and focused.
  EXPECT_TRUE(dialog1->GetWidget()->IsVisible());
  views::FocusManager* focus_manager = dialog1->GetWidget()->GetFocusManager();
  EXPECT_EQ(dialog1->GetContentsView(), focus_manager->GetFocusedView());

  // Create a second dialog. This will also be modal to |web_contents|, but will
  // remain hidden since the |dialog1| is still showing.
  scoped_ptr<TestDialog> dialog2 = ShowModalDialog(web_contents);
  EXPECT_FALSE(dialog2->GetWidget()->IsVisible());
  EXPECT_TRUE(dialog1->GetWidget()->IsVisible());
  EXPECT_EQ(focus_manager, dialog2->GetWidget()->GetFocusManager());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_EQ(dialog1->GetContentsView(), focus_manager->GetFocusedView());

  // Pressing return should close |dialog1|.
  EXPECT_TRUE(focus_manager->ProcessAccelerator(
      ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE)));
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog1->GetWidget());

  // |dialog2| should be visible and focused.
  EXPECT_TRUE(dialog2->GetWidget()->IsVisible());
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_EQ(dialog2->GetContentsView(), focus_manager->GetFocusedView());

  // Creating a new tab should take focus away from the other tab's dialog.
  const int tab_with_dialog = browser()->tab_strip_model()->active_index();
  chrome::NewTab(browser());
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_NE(dialog2->GetContentsView(), focus_manager->GetFocusedView());

  // Activating the previous tab should bring focus to the dialog.
  browser()->tab_strip_model()->ActivateTabAt(tab_with_dialog, false);
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_OMNIBOX));
  EXPECT_EQ(dialog2->GetContentsView(), focus_manager->GetFocusedView());

  // Pressing enter again should close |dialog2|.
  EXPECT_TRUE(focus_manager->ProcessAccelerator(
      ui::Accelerator(ui::VKEY_RETURN, ui::EF_NONE)));
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog2->GetWidget());
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_TAB_CONTAINER));
}

// Tests that the tab-modal window is closed properly when its tab is closed.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, TabCloseTest) {
  scoped_ptr<TestDialog> dialog = ShowModalDialog(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());
  chrome::CloseTab(browser());
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog->GetWidget());
}

// Tests that the tab-modal window is hidden when an other tab is selected and
// shown when its tab is selected again.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, TabSwitchTest) {
  scoped_ptr<TestDialog> dialog = ShowModalDialog(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());

  // Open a new tab. The tab-modal window should hide itself.
  chrome::NewTab(browser());
  EXPECT_FALSE(dialog->GetWidget()->IsVisible());

  // Close the new tab. The tab-modal window should show itself again.
  chrome::CloseTab(browser());
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());

  // Close the original tab.
  chrome::CloseTab(browser());
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog->GetWidget());
}

// Tests that tab-modal dialogs follow tabs dragged between browser windows.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, TabMoveTest) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  scoped_ptr<TestDialog> dialog = ShowModalDialog(web_contents);
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());

  // Move the tab to a second browser window; but first create another tab.
  // That prevents the first browser window from closing when its tab is moved.
  chrome::NewTab(browser());
  browser()->tab_strip_model()->DetachWebContentsAt(
      browser()->tab_strip_model()->GetIndexOfWebContents(web_contents));
  Browser* browser2 = CreateBrowser(browser()->profile());
  browser2->tab_strip_model()->AppendWebContents(web_contents, true);
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());

  // Close the first browser.
  chrome::CloseWindow(browser());
  content::RunAllPendingInMessageLoop();
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());

  // Close the dialog's browser window.
  chrome::CloseTab(browser2);
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog->GetWidget());
}

// Tests that the web contents navigates when backspace is pressed.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, NavigationOnBackspace) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  content::WaitForLoadStop(web_contents);
  const GURL original_url = web_contents->GetURL();
  EXPECT_NE(GURL(chrome::kChromeUIVersionURL), original_url);
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIVersionURL));
  content::WaitForLoadStop(web_contents);
  EXPECT_EQ(GURL(chrome::kChromeUIVersionURL), web_contents->GetURL());

  scoped_ptr<TestDialog> dialog = ShowModalDialog(web_contents);
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());
  EXPECT_EQ(dialog->GetContentsView(),
            dialog->GetWidget()->GetFocusManager()->GetFocusedView());

  // Pressing backspace should navigate back and close the dialog.
  EXPECT_TRUE(chrome::CanGoBack(browser()));
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_BACK,
                                              false, false, false, false));
  content::RunAllPendingInMessageLoop();
  content::WaitForLoadStop(web_contents);
  EXPECT_EQ(NULL, dialog->GetWidget());
  EXPECT_EQ(original_url, web_contents->GetURL());
}

// Tests that the dialog closes when the escape key is pressed.
IN_PROC_BROWSER_TEST_F(ConstrainedWindowViewTest, ClosesOnEscape) {
#if defined(OS_WIN)
  // TODO(msw): The widget is not made NULL on XP. http://crbug.com/177482
  if (base::win::GetVersion() < base::win::VERSION_VISTA)
    return;
#endif

  scoped_ptr<TestDialog> dialog = ShowModalDialog(
      browser()->tab_strip_model()->GetActiveWebContents());
  EXPECT_TRUE(dialog->GetWidget()->IsVisible());
  EXPECT_TRUE(ui_test_utils::SendKeyPressSync(browser(), ui::VKEY_ESCAPE,
                                              false, false, false, false));
  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(NULL, dialog->GetWidget());
}
