// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/dri/gbm_buffer.h"

#include <drm.h>
#include <fcntl.h>
#include <gbm.h>
#include <xf86drm.h>

#include "base/logging.h"
#include "ui/ozone/platform/dri/gbm_wrapper.h"

namespace ui {

namespace {

int GetGbmFormatFromBufferFormat(SurfaceFactoryOzone::BufferFormat fmt) {
  switch (fmt) {
    case SurfaceFactoryOzone::RGBA_8888:
      return GBM_BO_FORMAT_ARGB8888;
    case SurfaceFactoryOzone::RGBX_8888:
      return GBM_BO_FORMAT_XRGB8888;
    default:
      NOTREACHED();
      return 0;
  }
}

}  // namespace

GbmBuffer::GbmBuffer(GbmWrapper* gbm, gbm_bo* bo, bool scanout)
    : GbmBufferBase(gbm, bo, scanout) {
}

GbmBuffer::~GbmBuffer() {
  if (bo())
    gbm_bo_destroy(bo());
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBuffer(
    GbmWrapper* gbm,
    SurfaceFactoryOzone::BufferFormat format,
    const gfx::Size& size,
    bool scanout) {
  unsigned flags = GBM_BO_USE_RENDERING;
  if (scanout)
    flags |= GBM_BO_USE_SCANOUT;
  gbm_bo* bo = gbm_bo_create(gbm->device(), size.width(), size.height(),
                             GetGbmFormatFromBufferFormat(format), flags);
  if (!bo)
    return NULL;

  scoped_refptr<GbmBuffer> buffer(new GbmBuffer(gbm, bo, scanout));
  if (scanout && !buffer->GetFramebufferId())
    return NULL;

  return buffer;
}

GbmPixmap::GbmPixmap(scoped_refptr<GbmBuffer> buffer)
    : buffer_(buffer), dma_buf_(-1) {
}

bool GbmPixmap::Initialize(GbmWrapper* gbm) {
  // We want to use the GBM API because it's going to call into libdrm
  // which might do some optimizations on buffer allocation,
  // especially when sharing buffers via DMABUF.
  dma_buf_ = gbm_bo_get_fd(buffer_->bo());
  if (dma_buf_ < 0) {
    LOG(ERROR) << "Failed to export buffer to dma_buf";
    return false;
  }
  return true;
}

GbmPixmap::~GbmPixmap() {
  if (dma_buf_ > 0)
    close(dma_buf_);
}

void* GbmPixmap::GetEGLClientBuffer() {
  return nullptr;
}

int GbmPixmap::GetDmaBufFd() {
  return dma_buf_;
}

int GbmPixmap::GetDmaBufPitch() {
  return gbm_bo_get_stride(buffer_->bo());
}

}  // namespace ui
