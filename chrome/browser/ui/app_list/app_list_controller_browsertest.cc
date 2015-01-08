// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/app_list_service.h"
#include "chrome/browser/ui/app_list/test/chrome_app_list_test_support.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/app_list/app_list_model.h"
#include "ui/app_list/app_list_switches.h"
#include "ui/app_list/search_box_model.h"
#include "ui/app_list/search_result.h"
#include "ui/app_list/search_result_observer.h"
#include "ui/base/models/list_model_observer.h"

// Browser Test for AppListController that runs on all platforms supporting
// app_list.
typedef InProcessBrowserTest AppListControllerBrowserTest;

// Test the CreateNewWindow function of the controller delegate.
IN_PROC_BROWSER_TEST_F(AppListControllerBrowserTest, CreateNewWindow) {
  const chrome::HostDesktopType desktop = chrome::GetActiveDesktop();
  AppListService* service = test::GetAppListService();
  AppListControllerDelegate* controller(service->GetControllerDelegate());
  ASSERT_TRUE(controller);

  EXPECT_EQ(1U, chrome::GetBrowserCount(browser()->profile(), desktop));
  EXPECT_EQ(0U, chrome::GetBrowserCount(
      browser()->profile()->GetOffTheRecordProfile(), desktop));

  controller->CreateNewWindow(browser()->profile(), false);
  EXPECT_EQ(2U, chrome::GetBrowserCount(browser()->profile(), desktop));

  controller->CreateNewWindow(browser()->profile(), true);
  EXPECT_EQ(1U, chrome::GetBrowserCount(
      browser()->profile()->GetOffTheRecordProfile(), desktop));
}

// Test creating the app list for an incognito version of the current profile.
IN_PROC_BROWSER_TEST_F(AppListControllerBrowserTest, RegularThenIncognito) {
  AppListService* service = test::GetAppListService();
  // On Ash, the app list always has a profile.
  if (chrome::GetActiveDesktop() == chrome::HOST_DESKTOP_TYPE_ASH)
    EXPECT_TRUE(service->GetCurrentAppListProfile());
  else
    EXPECT_FALSE(service->GetCurrentAppListProfile());

  service->ShowForProfile(browser()->profile());
  EXPECT_EQ(browser()->profile(), service->GetCurrentAppListProfile());

  AppListControllerDelegate* controller = service->GetControllerDelegate();
  service->ShowForProfile(browser()->profile()->GetOffTheRecordProfile());

  // Should be showing the same profile.
  EXPECT_EQ(browser()->profile(), service->GetCurrentAppListProfile());

  // Should not have been reconstructed.
  EXPECT_EQ(controller, service->GetControllerDelegate());

  // Same set of tests using ShowForAppInstall, which the webstore uses and was
  // traditionally where issues with incognito-mode app launchers started. Pass
  // true for start_discovery_tracking to emulate what the webstore does on the
  // first app install, to cover AppListServiceImpl::CreateForProfile().
  service->ShowForAppInstall(
      browser()->profile()->GetOffTheRecordProfile(), "", true);
  EXPECT_EQ(browser()->profile(), service->GetCurrentAppListProfile());
  EXPECT_EQ(controller, service->GetControllerDelegate());
}

// Test creating the initial app list for incognito profile.
IN_PROC_BROWSER_TEST_F(AppListControllerBrowserTest, Incognito) {
  AppListService* service = test::GetAppListService();
  if (chrome::GetActiveDesktop() == chrome::HOST_DESKTOP_TYPE_ASH)
    EXPECT_TRUE(service->GetCurrentAppListProfile());
  else
    EXPECT_FALSE(service->GetCurrentAppListProfile());

  service->ShowForProfile(browser()->profile()->GetOffTheRecordProfile());
  // Initial load should have picked the non-incongito profile.
  EXPECT_EQ(browser()->profile(), service->GetCurrentAppListProfile());
}

// Browser Test for AppListController that observes search result changes.
class AppListControllerSearchResultsBrowserTest
    : public ExtensionBrowserTest,
      public app_list::SearchResultObserver,
      public ui::ListModelObserver {
 public:
  AppListControllerSearchResultsBrowserTest()
      : observed_result_(NULL),
        observed_results_list_(NULL) {}

  void WatchResultsLookingForItem(
      app_list::AppListModel::SearchResults* search_results,
      const std::string& extension_name) {
    EXPECT_FALSE(observed_results_list_);
    observed_results_list_ = search_results;
    observed_results_list_->AddObserver(this);
    item_to_observe_ = base::ASCIIToUTF16(extension_name);
  }

  void StopWatchingResults() {
    EXPECT_TRUE(observed_results_list_);
    observed_results_list_->RemoveObserver(this);
  }

 protected:
  void AttemptToLocateItem() {
    if (observed_result_) {
      observed_result_->RemoveObserver(this);
      observed_result_ = NULL;
    }

    for (size_t i = 0; i < observed_results_list_->item_count(); ++i) {
      if (observed_results_list_->GetItemAt(i)->title() != item_to_observe_)
        continue;

      // Ensure there is at most one.
      EXPECT_FALSE(observed_result_);
      observed_result_ = observed_results_list_->GetItemAt(i);
    }

    if (observed_result_)
      observed_result_->AddObserver(this);
  }

  // Overridden from SearchResultObserver:
  void OnIconChanged() override {}
  void OnActionsChanged() override {}
  void OnIsInstallingChanged() override {}
  void OnPercentDownloadedChanged() override {}
  void OnItemInstalled() override {}

  // Overridden from ui::ListModelObserver:
  void ListItemsAdded(size_t start, size_t count) override {
    AttemptToLocateItem();
  }
  void ListItemsRemoved(size_t start, size_t count) override {
    AttemptToLocateItem();
  }
  void ListItemMoved(size_t index, size_t target_index) override {}
  void ListItemsChanged(size_t start, size_t count) override {}

  app_list::SearchResult* observed_result_;

 private:
  base::string16 item_to_observe_;
  app_list::AppListModel::SearchResults* observed_results_list_;

  DISALLOW_COPY_AND_ASSIGN(AppListControllerSearchResultsBrowserTest);
};


// Flaky on Mac. https://crbug.com/415264
#if defined(OS_MACOSX)
#define MAYBE_UninstallSearchResult DISABLED_UninstallSearchResult
#else
#define MAYBE_UninstallSearchResult UninstallSearchResult
#endif
// Test showing search results, and uninstalling one of them while displayed.
IN_PROC_BROWSER_TEST_F(AppListControllerSearchResultsBrowserTest,
                       MAYBE_UninstallSearchResult) {
  // TODO(calamity): This test fails in the experimental app list
  // (http://crbug.com/438119). For now, force the test to run in the classic
  // app list.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      app_list::switches::kDisableExperimentalAppList);

  base::FilePath test_extension_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &test_extension_path));
  test_extension_path = test_extension_path.AppendASCII("extensions")
      .AppendASCII("platform_apps")
      .AppendASCII("minimal");
  const extensions::Extension* extension =
      InstallExtension(test_extension_path,
                       1 /* expected_change: new install */);
  ASSERT_TRUE(extension);

  AppListService* service = test::GetAppListService();
  ASSERT_TRUE(service);
  service->ShowForProfile(browser()->profile());

  app_list::AppListModel* model = test::GetAppListModel(service);
  ASSERT_TRUE(model);
  WatchResultsLookingForItem(model->results(), extension->name());

  // Ensure a search finds the extension.
  EXPECT_FALSE(observed_result_);
  model->search_box()->SetText(base::ASCIIToUTF16("minimal"));
  EXPECT_TRUE(observed_result_);

  // Ensure the UI is updated. This is via PostTask in views.
  base::RunLoop().RunUntilIdle();

  // Now uninstall and ensure this browser test observes it.
  UninstallExtension(extension->id());

  // Allow async AppSearchProvider::UpdateResults to run.
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(observed_result_);
  StopWatchingResults();
  service->DismissAppList();
}
