// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_message_port_client.h"

#include "android_webview/common/aw_message_port_messages.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/v8_value_converter.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebSerializedScriptValue.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"

using blink::WebSerializedScriptValue;
using content::V8ValueConverter;
using std::vector;

namespace android_webview {

AwMessagePortClient::AwMessagePortClient(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
}

AwMessagePortClient::~AwMessagePortClient() {
}

bool AwMessagePortClient::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwMessagePortClient, message)
    IPC_MESSAGE_HANDLER(AwMessagePortMsg_Message, OnPostMessage)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AwMessagePortClient::OnPostMessage(
    int message_port_id,
    const base::string16& message,
    const vector<int>& sent_message_port_ids) {
  v8::HandleScope handle_scope(blink::mainThreadIsolate());
  blink::WebFrame* main_frame =
      render_frame()->GetRenderView()->GetWebView()->mainFrame();
  if (main_frame == nullptr) {
    return;
  }
  v8::Local<v8::Context> context = main_frame->mainWorldScriptContext();
  v8::Context::Scope context_scope(context);
  DCHECK(!context.IsEmpty());
  WebSerializedScriptValue v = WebSerializedScriptValue::fromString(message);
  v8::Handle<v8::Value> v8value = v.deserialize();

  scoped_ptr<V8ValueConverter> converter;
  converter.reset(V8ValueConverter::create());
  converter->SetDateAllowed(true);
  converter->SetRegExpAllowed(true);
  base::ListValue result;
  result.Append(converter->FromV8Value(v8value, context));
  Send(new AwMessagePortHostMsg_ConvertedMessage(render_frame()->GetRoutingID(),
                                                 message_port_id,
                                                 result,
                                                 sent_message_port_ids));
}

}
