# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      # GN version: //components/history/core/browser
      'target_name': 'history_core_browser',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../net/net.gyp:net',
        '../sql/sql.gyp:sql',
        '../third_party/sqlite/sqlite.gyp:sqlite',
        '../ui/base/ui_base.gyp:ui_base',
        '../ui/gfx/gfx.gyp:gfx',
        '../url/url.gyp:url_lib',
        'favicon_base',
        'history_core_browser_proto',
        'keyed_service_core',
        'query_parser',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'history/core/browser/download_constants.h',
        'history/core/browser/download_database.cc',
        'history/core/browser/download_database.h',
        'history/core/browser/download_row.cc',
        'history/core/browser/download_row.h',
        'history/core/browser/download_types.cc',
        'history/core/browser/download_types.h',
        'history/core/browser/expire_history_backend.cc',
        'history/core/browser/expire_history_backend.h',
        'history/core/browser/history_backend_notifier.h',
        'history/core/browser/history_backend_observer.h',
        'history/core/browser/history_client.cc',
        'history/core/browser/history_client.h',
        'history/core/browser/history_constants.cc',
        'history/core/browser/history_constants.h',
        'history/core/browser/history_context.h',
        'history/core/browser/history_database.cc',
        'history/core/browser/history_database.h',
        'history/core/browser/history_database_params.cc',
        'history/core/browser/history_database_params.h',
        'history/core/browser/history_db_task.h',
        'history/core/browser/history_match.cc',
        'history/core/browser/history_match.h',
        'history/core/browser/history_service_observer.h',
        'history/core/browser/history_types.cc',
        'history/core/browser/history_types.h',
        'history/core/browser/in_memory_database.cc',
        'history/core/browser/in_memory_database.h',
        'history/core/browser/in_memory_url_index_types.cc',
        'history/core/browser/in_memory_url_index_types.h',
        'history/core/browser/keyword_id.h',
        'history/core/browser/keyword_search_term.cc',
        'history/core/browser/keyword_search_term.h',
        'history/core/browser/page_usage_data.cc',
        'history/core/browser/page_usage_data.h',
        'history/core/browser/thumbnail_database.cc',
        'history/core/browser/thumbnail_database.h',
        'history/core/browser/top_sites_cache.cc',
        'history/core/browser/top_sites_cache.h',
        'history/core/browser/top_sites_observer.h',
        'history/core/browser/url_database.cc',
        'history/core/browser/url_database.h',
        'history/core/browser/url_row.cc',
        'history/core/browser/url_row.h',
        'history/core/browser/url_utils.cc',
        'history/core/browser/url_utils.h',
        'history/core/browser/visit_database.cc',
        'history/core/browser/visit_database.h',
        'history/core/browser/visit_filter.cc',
        'history/core/browser/visit_filter.h',
        'history/core/browser/visit_tracker.cc',
        'history/core/browser/visit_tracker.h',
        'history/core/browser/visitsegment_database.cc',
        'history/core/browser/visitsegment_database.h',
      ],
      'conditions': [
        ['OS=="android"', {
          'sources': [
            'history/core/browser/android/android_cache_database.cc',
            'history/core/browser/android/android_cache_database.h',
            'history/core/browser/android/android_history_types.cc',
            'history/core/browser/android/android_history_types.h',
            'history/core/browser/android/android_time.h',
            'history/core/browser/android/android_urls_database.cc',
            'history/core/browser/android/android_urls_database.h',
            'history/core/browser/android/android_urls_sql_handler.cc',
            'history/core/browser/android/android_urls_sql_handler.h',
            'history/core/browser/android/favicon_sql_handler.cc',
            'history/core/browser/android/favicon_sql_handler.h',
            'history/core/browser/android/sql_handler.cc',
            'history/core/browser/android/sql_handler.h',
            'history/core/browser/android/urls_sql_handler.cc',
            'history/core/browser/android/urls_sql_handler.h',
            'history/core/browser/android/visit_sql_handler.cc',
            'history/core/browser/android/visit_sql_handler.h',
          ],
        }],
      ],
    },
    {
      # GN version: //components/history/core/browser:proto
      # Protobuf compiler / generator for the InMemoryURLIndex caching
      # protocol buffer.
      'target_name': 'history_core_browser_proto',
      'type': 'static_library',
      'sources': [
        'history/core/browser/in_memory_url_index_cache.proto',
      ],
      'variables': {
        'proto_in_dir': 'history/core/browser',
        'proto_out_dir': 'components/history/core/browser',
      },
      'includes': [ '../build/protoc.gypi' ]
    },
    {
      # GN version: //components/history/core/common
      'target_name': 'history_core_common',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        '../base/base.gyp:base',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'history/core/common/thumbnail_score.cc',
        'history/core/common/thumbnail_score.h',
      ],
    },
    {
      # GN version: //components/history/core/test
      'target_name': 'history_core_test_support',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../sql/sql.gyp:sql',
        '../testing/gtest.gyp:gtest',
        '../url/url.gyp:url_lib',
        'history_core_browser',
      ],
      'sources': [
        # Note: sources list duplicated in GN build.
        'history/core/test/history_client_fake_bookmarks.cc',
        'history/core/test/history_client_fake_bookmarks.h',
        'history/core/test/history_unittest_base.cc',
        'history/core/test/history_unittest_base.h',
        'history/core/test/test_history_database.cc',
        'history/core/test/test_history_database.h',
      ],
    },
  ],
  'conditions': [
    ['OS!="ios"', {
      'targets': [
        {
          # GN version: //components/history/content/browser
          'target_name': 'history_content_browser',
          'type': 'static_library',
          'include_dirs': [
            '..',
          ],
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content_browser',
            'history_core_browser',
          ],
          'sources': [
            'history/content/browser/download_constants_utils.cc',
            'history/content/browser/download_constants_utils.h',
            'history/content/browser/history_context_helper.cc',
            'history/content/browser/history_context_helper.h',
            'history/content/browser/history_database_helper.cc',
            'history/content/browser/history_database_helper.h',
          ],
        }
      ],
    }],
  ],
}
