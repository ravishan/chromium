// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_unregister_job.h"

#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_job_coordinator.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_storage.h"

namespace content {

typedef ServiceWorkerRegisterJobBase::RegistrationJobType RegistrationJobType;

ServiceWorkerUnregisterJob::ServiceWorkerUnregisterJob(
    base::WeakPtr<ServiceWorkerContextCore> context,
    const GURL& pattern)
    : context_(context),
      pattern_(pattern),
      weak_factory_(this) {}

ServiceWorkerUnregisterJob::~ServiceWorkerUnregisterJob() {}

void ServiceWorkerUnregisterJob::AddCallback(
    const UnregistrationCallback& callback) {
  callbacks_.push_back(callback);
}

void ServiceWorkerUnregisterJob::Start() {
  context_->storage()->FindRegistrationForPattern(
      pattern_,
      base::Bind(&ServiceWorkerUnregisterJob::DeleteExistingRegistration,
                 weak_factory_.GetWeakPtr()));
}

bool ServiceWorkerUnregisterJob::Equals(ServiceWorkerRegisterJobBase* job) {
  if (job->GetType() != GetType())
    return false;
  return static_cast<ServiceWorkerUnregisterJob*>(job)->pattern_ == pattern_;
}

RegistrationJobType ServiceWorkerUnregisterJob::GetType() {
  return ServiceWorkerRegisterJobBase::UNREGISTRATION;
}

void ServiceWorkerUnregisterJob::DeleteExistingRegistration(
    ServiceWorkerStatusCode status,
    const scoped_refptr<ServiceWorkerRegistration>& registration) {
  if (status == SERVICE_WORKER_OK) {
    context_->storage()->DeleteRegistration(
        pattern_,
        base::Bind(&ServiceWorkerUnregisterJob::Complete,
                   weak_factory_.GetWeakPtr()));
    return;
  }

  if (status == SERVICE_WORKER_ERROR_NOT_FOUND) {
    // The previous registration does not exist, which is ok.
    Complete(SERVICE_WORKER_OK);
    return;
  }

  Complete(status);
}

void ServiceWorkerUnregisterJob::Complete(ServiceWorkerStatusCode status) {
  for (std::vector<UnregistrationCallback>::iterator it = callbacks_.begin();
       it != callbacks_.end();
       ++it) {
    it->Run(status);
  }
  context_->job_coordinator()->FinishJob(pattern_, this);
}

}  // namespace content
