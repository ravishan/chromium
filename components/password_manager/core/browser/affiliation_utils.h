// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains utilities related to working with "facets".
//
// A "facet" is defined as the manifestation of a logical application on a given
// platform. For example, "My Bank" may have released an Android application
// and a Web application accessible from a browser. These are all facets of the
// "My Bank" logical application.
//
// Facets that belong to the same logical application are said to be affiliated
// with each other. Conceptually, "affiliations" can be seen as an equivalence
// relation defined over the set of all facets. Each equivalence class contains
// facets that belong to the same logical application, and therefore should be
// treated as synonymous for certain purposes, e.g., sharing credentials.
//
// A valid facet identifier will be a URI of the form:
//
//   * https://<host>[:<port>]
//
//      For web sites. Only HTTPS sites are supported. The syntax corresponds to
//      that of 'serialized-origin' in RFC 6454. That is, in canonical form, the
//      URI must not contain components other than the scheme (required, must be
//      "https"), host (required), and port (optional); with canonicalization
//      performed the same way as it normally would be for standard URLs.
//
//   * android://<certificate_hash>@<package_name>
//
//      For Android applications. In canonical form, the URI must not contain
//      components other than the scheme (must be "android"), username, and host
//      (all required). The host part must be a valid Android package name, with
//      no escaping, so it must be composed of characters [a-zA-Z0-9_.].
//
//      The username part must be the hash of the certificate used to sign the
//      APK, base64-encoded using padding and the "URL and filename safe" base64
//      alphabet, with no further escaping. This is normally calculated as:
//
//        echo -n -e "$PEM_KEY" |
//          openssl x509 -outform DER |
//          openssl sha -sha512 -binary | base64 | tr '+/' '-_'
//

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_UTILS_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_UTILS_H_

#include <string>
#include <vector>

#include "base/logging.h"

namespace password_manager {

// Encapsulates a facet URI in canonical form.
//
// This is a very light-weight wrapper around an std::string containing the text
// of the URI, and can be passed around as a value. The main rationale for the
// existance of this class is to make it clearer in the code when a certain URI
// is known to be a valid facet URI in canonical form, and to allow verifying
// and converting URIs to such canonical form.
//
// Note that it would be impractical to use GURL to represent facet URIs, as
// GURL has built-in logic to parse the rest of the URI according to its scheme,
// and obviously, it does not recognize the "android" scheme. Therefore, after
// parsing, everything ends up in the path component, which is not too helpful.
class FacetURI {
 public:
  FacetURI();

  // As a light-weight std::string wrapper, allow copy and assign.
  FacetURI(const FacetURI&) = default;
  FacetURI& operator=(const FacetURI&) = default;

  // Constructs an instance to encapsulate the canonical form of |spec|.
  // If |spec| is not a valid facet URI, then an invalid instance is returned,
  // which then should be discarded.
  static FacetURI FromPotentiallyInvalidSpec(const std::string& spec);

  // Constructs a valid FacetURI instance from a valid |canonical_spec|.
  // Note: The passed-in URI is not verified at all. Use only when you are sure
  // the URI is valid and in canonical form.
  static FacetURI FromCanonicalSpec(const std::string& canonical_spec);

  // Comparison operators so that FacetURI can be used in std::equal.
  bool operator==(const FacetURI& other) const;
  bool operator!=(const FacetURI& other) const;

  // Relational operators so that FacetURI can be used in sorted containers.
  bool operator<(const FacetURI& other) const;
  bool operator>(const FacetURI& other) const;

  // Returns whether or not this instance represents a valid facet identifier
  // referring to a Web application.
  bool IsValidWebFacetURI() const;

  // Returns whether or not this instance represents a valid facet identifier
  // referring to an Android application.
  bool IsValidAndroidFacetURI() const;

  // Returns whether or not this instance represents a valid facet identifier
  // referring to either a Web or an Android application.
  bool is_valid() const {
    return is_valid_;
  }

  // Returns the text of the encapsulated canonical URI, which must be valid.
  const std::string& canonical_spec() const {
    DCHECK(is_valid_);
    return canonical_spec_;
  }

 private:
  // Internal constructor to be used by the static factory methods.
  FacetURI(const std::string& canonical_spec, bool is_valid);

  // Whether |canonical_spec_| contains a valid facet URI in canonical form.
  bool is_valid_;

  // The text of the encapsulated canonical URI, valid if and only if
  // |is_valid_| is true.
  std::string canonical_spec_;
};

// A collection of facets affiliated with each other, i.e. an equivalence class.
typedef std::vector<FacetURI> AffiliatedFacets;

// Returns whether or not equivalence classes |a| and |b| are equal, that is,
// whether or not they consist of the same set of facets.
//
// Note that this will do some sorting, so it can be expensive for large inputs.
bool AreEquivalenceClassesEqual(const AffiliatedFacets& a,
                                const AffiliatedFacets& b);

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_UTILS_H_
