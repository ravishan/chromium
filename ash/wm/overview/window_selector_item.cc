// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/window_selector_item.h"

#include <algorithm>
#include <vector>

#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/wm/overview/overview_animation_type.h"
#include "ash/wm/overview/overview_window_button.h"
#include "ash/wm/overview/scoped_overview_animation_settings.h"
#include "ash/wm/overview/scoped_transform_overview_window.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "base/auto_reset.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "grit/ash_resources.h"
#include "grit/ash_strings.h"
#include "ui/aura/window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/transform_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace ash {

namespace {

// In the conceptual overview table, the window margin is the space reserved
// around the window within the cell. This margin does not overlap so the
// closest distance between adjacent windows will be twice this amount.
static const int kWindowMargin = 30;

// Opacity for dimmed items.
static const float kDimmedItemOpacity = 0.5f;

// Calculates the |window| bounds after being transformed to the selector's
// space. The returned Rect is in virtual screen coordinates.
gfx::Rect GetTransformedBounds(aura::Window* window) {
  gfx::RectF bounds(ScreenUtil::ConvertRectToScreen(window->GetRootWindow(),
      window->layer()->GetTargetBounds()));
  gfx::Transform new_transform = TransformAboutPivot(
      gfx::Point(bounds.x(), bounds.y()),
      window->layer()->GetTargetTransform());
  new_transform.TransformRect(&bounds);
  return ToEnclosingRect(bounds);
}

// An image button with a close window icon.
class OverviewCloseButton : public views::ImageButton {
 public:
  explicit OverviewCloseButton(views::ButtonListener* listener);
  ~OverviewCloseButton() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OverviewCloseButton);
};

OverviewCloseButton::OverviewCloseButton(views::ButtonListener* listener)
    : views::ImageButton(listener) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  SetImage(views::CustomButton::STATE_NORMAL,
           rb.GetImageSkiaNamed(IDR_AURA_WINDOW_OVERVIEW_CLOSE));
  SetImage(views::CustomButton::STATE_HOVERED,
           rb.GetImageSkiaNamed(IDR_AURA_WINDOW_OVERVIEW_CLOSE_H));
  SetImage(views::CustomButton::STATE_PRESSED,
           rb.GetImageSkiaNamed(IDR_AURA_WINDOW_OVERVIEW_CLOSE_P));
}

OverviewCloseButton::~OverviewCloseButton() {
}

}  // namespace

WindowSelectorItem::WindowSelectorItem(aura::Window* window)
    : dimmed_(false),
      root_window_(window->GetRootWindow()),
      transform_window_(window),
      in_bounds_update_(false),
      close_button_(new OverviewCloseButton(this)),
      overview_window_button_(new OverviewWindowButton(window)) {
  views::Widget::InitParams params;
  params.type = views::Widget::InitParams::TYPE_POPUP;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.parent = Shell::GetContainer(root_window_,
                                      kShellWindowId_OverlayContainer);
  close_button_widget_.set_focus_on_creation(false);
  close_button_widget_.Init(params);
  close_button_->SetVisible(false);
  close_button_widget_.SetContentsView(close_button_);
  close_button_widget_.SetSize(close_button_->GetPreferredSize());
  close_button_widget_.Show();

  gfx::Rect close_button_rect(close_button_widget_.GetNativeWindow()->bounds());
  // Align the center of the button with position (0, 0) so that the
  // translate transform does not need to take the button dimensions into
  // account.
  close_button_rect.set_x(-close_button_rect.width() / 2);
  close_button_rect.set_y(-close_button_rect.height() / 2);
  close_button_widget_.GetNativeWindow()->SetBounds(close_button_rect);

  GetWindow()->AddObserver(this);

  UpdateCloseButtonAccessibilityName();
}

WindowSelectorItem::~WindowSelectorItem() {
  GetWindow()->RemoveObserver(this);
}

aura::Window* WindowSelectorItem::GetWindow() {
  return transform_window_.window();
}

void WindowSelectorItem::RestoreWindow() {
  transform_window_.RestoreWindow();
}

void WindowSelectorItem::ShowWindowOnExit() {
  transform_window_.ShowWindowOnExit();
}

void WindowSelectorItem::PrepareForOverview() {
  transform_window_.PrepareForOverview();
}

bool WindowSelectorItem::Contains(const aura::Window* target) const {
  return transform_window_.Contains(target);
}

void WindowSelectorItem::SetBounds(const gfx::Rect& target_bounds,
                                   OverviewAnimationType animation_type) {
  if (in_bounds_update_)
    return;
  base::AutoReset<bool> auto_reset_in_bounds_update(&in_bounds_update_, true);
  target_bounds_ = target_bounds;

  overview_window_button_->SetBounds(target_bounds, animation_type);

  gfx::Rect inset_bounds(target_bounds);
  inset_bounds.Inset(kWindowMargin, kWindowMargin);
  SetItemBounds(inset_bounds, animation_type);

  // SetItemBounds is called before UpdateCloseButtonLayout so the close button
  // can properly use the updated windows bounds.
  UpdateCloseButtonLayout(animation_type);
}

void WindowSelectorItem::RecomputeWindowTransforms() {
  if (in_bounds_update_ || target_bounds_.IsEmpty())
    return;
  base::AutoReset<bool> auto_reset_in_bounds_update(&in_bounds_update_, true);
  gfx::Rect inset_bounds(target_bounds_);
  inset_bounds.Inset(kWindowMargin, kWindowMargin);
  SetItemBounds(inset_bounds, OverviewAnimationType::OVERVIEW_ANIMATION_NONE);
  UpdateCloseButtonLayout(OverviewAnimationType::OVERVIEW_ANIMATION_NONE);
}

void WindowSelectorItem::SendFocusAlert() const {
  overview_window_button_->SendFocusAlert();
}

void WindowSelectorItem::SetDimmed(bool dimmed) {
  dimmed_ = dimmed;
  SetOpacity(dimmed ? kDimmedItemOpacity : 1.0f);
}

void WindowSelectorItem::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  transform_window_.Close();
}

void WindowSelectorItem::OnWindowDestroying(aura::Window* window) {
  window->RemoveObserver(this);
  transform_window_.OnWindowDestroyed();
}

void WindowSelectorItem::OnWindowTitleChanged(aura::Window* window) {
  // TODO(flackr): Maybe add the new title to a vector of titles so that we can
  // filter any of the titles the window had while in the overview session.
  overview_window_button_->SetLabelText(window->title());
  UpdateCloseButtonAccessibilityName();
}

void WindowSelectorItem::SetItemBounds(const gfx::Rect& target_bounds,
                                       OverviewAnimationType animation_type) {
  DCHECK(root_window_ == GetWindow()->GetRootWindow());
  gfx::Rect screen_bounds = transform_window_.GetTargetBoundsInScreen();
  gfx::Rect selector_item_bounds =
      ScopedTransformOverviewWindow::ShrinkRectToFitPreservingAspectRatio(
          screen_bounds, target_bounds);
  gfx::Transform transform =
      ScopedTransformOverviewWindow::GetTransformForRect(screen_bounds,
          selector_item_bounds);
  ScopedTransformOverviewWindow::ScopedAnimationSettings animation_settings;
  transform_window_.BeginScopedAnimation(animation_type, &animation_settings);
  transform_window_.SetTransform(root_window_, transform);
  transform_window_.set_overview_transform(transform);
}

void WindowSelectorItem::SetOpacity(float opacity) {
  overview_window_button_->SetOpacity(opacity);
  close_button_widget_.GetNativeWindow()->layer()->SetOpacity(opacity);

  transform_window_.SetOpacity(opacity);
}

void WindowSelectorItem::UpdateCloseButtonLayout(
    OverviewAnimationType animation_type) {
  if (!close_button_->visible()) {
    close_button_->SetVisible(true);
    ScopedOverviewAnimationSettings::SetupFadeInAfterLayout(
        close_button_widget_.GetNativeWindow());
  }
  ScopedOverviewAnimationSettings animation_settings(animation_type,
      close_button_widget_.GetNativeWindow());

  gfx::Rect transformed_window_bounds = ScreenUtil::ConvertRectFromScreen(
      close_button_widget_.GetNativeWindow()->GetRootWindow(),
      GetTransformedBounds(GetWindow()));

  gfx::Transform close_button_transform;
  close_button_transform.Translate(transformed_window_bounds.right(),
                                   transformed_window_bounds.y());
  close_button_widget_.GetNativeWindow()->SetTransform(
      close_button_transform);
}

void WindowSelectorItem::UpdateCloseButtonAccessibilityName() {
  close_button_->SetAccessibleName(l10n_util::GetStringFUTF16(
      IDS_ASH_OVERVIEW_CLOSE_ITEM_BUTTON_ACCESSIBLE_NAME,
      GetWindow()->title()));
}

}  // namespace ash
