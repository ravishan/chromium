// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/wallet/real_pan_wallet_client.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/autofill/core/browser/credit_card.h"
#include "google_apis/gaia/identity_provider.h"
#include "net/base/escape.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace autofill {
namespace wallet {

namespace {

const char kUnmaskCardRequestFormat[] =
    "request_content_type=application/json&request=%s&cvc=%s";

// TODO(estade): use a sandbox server on dev builds?
const char kUnmaskCardRequestUrl[] =
    "https://wallet.google.com/payments/apis-secure/creditcardservice"
    "/GetRealPan?s7e=cvc";

const char kTokenServiceConsumerId[] = "real_pan_wallet_client";
const char kWalletOAuth2Scope[] =
    "https://www.googleapis.com/auth/wallet.chrome";

}  // namespace

RealPanWalletClient::RealPanWalletClient(
    net::URLRequestContextGetter* context_getter,
    Delegate* delegate)
    : OAuth2TokenService::Consumer(kTokenServiceConsumerId),
      context_getter_(context_getter),
      delegate_(delegate),
      weak_ptr_factory_(this) {
  DCHECK(delegate);
}

RealPanWalletClient::~RealPanWalletClient() {
}

void RealPanWalletClient::Prepare() {
  if (access_token_.empty())
    StartTokenFetch();
}

void RealPanWalletClient::UnmaskCard(const CreditCard& card,
                                     const std::string& cvc) {
  DCHECK_EQ(CreditCard::MASKED_SERVER_CARD, card.record_type());

  request_.reset(net::URLFetcher::Create(
      0, GURL(kUnmaskCardRequestUrl), net::URLFetcher::POST, this));
  request_->SetRequestContext(context_getter_.get());

  base::DictionaryValue request_dict;
  request_dict.SetString("encrypted_cvc", "__param:cvc");
  // TODO(estade): is this the correct "token"?
  request_dict.SetString("credit_card_token", card.server_id());
  std::string json_request;
  base::JSONWriter::Write(&request_dict, &json_request);
  std::string post_body = base::StringPrintf(kUnmaskCardRequestFormat,
      net::EscapeUrlEncodedData(json_request, true).c_str(),
      net::EscapeUrlEncodedData(cvc, true).c_str());
  request_->SetUploadData("application/x-www-form-urlencoded", post_body);

  if (access_token_.empty())
    StartTokenFetch();
  else
    SetOAuth2TokenAndStartRequest();
}

void RealPanWalletClient::CancelRequest() {
  request_.reset();
}

void RealPanWalletClient::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, request_.get());

  // |request_|, which is aliased to |source|, might continue to be used in this
  // |method, but should be freed once control leaves the method.
  scoped_ptr<net::URLFetcher> scoped_request(request_.Pass());
  scoped_ptr<base::DictionaryValue> response_dict;
  int response_code = source->GetResponseCode();

  // TODO(estade): OAuth2 may fail due to an expired access token, in which case
  // we should invalidate the token and try again. How is that failure reported?

  switch (response_code) {
    // Valid response.
    case net::HTTP_OK: {
      std::string data;
      source->GetResponseAsString(&data);
      scoped_ptr<base::Value> message_value(base::JSONReader::Read(data));
      if (message_value.get() &&
          message_value->IsType(base::Value::TYPE_DICTIONARY)) {
        response_dict.reset(
            static_cast<base::DictionaryValue*>(message_value.release()));
      } else {
        NOTIMPLEMENTED();
      }
      break;
    }

    // HTTP_BAD_REQUEST means the arguments are invalid. No point retrying.
    case net::HTTP_BAD_REQUEST: {
      NOTIMPLEMENTED();
      break;
    }

    // Response contains an error to show the user.
    case net::HTTP_FORBIDDEN:
    case net::HTTP_INTERNAL_SERVER_ERROR: {
      NOTIMPLEMENTED();
      break;
    }

    // Handle anything else as a generic error.
    default:
      NOTIMPLEMENTED();
      break;
  }

  std::string real_pan;
  if (response_dict)
    response_dict->GetString("pan", &real_pan);
  delegate_->OnDidGetRealPan(real_pan);
}

void RealPanWalletClient::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  DCHECK_EQ(request, access_token_request_.get());
  access_token_ = access_token;
  if (request_)
    SetOAuth2TokenAndStartRequest();

  access_token_request_.reset();
}

void RealPanWalletClient::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  DCHECK_EQ(request, access_token_request_.get());
  if (request_) {
    request_.reset();
    delegate_->OnDidGetRealPan(std::string());
  }
  // TODO(estade): what do we do in the failure case?
  NOTIMPLEMENTED();

  access_token_request_.reset();
}

void RealPanWalletClient::StartTokenFetch() {
  // Don't cancel outstanding requests.
  if (access_token_request_)
    return;

  // However, do clear old tokens.
  access_token_.clear();

  OAuth2TokenService::ScopeSet wallet_scopes;
  wallet_scopes.insert(kWalletOAuth2Scope);
  IdentityProvider* identity = delegate_->GetIdentityProvider();
  access_token_request_ = identity->GetTokenService()->StartRequest(
      identity->GetActiveAccountId(), wallet_scopes, this);
}

void RealPanWalletClient::SetOAuth2TokenAndStartRequest() {
  request_->AddExtraRequestHeader("Authorization: " + access_token_);
  request_->Start();
}

}  // namespace wallet
}  // namespace autofill
