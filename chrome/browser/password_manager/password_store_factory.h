// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_

#include "base/basictypes.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/core/service_access_type.h"

class Profile;

namespace password_manager {
class PasswordStore;
}

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS) && defined(OS_POSIX)
// Local profile ids are used to associate resources stored outside the profile
// directory, like saved passwords in GNOME Keyring / KWallet, with a profile.
// With high probability, they are unique on the local machine. They are almost
// certainly not unique globally, by design. Do not send them over the network.
typedef int LocalProfileId;
#endif

// A wrapper of PasswordStore so we can use it as a profiled keyed service.
class PasswordStoreService : public KeyedService {
 public:
  // |password_store| needs to be not-NULL, and the constructor expects that
  // Init() was already called successfully on it.
  explicit PasswordStoreService(
      scoped_refptr<password_manager::PasswordStore> password_store);
  ~PasswordStoreService() override;

  scoped_refptr<password_manager::PasswordStore> GetPasswordStore();

  // KeyedService implementation.
  void Shutdown() override;

 private:
  scoped_refptr<password_manager::PasswordStore> password_store_;
  DISALLOW_COPY_AND_ASSIGN(PasswordStoreService);
};

// Singleton that owns all PasswordStores and associates them with
// Profiles.
class PasswordStoreFactory : public BrowserContextKeyedServiceFactory {
 public:
  static scoped_refptr<password_manager::PasswordStore> GetForProfile(
      Profile* profile,
      ServiceAccessType set);

  static PasswordStoreFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<PasswordStoreFactory>;

  PasswordStoreFactory();
  ~PasswordStoreFactory() override;

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS) && defined(OS_POSIX)
  LocalProfileId GetLocalProfileId(PrefService* prefs) const;
#endif

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreFactory);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_
