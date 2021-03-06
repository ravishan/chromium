// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/browser_plugin/browser_plugin_manager.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/browser_plugin/browser_plugin_constants.h"
#include "content/common/browser_plugin/browser_plugin_messages.h"
#include "content/common/frame_messages.h"
#include "content/public/renderer/browser_plugin_delegate.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/browser_plugin/browser_plugin.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/ipc_message_macros.h"

namespace content {

// static
BrowserPluginManager* BrowserPluginManager::Get() {
  if (!RenderThreadImpl::current())
    return nullptr;
  return RenderThreadImpl::current()->browser_plugin_manager();
}

BrowserPluginManager::BrowserPluginManager() {
}

BrowserPluginManager::~BrowserPluginManager() {
}

void BrowserPluginManager::AddBrowserPlugin(
    int browser_plugin_instance_id,
    BrowserPlugin* browser_plugin) {
  instances_.AddWithID(browser_plugin, browser_plugin_instance_id);
}

void BrowserPluginManager::RemoveBrowserPlugin(int browser_plugin_instance_id) {
  instances_.Remove(browser_plugin_instance_id);
}

BrowserPlugin* BrowserPluginManager::GetBrowserPlugin(
    int browser_plugin_instance_id) const {
  return instances_.Lookup(browser_plugin_instance_id);
}

int BrowserPluginManager::GetNextInstanceID() {
  return RenderThreadImpl::current()->GenerateRoutingID();
}

void BrowserPluginManager::UpdateDeviceScaleFactor() {
  IDMap<BrowserPlugin>::iterator iter(&instances_);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->UpdateDeviceScaleFactor();
    iter.Advance();
  }
}

void BrowserPluginManager::UpdateFocusState() {
  IDMap<BrowserPlugin>::iterator iter(&instances_);
  while (!iter.IsAtEnd()) {
    iter.GetCurrentValue()->UpdateGuestFocusState(blink::WebFocusTypeNone);
    iter.Advance();
  }
}

void BrowserPluginManager::Attach(int browser_plugin_instance_id) {
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  if (plugin)
    plugin->Attach();
}

void BrowserPluginManager::Detach(int browser_plugin_instance_id) {
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  if (plugin)
    plugin->Detach();
}

BrowserPlugin* BrowserPluginManager::CreateBrowserPlugin(
    RenderFrame* render_frame,
    scoped_ptr<BrowserPluginDelegate> delegate) {
  return new BrowserPlugin(render_frame, delegate.Pass());
}

void BrowserPluginManager::DidCommitCompositorFrame(
    int render_view_routing_id) {
  IDMap<BrowserPlugin>::iterator iter(&instances_);
  while (!iter.IsAtEnd()) {
    if (iter.GetCurrentValue()->render_view_routing_id() ==
        render_view_routing_id) {
      iter.GetCurrentValue()->DidCommitCompositorFrame();
    }
    iter.Advance();
  }
}

bool BrowserPluginManager::OnControlMessageReceived(
    const IPC::Message& message) {
  if (!BrowserPlugin::ShouldForwardToBrowserPlugin(message) &&
      !content::GetContentClient()->renderer()->
          ShouldForwardToGuestContainer(message)) {
    return false;
  }

  int browser_plugin_instance_id = browser_plugin::kInstanceIDNone;
  // All allowed messages must have |browser_plugin_instance_id| as their
  // first parameter.
  PickleIterator iter(message);
  bool success = iter.ReadInt(&browser_plugin_instance_id);
  DCHECK(success);
  BrowserPlugin* plugin = GetBrowserPlugin(browser_plugin_instance_id);
  if (plugin && plugin->OnMessageReceived(message))
    return true;

  // TODO(fsamuel): This is probably forcing the compositor to continue working
  // even on display:none. We should optimize this.
  if (message.type() == BrowserPluginMsg_CompositorFrameSwapped::ID) {
    OnCompositorFrameSwappedPluginUnavailable(message);
    return true;
  }

  return false;
}

bool BrowserPluginManager::Send(IPC::Message* msg) {
  return RenderThreadImpl::current()->Send(msg);
}

void BrowserPluginManager::OnCompositorFrameSwappedPluginUnavailable(
    const IPC::Message& message) {
  BrowserPluginMsg_CompositorFrameSwapped::Param param;
  if (!BrowserPluginMsg_CompositorFrameSwapped::Read(&message, &param))
    return;

  FrameHostMsg_CompositorFrameSwappedACK_Params params;
  params.producing_host_id = get<1>(param).producing_host_id;
  params.producing_route_id = get<1>(param).producing_route_id;
  params.output_surface_id = get<1>(param).output_surface_id;
  Send(new BrowserPluginHostMsg_CompositorFrameSwappedACK(
      message.routing_id(), get<0>(param), params));
}

}  // namespace content
