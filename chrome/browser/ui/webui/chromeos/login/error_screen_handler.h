// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_

#include "base/cancelable_callback.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/chromeos/login/screens/error_screen_actor.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_state_informer.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

class CaptivePortalWindowProxy;
class NativeWindowDelegate;
class NetworkStateInformer;

// A class that handles the WebUI hooks in error screen.
class ErrorScreenHandler : public BaseScreenHandler,
                           public ErrorScreenActor {
 public:
  ErrorScreenHandler(
      const scoped_refptr<NetworkStateInformer>& network_state_informer);
  ~ErrorScreenHandler() override;

  // ErrorScreenActor implementation:
  void SetDelegate(ErrorScreenActorDelegate* delegate) override;
  void Show(OobeDisplay::Screen parent_screen,
            base::DictionaryValue* params) override;
  void Show(OobeDisplay::Screen parent_screen,
            base::DictionaryValue* params,
            const base::Closure& on_hide) override;
  void Hide() override;
  void FixCaptivePortal() override;
  void ShowCaptivePortal() override;
  void HideCaptivePortal() override;
  void SetUIState(ErrorScreen::UIState ui_state) override;
  void SetErrorState(ErrorScreen::ErrorState error_state,
                     const std::string& network) override;
  void AllowGuestSignin(bool allowed) override;
  void AllowOfflineLogin(bool allowed) override;
  void ShowConnectingIndicator(bool show) override;

 private:
  // Sends notification that error message is shown.
  void NetworkErrorShown();

  bool GetScreenName(OobeUI::Screen screen, std::string* name) const;

  // Default hide_closure for Show/Hide.
  void CheckAndShowScreen();

  // WebUI message handlers.
  void HandleShowCaptivePortal();
  void HandleHideCaptivePortal();
  void HandleLocalStateErrorPowerwashButtonClicked();
  void HandleRebootButtonClicked();
  void HandleDiagnoseButtonClicked();
  void HandleConfigureCerts();
  void HandleLaunchOobeGuestSession();

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // Non-owning ptr.
  ErrorScreenActorDelegate* delegate_;

  // Proxy which manages showing of the window for captive portal entering.
  scoped_ptr<CaptivePortalWindowProxy> captive_portal_window_proxy_;

  // Network state informer used to keep error screen up.
  scoped_refptr<NetworkStateInformer> network_state_informer_;

  // NativeWindowDelegate used to get reference to NativeWindow.
  NativeWindowDelegate* native_window_delegate_;

  // Keeps whether screen should be shown right after initialization.
  bool show_on_init_;

  scoped_ptr<base::Closure> on_hide_;

  base::WeakPtrFactory<ErrorScreenHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ErrorScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ERROR_SCREEN_HANDLER_H_
