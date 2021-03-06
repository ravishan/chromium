// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/ios/ios_serialized_navigation_driver.h"

#include "base/memory/singleton.h"
#include "components/sessions/serialized_navigation_entry.h"
#include "ios/web/public/referrer.h"

namespace sessions {

// static
SerializedNavigationDriver* SerializedNavigationDriver::Get() {
  return IOSSerializedNavigationDriver::GetInstance();
}

// static
IOSSerializedNavigationDriver*
IOSSerializedNavigationDriver::GetInstance() {
  return Singleton<IOSSerializedNavigationDriver,
      LeakySingletonTraits<IOSSerializedNavigationDriver>>::get();
}

IOSSerializedNavigationDriver::IOSSerializedNavigationDriver() {
}

IOSSerializedNavigationDriver::~IOSSerializedNavigationDriver() {
}

int IOSSerializedNavigationDriver::GetDefaultReferrerPolicy() const {
  return web::ReferrerPolicyDefault;
}

std::string
IOSSerializedNavigationDriver::GetSanitizedPageStateForPickle(
    const SerializedNavigationEntry* navigation) const {
  return std::string();
}

void IOSSerializedNavigationDriver::Sanitize(
    SerializedNavigationEntry* navigation) const {
  web::Referrer referrer(
      navigation->referrer_url_,
      static_cast<web::ReferrerPolicy>(navigation->referrer_policy_));

  if (!navigation->virtual_url_.SchemeIsHTTPOrHTTPS() ||
      !referrer.url.SchemeIsHTTPOrHTTPS()) {
    referrer.url = GURL();
  } else {
    switch (referrer.policy) {
      case web::ReferrerPolicyDefault:
        if (referrer.url.SchemeIsSecure() &&
            !navigation->virtual_url_.SchemeIsSecure()) {
          referrer.url = GURL();
        }
        break;
      case web::ReferrerPolicyAlways:
        break;
      case web::ReferrerPolicyNever:
        referrer.url = GURL();
        break;
      case web::ReferrerPolicyOrigin:
        referrer.url = referrer.url.GetOrigin();
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  // Reset the referrer if it has changed.
  if (navigation->referrer_url_ != referrer.url) {
    navigation->referrer_url_ = GURL();
    navigation->referrer_policy_ = GetDefaultReferrerPolicy();
  }
}

}  // namespace sessions
