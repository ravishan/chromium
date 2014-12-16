// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/touch/touch_device.h"
#include "base/logging.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/x11/touch_factory_x11.h"

namespace ui {

bool IsTouchDevicePresent() {
  return ui::TouchFactory::GetInstance()->IsTouchDevicePresent();
}

int MaxTouchPoints() {
  return ui::TouchFactory::GetInstance()->GetMaxTouchPoints();
}

// FIXME: Use mouse detection logic. crbug.com/440503
int GetAvailablePointerTypes() {
  int available_pointer_types = 0;

  if (ui::DeviceDataManager::GetInstance()->keyboard_devices().size() > 0)
      available_pointer_types |= POINTER_TYPE_NONE;

  // Assume either a touch-device or a mouse is there
  if (IsTouchDevicePresent())
    available_pointer_types |= POINTER_TYPE_COARSE;
  else
    available_pointer_types |= POINTER_TYPE_FINE;

  DCHECK(available_pointer_types);
  return available_pointer_types;
}

PointerType GetPrimaryPointerType() {
  int available_pointer_types = GetAvailablePointerTypes();
  if (available_pointer_types & POINTER_TYPE_COARSE)
    return POINTER_TYPE_COARSE;
  if (available_pointer_types & POINTER_TYPE_FINE)
    return POINTER_TYPE_FINE;
  DCHECK(available_pointer_types & POINTER_TYPE_NONE);
  return POINTER_TYPE_NONE;
}

// FIXME: Use mouse detection logic. crbug.com/440503
int GetAvailableHoverTypes() {
  int available_hover_types = 0;

  if (ui::DeviceDataManager::GetInstance()->keyboard_devices().size() > 0)
      available_hover_types |= HOVER_TYPE_NONE;

  // Assume either a touch-device or a mouse is there
  if (IsTouchDevicePresent())
    available_hover_types |= HOVER_TYPE_ON_DEMAND;
  else
    available_hover_types |= HOVER_TYPE_HOVER;

  DCHECK(available_hover_types);
  return available_hover_types;
}

HoverType GetPrimaryHoverType() {
  int available_hover_types = GetAvailableHoverTypes();
  if (available_hover_types & HOVER_TYPE_ON_DEMAND)
    return HOVER_TYPE_ON_DEMAND;
  if (available_hover_types & HOVER_TYPE_HOVER)
    return HOVER_TYPE_HOVER;
  DCHECK(available_hover_types & HOVER_TYPE_NONE);
  return HOVER_TYPE_NONE;
}

}  // namespace ui
