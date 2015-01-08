// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SERVICE_H_
#define CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "base/prefs/pref_change_registrar.h"
#include "base/scoped_observer.h"
#include "base/strings/string16.h"
#include "chrome/browser/supervised_user/experimental/supervised_user_blacklist.h"
#include "chrome/browser/supervised_user/supervised_user_url_filter.h"
#include "chrome/browser/supervised_user/supervised_users.h"
#include "chrome/browser/sync/profile_sync_service_observer.h"
#include "chrome/browser/sync/sync_type_preference_provider.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "components/keyed_service/core/keyed_service.h"

#if defined(ENABLE_EXTENSIONS)
#include "extensions/browser/management_policy.h"
#endif

class Browser;
class GoogleServiceAuthError;
class PermissionRequestCreator;
class Profile;
class SupervisedUserBlacklistDownloader;
class SupervisedUserRegistrationUtility;
class SupervisedUserServiceObserver;
class SupervisedUserSettingsService;
class SupervisedUserSiteList;
class SupervisedUserURLFilter;
class SupervisedUserWhitelistService;

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace extensions {
class ExtensionRegistry;
}

namespace net {
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

// This class handles all the information related to a given supervised profile
// (e.g. the installed content packs, the default URL filtering behavior, or
// manual whitelist/blacklist overrides).
class SupervisedUserService : public KeyedService,
#if defined(ENABLE_EXTENSIONS)
                              public extensions::ManagementPolicy::Provider,
#endif
                              public SyncTypePreferenceProvider,
                              public ProfileSyncServiceObserver,
                              public chrome::BrowserListObserver,
                              public SupervisedUserURLFilter::Observer {
 public:
  typedef base::Callback<void(content::WebContents*)> NavigationBlockedCallback;
  typedef base::Callback<void(const GoogleServiceAuthError&)> AuthErrorCallback;
  typedef base::Callback<void(bool)> SuccessCallback;

  class Delegate {
   public:
    virtual ~Delegate() {}
    // Returns true to indicate that the delegate handled the (de)activation, or
    // false to indicate that the SupervisedUserService itself should handle it.
    virtual bool SetActive(bool active) = 0;
    // Returns the path to a blacklist file to load, or an empty path to
    // indicate "none".
    virtual base::FilePath GetBlacklistPath() const;
    // Returns the URL from which to download a blacklist if no local one exists
    // yet. The blacklist file will be stored at |GetBlacklistPath()|.
    virtual GURL GetBlacklistURL() const;
    // Returns the identifier ("cx") of the Custom Search Engine to use for the
    // experimental "SafeSites" feature, or the empty string to disable the
    // feature.
    virtual std::string GetSafeSitesCx() const;
  };

  ~SupervisedUserService() override;

  // ProfileKeyedService override:
  void Shutdown() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void SetDelegate(Delegate* delegate);

  // Returns the URL filter for the IO thread, for filtering network requests
  // (in SupervisedUserResourceThrottle).
  scoped_refptr<const SupervisedUserURLFilter> GetURLFilterForIOThread();

  // Returns the URL filter for the UI thread, for filtering navigations and
  // classifying sites in the history view.
  SupervisedUserURLFilter* GetURLFilterForUIThread();

  // Returns the whitelist service.
  SupervisedUserWhitelistService* GetWhitelistService();

  // Whether the user can request access to blocked URLs.
  bool AccessRequestsEnabled();

  // Adds an access request for the given URL. The requests are stored using
  // a prefix followed by a URIEncoded version of the URL. Each entry contains
  // a dictionary which currently has the timestamp of the request in it.
  void AddAccessRequest(const GURL& url, const SuccessCallback& callback);

  // Returns the email address of the custodian.
  std::string GetCustodianEmailAddress() const;

  // Returns the name of the custodian, or the email address if the name is
  // empty.
  std::string GetCustodianName() const;

  // Returns the email address of the second custodian, or the empty string
  // if there is no second custodian.
  std::string GetSecondCustodianEmailAddress() const;

  // Returns the name of the second custodian, or the email address if the name
  // is empty, or the empty string is there is no second custodian.
  std::string GetSecondCustodianName() const;

  // Initializes this object. This method does nothing if the profile is not
  // supervised.
  void Init();

  // Initializes this profile for syncing, using the provided |refresh_token| to
  // mint access tokens for Sync.
  void InitSync(const std::string& refresh_token);

  // Convenience method that registers this supervised user using
  // |registration_utility| and initializes sync with the returned token.
  // The |callback| will be called when registration is complete,
  // whether it succeeded or not -- unless registration was cancelled manually,
  // in which case the callback will be ignored.
  void RegisterAndInitSync(
      SupervisedUserRegistrationUtility* registration_utility,
      Profile* custodian_profile,
      const std::string& supervised_user_id,
      const AuthErrorCallback& callback);

  void AddNavigationBlockedCallback(const NavigationBlockedCallback& callback);
  void DidBlockNavigation(content::WebContents* web_contents);

  void AddObserver(SupervisedUserServiceObserver* observer);
  void RemoveObserver(SupervisedUserServiceObserver* observer);

  void AddPermissionRequestCreator(
      scoped_ptr<PermissionRequestCreator> creator);

#if defined(ENABLE_EXTENSIONS)
  // extensions::ManagementPolicy::Provider implementation:
  std::string GetDebugPolicyProviderName() const override;
  bool UserMayLoad(const extensions::Extension* extension,
                   base::string16* error) const override;
  bool UserMayModifySettings(const extensions::Extension* extension,
                             base::string16* error) const override;
#endif

  // SyncTypePreferenceProvider implementation:
  syncer::ModelTypeSet GetPreferredDataTypes() const override;

  // ProfileSyncServiceObserver implementation:
  void OnStateChanged() override;

  // chrome::BrowserListObserver implementation:
  void OnBrowserSetLastActive(Browser* browser) override;

  // SupervisedUserURLFilter::Observer implementation:
  void OnSiteListUpdated() override;

 private:
  friend class SupervisedUserServiceExtensionTestBase;
  friend class SupervisedUserServiceFactory;
  FRIEND_TEST_ALL_PREFIXES(SingleClientSupervisedUserSettingsSyncTest, Sanity);
  FRIEND_TEST_ALL_PREFIXES(SupervisedUserServiceTest, ClearOmitOnRegistration);
  FRIEND_TEST_ALL_PREFIXES(SupervisedUserServiceTest,
                           ChangesIncludedSessionOnChangedSettings);
  FRIEND_TEST_ALL_PREFIXES(SupervisedUserServiceTest,
                           ChangesSyncSessionStateOnChangedSettings);

  // A bridge from the UI thread to the SupervisedUserURLFilters, one of which
  // lives on the IO thread. This class mediates access to them and makes sure
  // they are kept in sync.
  class URLFilterContext {
   public:
    URLFilterContext();
    ~URLFilterContext();

    SupervisedUserURLFilter* ui_url_filter() const;
    SupervisedUserURLFilter* io_url_filter() const;

    void SetDefaultFilteringBehavior(
        SupervisedUserURLFilter::FilteringBehavior behavior);
    void LoadWhitelists(
        const std::vector<scoped_refptr<SupervisedUserSiteList>>& site_lists);
    void LoadBlacklist(const base::FilePath& path,
                       const base::Closure& callback);
    void SetManualHosts(scoped_ptr<std::map<std::string, bool>> host_map);
    void SetManualURLs(scoped_ptr<std::map<GURL, bool>> url_map);

    void InitAsyncURLChecker(net::URLRequestContextGetter* context,
                             const std::string& cx);

   private:
    void OnBlacklistLoaded(const base::Closure& callback);

    // SupervisedUserURLFilter is refcounted because the IO thread filter is
    // used both by ProfileImplIOData and OffTheRecordProfileIOData (to filter
    // network requests), so they both keep a reference to it.
    // Clients should not keep references to the UI thread filter, however
    // (the filter will live as long as the profile lives, and afterwards it
    // should not be used anymore either).
    scoped_refptr<SupervisedUserURLFilter> ui_url_filter_;
    scoped_refptr<SupervisedUserURLFilter> io_url_filter_;

    SupervisedUserBlacklist blacklist_;

    DISALLOW_COPY_AND_ASSIGN(URLFilterContext);
  };

  // Use |SupervisedUserServiceFactory::GetForProfile(..)| to get
  // an instance of this service.
  explicit SupervisedUserService(Profile* profile);

  void SetActive(bool active);

  void OnCustodianProfileDownloaded(const base::string16& full_name);

  void OnSupervisedUserRegistered(const AuthErrorCallback& callback,
                                  Profile* custodian_profile,
                                  const GoogleServiceAuthError& auth_error,
                                  const std::string& token);

  void SetupSync();
  void StartSetupSync();
  void FinishSetupSyncWhenReady();
  void FinishSetupSync();

  bool ProfileIsSupervised() const;

  void OnCustodianInfoChanged();

#if defined(ENABLE_EXTENSIONS)
  // Internal implementation for ExtensionManagementPolicy::Delegate methods.
  // If |error| is not NULL, it will be filled with an error message if the
  // requested extension action (install, modify status, etc.) is not permitted.
  bool ExtensionManagementPolicyImpl(const extensions::Extension* extension,
                                     base::string16* error) const;

  // Extensions helper to SetActive().
  void SetExtensionsActive();
#endif

  SupervisedUserSettingsService* GetSettingsService();

  size_t FindEnabledPermissionRequestCreator(size_t start);
  void AddAccessRequestInternal(const GURL& url,
                                const SuccessCallback& callback,
                                size_t index);
  void OnPermissionRequestIssued(const GURL& url,
                                 const SuccessCallback& callback,
                                 size_t index,
                                 bool success);

  void OnSupervisedUserIdChanged();

  void OnDefaultFilteringBehaviorChanged();

  void OnSiteListsChanged(
      const std::vector<scoped_refptr<SupervisedUserSiteList>>& site_lists);

  // Asynchronously downloads a static blacklist file from |url|, stores it at
  // |path|, loads it, and applies it to the URL filters. If |url| is not valid
  // (e.g. empty), directly tries to load from |path|.
  void LoadBlacklist(const base::FilePath& path, const GURL& url);

  // Asynchronously loads a static blacklist from a binary file at |path| and
  // applies it to the URL filters.
  void LoadBlacklistFromFile(const base::FilePath& path);

  void OnBlacklistDownloadDone(const base::FilePath& path, bool success);

  void OnBlacklistLoaded();

  // Updates the manual overrides for hosts in the URL filters when the
  // corresponding preference is changed.
  void UpdateManualHosts();

  // Updates the manual overrides for URLs in the URL filters when the
  // corresponding preference is changed.
  void UpdateManualURLs();

  // Returns the human readable name of the supervised user.
  std::string GetSupervisedUserName() const;

  // Subscribes to the SupervisedUserPrefStore, refreshes
  // |includes_sync_sessions_type_| and triggers reconfiguring the
  // ProfileSyncService.
  void OnHistoryRecordingStateChanged();

  // Returns true if the syncer::SESSIONS type should be included in Sync.
  bool IncludesSyncSessionsType() const;

  // The option a custodian sets to either record or prevent recording the
  // supervised user's history. Set by |FetchNewSessionSyncState()| and
  // defaults to true.
  bool includes_sync_sessions_type_;

  // Owns us via the KeyedService mechanism.
  Profile* profile_;

  bool active_;

  Delegate* delegate_;

  PrefChangeRegistrar pref_change_registrar_;

  // True iff we're waiting for the Sync service to be initialized.
  bool waiting_for_sync_initialization_;
  bool is_profile_active_;

  std::vector<NavigationBlockedCallback> navigation_blocked_callbacks_;

  // True only when |Init()| method has been called.
  bool did_init_;

  // True only when |Shutdown()| method has been called.
  bool did_shutdown_;

  URLFilterContext url_filter_context_;
  scoped_ptr<SupervisedUserBlacklistDownloader> blacklist_downloader_;

  scoped_ptr<SupervisedUserWhitelistService> whitelist_service_;

  // Used to create permission requests.
  ScopedVector<PermissionRequestCreator> permissions_creators_;

  ObserverList<SupervisedUserServiceObserver> observer_list_;

  base::WeakPtrFactory<SupervisedUserService> weak_ptr_factory_;
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_SUPERVISED_USER_SERVICE_H_
