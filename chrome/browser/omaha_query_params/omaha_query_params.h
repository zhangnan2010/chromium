// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OMAHA_QUERY_PARAMS_OMAHA_QUERY_PARAMS_H_
#define CHROME_BROWSER_OMAHA_QUERY_PARAMS_OMAHA_QUERY_PARAMS_H_

#include <string>

#include "base/basictypes.h"

namespace chrome {

class OmahaQueryParams {
 public:
  enum ProdId {
    CHROME = 0,
    CRX,
  };

  // Generates a string of URL query paramaters to be used when getting
  // component and extension updates. Includes the following fields: os, arch,
  // prod, prodchannel, prodversion, lang.
  static std::string Get(ProdId prod);

  // Returns the value we use for the "prod=" parameter. Possible return values
  // include "chrome", "chromecrx", "chromiumcrx", and "unknown".
  static const char* GetProdIdString(chrome::OmahaQueryParams::ProdId prod);

  // Returns the value we use for the "os=" parameter. Possible return values
  // include: "mac", "win", "android", "cros", "linux", and "openbsd".
  static const char* GetOS();

  // Returns the value we use for the "arch=" parameter. Possible return values
  // include: "x86", "x64", and "arm".
  static const char* GetArch();

  // Returns the value we use for the "nacl_arch" parameter. Note that this may
  // be different from the "arch" parameter above (e.g. one may be 32-bit and
  // the other 64-bit). Possible return values include: "x86-32", "x86-64",
  // "arm", and "mips32".
  static const char* GetNaclArch();

  // Returns the value we use for the "updaterchannel=" and "prodchannel="
  // parameters. Possible return values include: "canary", "dev", "beta", and
  // "stable".
  static const char* GetChannelString();

  // Returns the language for the present locale. Possible return values are
  // standard tags for languages, such as "en", "en-US", "de", "fr", "af", etc.
  static const char* GetLang();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OmahaQueryParams);
};

}  // namespace chrome

#endif  // CHROME_BROWSER_OMAHA_QUERY_PARAMS_OMAHA_QUERY_PARAMS_H_
