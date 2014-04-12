// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_CONTEXT_MENU_CONTEXT_MENU_CONTENT_TYPE_PANEL_H_
#define CHROME_BROWSER_RENDERER_CONTEXT_MENU_CONTEXT_MENU_CONTENT_TYPE_PANEL_H_

#include "chrome/browser/renderer_context_menu/context_menu_content_type.h"

class ContextMenuContentTypePanel : public ContextMenuContentType {
 public:
  virtual ~ContextMenuContentTypePanel();

  // ContextMenuContentType overrides.
  virtual bool SupportsGroup(int group) OVERRIDE;

 protected:
  ContextMenuContentTypePanel(content::WebContents* web_contents,
                              const content::ContextMenuParams& params);

 private:
  friend class ContextMenuContentTypeFactory;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuContentTypePanel);
};

#endif  // CHROME_BROWSER_RENDERER_CONTEXT_MENU_CONTEXT_MENU_CONTENT_TYPE_PANEL_H_
