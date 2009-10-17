// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>
#include <atlsecurity.h>
#include <shellapi.h>

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/ref_counted.h"
#include "base/scoped_handle.h"
#include "base/task.h"
#include "base/win_util.h"
#include "net/base/net_util.h"

#include "chrome_frame/test/chrome_frame_unittests.h"
#include "chrome_frame/chrome_frame_automation.h"
#include "chrome_frame/chrome_frame_delegate.h"
#include "chrome_frame/html_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kChromeFrameUserAgent[] = "chromeframe";

class HtmlUtilUnittest : public testing::Test {
 protected:
  // Constructor
  HtmlUtilUnittest() {}

  // Returns the test path given a test case.
  virtual bool GetTestPath(const std::wstring& test_case, std::wstring* path) {
    if (!path) {
      NOTREACHED();
      return false;
    }

    std::wstring test_path;
    if (!PathService::Get(base::DIR_SOURCE_ROOT, &test_path)) {
      NOTREACHED();
      return false;
    }

    file_util::AppendToPath(&test_path, L"chrome_frame");
    file_util::AppendToPath(&test_path, L"test");
    file_util::AppendToPath(&test_path, L"html_util_test_data");
    file_util::AppendToPath(&test_path, test_case);

    *path = test_path;
    return true;
  }

  virtual bool GetTestData(const std::wstring& test_case, std::wstring* data) {
    if (!data) {
      NOTREACHED();
      return false;
    }

    std::wstring path;
    if (!GetTestPath(test_case, &path)) {
      NOTREACHED();
      return false;
    }

    std::string raw_data;
    file_util::ReadFileToString(path, &raw_data);

    // Convert to wide using the "best effort" assurance described in
    // string_util.h
    data->assign(UTF8ToWide(raw_data));
    return true;
  }
};

TEST_F(HtmlUtilUnittest, BasicTest) {
  std::wstring test_data;
  GetTestData(L"basic_test.html", &test_data);

  HTMLScanner scanner(test_data.c_str());

  // Grab the meta tag from the document and ensure that we get exactly one.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  ASSERT_EQ(1, tag_list.size());

  // Pull out the http-equiv attribute and check its value:
  HTMLScanner::StringRange attribute_value;
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"http-equiv", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"X-UA-Compatible"));

  // Pull out the content attribute and check its value:
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"content", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"chrome=1"));
}

TEST_F(HtmlUtilUnittest, QuotesTest) {
  std::wstring test_data;
  GetTestData(L"quotes_test.html", &test_data);

  HTMLScanner scanner(test_data.c_str());

  // Grab the meta tag from the document and ensure that we get exactly one.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  ASSERT_EQ(1, tag_list.size());

  // Pull out the http-equiv attribute and check its value:
  HTMLScanner::StringRange attribute_value;
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"http-equiv", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"X-UA-Compatible"));

  // Pull out the content attribute and check its value:
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"content", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"chrome=1"));
}

TEST_F(HtmlUtilUnittest, DegenerateCasesTest) {
  std::wstring test_data;
  GetTestData(L"degenerate_cases_test.html", &test_data);

  HTMLScanner scanner(test_data.c_str());

  // Scan for meta tags in the document. We expect not to pick up the one
  // that appears to be there since it is technically inside a quote block.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  EXPECT_TRUE(tag_list.empty());
}

TEST_F(HtmlUtilUnittest, MultipleTagsTest) {
  std::wstring test_data;
  GetTestData(L"multiple_tags.html", &test_data);

  HTMLScanner scanner(test_data.c_str());

  // Grab the meta tag from the document and ensure that we get exactly three.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  EXPECT_EQ(7, tag_list.size());

  // Pull out the content attribute for each tag and check its value:
  HTMLScanner::StringRange attribute_value;
  HTMLScanner::StringRangeList::const_iterator tag_list_iter(
      tag_list.begin());
  int valid_tag_count = 0;
  for (; tag_list_iter != tag_list.end(); tag_list_iter++) {
    HTMLScanner::StringRange attribute_value;
    if (tag_list_iter->GetTagAttribute(L"http-equiv", &attribute_value) &&
        attribute_value.Equals(L"X-UA-Compatible")) {
      EXPECT_TRUE(tag_list_iter->GetTagAttribute(L"content", &attribute_value));
      EXPECT_TRUE(attribute_value.Equals(L"chrome=1"));
      valid_tag_count++;
    }
  }
  EXPECT_EQ(3, valid_tag_count);
}

TEST_F(HtmlUtilUnittest, ShortDegenerateTest1) {
  std::wstring test_data(
      L"<foo><META http-equiv=X-UA-Compatible content='chrome=1'");

  HTMLScanner scanner(test_data.c_str());

  // Scan for meta tags in the document. We expect not to pick up the one
  // that is there since it is not properly closed.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  EXPECT_TRUE(tag_list.empty());
}

TEST_F(HtmlUtilUnittest, ShortDegenerateTest2) {
  std::wstring test_data(
    L"<foo <META http-equiv=X-UA-Compatible content='chrome=1'/>");

  HTMLScanner scanner(test_data.c_str());

  // Scan for meta tags in the document. We expect not to pick up the one
  // that appears to be there since it is inside a non-closed tag.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  EXPECT_TRUE(tag_list.empty());
}

TEST_F(HtmlUtilUnittest, QuoteInsideHTMLCommentTest) {
  std::wstring test_data(
    L"<!-- comment' --><META http-equiv=X-UA-Compatible content='chrome=1'/>");

  HTMLScanner scanner(test_data.c_str());

  // Grab the meta tag from the document and ensure that we get exactly one.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  ASSERT_EQ(1, tag_list.size());

  // Pull out the http-equiv attribute and check its value:
  HTMLScanner::StringRange attribute_value;
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"http-equiv", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"X-UA-Compatible"));

  // Pull out the content attribute and check its value:
  EXPECT_TRUE(tag_list[0].GetTagAttribute(L"content", &attribute_value));
  EXPECT_TRUE(attribute_value.Equals(L"chrome=1"));
}

TEST_F(HtmlUtilUnittest, CloseTagInsideHTMLCommentTest) {
  std::wstring test_data(
    L"<!-- comment> <META http-equiv=X-UA-Compatible content='chrome=1'/>-->");

  HTMLScanner scanner(test_data.c_str());

  // Grab the meta tag from the document and ensure that we get exactly one.
  HTMLScanner::StringRangeList tag_list;
  scanner.GetTagsByName(L"meta", &tag_list, L"body");
  ASSERT_TRUE(tag_list.empty());
}

TEST_F(HtmlUtilUnittest, AddChromeFrameToUserAgentValue) {
  struct TestCase {
    std::string input_;
    std::string expected_;
  } test_cases[] = {
    {
      "", ""
    }, {
      "Mozilla/4.7 [en] (WinNT; U)",
      "Mozilla/4.7 [en] (WinNT; U) chromeframe/0.0"
    }, {
      "Mozilla/4.0 (compatible; MSIE 5.01; Windows NT)",
      "Mozilla/4.0 (compatible; MSIE 5.01; Windows NT) chromeframe/0.0"
    }, {
      "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; T312461; "
        ".NET CLR 1.1.4322)",
      "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; T312461; "
        ".NET CLR 1.1.4322) chromeframe/0.0"
    }, {
      "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT 4.0) Opera 5.11 [en]",
      "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT 4.0) "
          "Opera 5.11 [en] chromeframe/0.0"
    }, {
      "Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:1.0.2) "
          "Gecko/20030208 Netscape/7.02",
      "Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:1.0.2) "
          "Gecko/20030208 Netscape/7.02 chromeframe/0.0"
    }, {
      "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.6) Gecko/20040612 "
          "Firefox/0.8",
      "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.6) Gecko/20040612 "
          "Firefox/0.8 chromeframe/0.0"
    }, {
      "Mozilla/5.0 (compatible; Konqueror/3.2; Linux) (KHTML, like Gecko)",
      "Mozilla/5.0 (compatible; Konqueror/3.2; Linux) "
          "(KHTML, like Gecko) chromeframe/0.0"
    }, {
      "Lynx/2.8.4rel.1 libwww-FM/2.14 SSL-MM/1.4.1 OpenSSL/0.9.6h",
      "Lynx/2.8.4rel.1 libwww-FM/2.14 SSL-MM/1.4.1 "
          "OpenSSL/0.9.6h chromeframe/0.0",
    }, {
      "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.7.10) "
          "Gecko/20050716 Firefox/1.0.6",
      "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.7.10) "
          "Gecko/20050716 Firefox/1.0.6 chromeframe/0.0"
    }, {
      "Invalid/1.1 ((((((",
      "Invalid/1.1 (((((( chromeframe/0.0",
    }, {
      "Invalid/1.1 ()))))",
      "Invalid/1.1 ())))) chromeframe/0.0",
    }, {
      "Strange/1.1 ()",
      "Strange/1.1 () chromeframe/0.0",
    }
  };

  for (int i = 0; i < arraysize(test_cases); ++i) {
    std::string new_ua(
        http_utils::AddChromeFrameToUserAgentValue(test_cases[i].input_));
    EXPECT_EQ(test_cases[i].expected_, new_ua);
  }

  // Now do the same test again, but test that we don't add the chromeframe
  // tag if we've already added it.
  for (int i = 0; i < arraysize(test_cases); ++i) {
    std::string ua(test_cases[i].expected_);
    std::string new_ua(http_utils::AddChromeFrameToUserAgentValue(ua));
    EXPECT_EQ(test_cases[i].expected_, new_ua);
  }
}

TEST_F(HtmlUtilUnittest, GetDefaultUserAgentHeaderWithCFTag) {
  std::string ua(http_utils::GetDefaultUserAgentHeaderWithCFTag());
  EXPECT_NE(0, ua.length());
  EXPECT_NE(std::string::npos, ua.find("Mozilla"));
  EXPECT_NE(std::string::npos, ua.find(kChromeFrameUserAgent));
}

TEST_F(HtmlUtilUnittest, GetDefaultUserAgent) {
  std::string ua(http_utils::GetDefaultUserAgent());
  EXPECT_NE(0, ua.length());
  EXPECT_NE(std::string::npos, ua.find("Mozilla"));
}

TEST_F(HtmlUtilUnittest, GetChromeFrameUserAgent) {
  const char* call1 = http_utils::GetChromeFrameUserAgent();
  const char* call2 = http_utils::GetChromeFrameUserAgent();
  // Expect static buffer since caller does no cleanup.
  EXPECT_EQ(call1, call2);
  std::string ua(call1);
  EXPECT_EQ("chromeframe/0.0", ua);
}
