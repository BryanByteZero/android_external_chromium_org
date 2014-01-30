// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Unit tests for eliding and formatting utility functions.

#include "ui/gfx/text_elider.h"

#include "base/files/file_path.h"
#include "base/i18n/rtl.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/text_utils.h"

using base::ASCIIToUTF16;
using base::UTF16ToUTF8;
using base::UTF16ToWide;
using base::UTF8ToUTF16;
using base::WideToUTF16;

namespace gfx {

namespace {

struct Testcase {
  const std::string input;
  const std::string output;
};

struct FileTestcase {
  const base::FilePath::StringType input;
  const std::string output;
};

struct UTF16Testcase {
  const base::string16 input;
  const base::string16 output;
};

struct TestData {
  const std::string a;
  const std::string b;
  const int compare_result;
};

}  // namespace

// TODO(ios): This test fails on iOS because iOS version of GetStringWidthF
// that calls [NSString sizeWithFont] returns the rounded string width.
// TODO(338784): Enable this on android.
#if defined(OS_IOS) || defined(OS_ANDROID)
#define MAYBE_ElideEmail DISABLED_ElideEmail
#else
#define MAYBE_ElideEmail ElideEmail
#endif
TEST(TextEliderTest, MAYBE_ElideEmail) {
  const std::string kEllipsisStr(kEllipsis);

  // Test emails and their expected elided forms (from which the available
  // widths will be derived).
  // For elided forms in which both the username and domain must be elided:
  // the result (how many characters are left on each side) can be font
  // dependent. To avoid this, the username is prefixed with the characters
  // expected to remain in the domain.
  Testcase testcases[] = {
      {"g@g.c", "g@g.c"},
      {"g@g.c", kEllipsisStr},
      {"ga@co.ca", "ga@c" + kEllipsisStr + "a"},
      {"short@small.com", "s" + kEllipsisStr + "@s" + kEllipsisStr},
      {"short@small.com", "s" + kEllipsisStr + "@small.com"},
      {"short@longbutlotsofspace.com", "short@longbutlotsofspace.com"},
      {"short@longbutnotverymuchspace.com",
       "short@long" + kEllipsisStr + ".com"},
      {"la_short@longbutverytightspace.ca",
       "la" + kEllipsisStr + "@l" + kEllipsisStr + "a"},
      {"longusername@gmail.com", "long" + kEllipsisStr + "@gmail.com"},
      {"elidetothemax@justfits.com", "e" + kEllipsisStr + "@justfits.com"},
      {"thatom_somelongemail@thatdoesntfit.com",
       "thatom" + kEllipsisStr + "@tha" + kEllipsisStr + "om"},
      {"namefits@butthedomaindoesnt.com",
       "namefits@butthedo" + kEllipsisStr + "snt.com"},
      {"widthtootight@nospace.com", kEllipsisStr},
      {"nospaceforusername@l", kEllipsisStr},
      {"little@littlespace.com", "l" + kEllipsisStr + "@l" + kEllipsisStr},
      {"l@llllllllllllllllllllllll.com", "l@lllll" + kEllipsisStr + ".com"},
      {"messed\"up@whyanat\"++@notgoogley.com",
       "messed\"up@whyanat\"++@notgoogley.com"},
      {"messed\"up@whyanat\"++@notgoogley.com",
       "messed\"up@why" + kEllipsisStr + "@notgoogley.com"},
      {"noca_messed\"up@whyanat\"++@notgoogley.ca",
       "noca" + kEllipsisStr + "@no" + kEllipsisStr + "ca"},
      {"at\"@@@@@@@@@...@@.@.@.@@@\"@madness.com",
       "at\"@@@@@@@@@...@@.@." + kEllipsisStr + "@madness.com"},
      // Special case: "m..." takes more than half of the available width; thus
      // the domain must elide to "l..." and not "l...l" as it must allow enough
      // space for the minimal username elision although its half of the
      // available width would normally allow it to elide to "l...l".
      {"mmmmm@llllllllll", "m" + kEllipsisStr + "@l" + kEllipsisStr},
  };

  const FontList font_list;
  for (size_t i = 0; i < arraysize(testcases); ++i) {
    const base::string16 expected_output = UTF8ToUTF16(testcases[i].output);
    EXPECT_EQ(expected_output,
              ElideEmail(
                  UTF8ToUTF16(testcases[i].input),
                  font_list,
                  GetStringWidthF(expected_output, font_list)));
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideEmailMoreSpace DISABLED_ElideEmailMoreSpace
#else
#define MAYBE_ElideEmailMoreSpace ElideEmailMoreSpace
#endif
TEST(TextEliderTest, MAYBE_ElideEmailMoreSpace) {
  const int test_width_factors[] = {
      100,
      10000,
      1000000,
  };
  const std::string test_emails[] = {
      "a@c",
      "test@email.com",
      "short@verysuperdupperlongdomain.com",
      "supermegalongusername@withasuperlonnnggggdomain.gouv.qc.ca",
  };

  const FontList font_list;
  for (size_t i = 0; i < arraysize(test_width_factors); ++i) {
    const int test_width =
        font_list.GetExpectedTextWidth(test_width_factors[i]);
    for (size_t j = 0; j < arraysize(test_emails); ++j) {
      // Extra space is available: the email should not be elided.
      const base::string16 test_email = UTF8ToUTF16(test_emails[j]);
      EXPECT_EQ(test_email, ElideEmail(test_email, font_list, test_width));
    }
  }
}

// TODO(ios): This test fails on iOS because iOS version of GetStringWidthF
// that calls [NSString sizeWithFont] returns the rounded string width.
// TODO(338784): Enable this on android.
#if defined(OS_IOS) || defined(OS_ANDROID)
#define MAYBE_TestFilenameEliding DISABLED_TestFilenameEliding
#else
#define MAYBE_TestFilenameEliding TestFilenameEliding
#endif
TEST(TextEliderTest, MAYBE_TestFilenameEliding) {
  const std::string kEllipsisStr(kEllipsis);
  const base::FilePath::StringType kPathSeparator =
      base::FilePath::StringType().append(1, base::FilePath::kSeparators[0]);

  FileTestcase testcases[] = {
    {FILE_PATH_LITERAL(""), ""},
    {FILE_PATH_LITERAL("."), "."},
    {FILE_PATH_LITERAL("filename.exe"), "filename.exe"},
    {FILE_PATH_LITERAL(".longext"), ".longext"},
    {FILE_PATH_LITERAL("pie"), "pie"},
    {FILE_PATH_LITERAL("c:") + kPathSeparator + FILE_PATH_LITERAL("path") +
      kPathSeparator + FILE_PATH_LITERAL("filename.pie"),
      "filename.pie"},
    {FILE_PATH_LITERAL("c:") + kPathSeparator + FILE_PATH_LITERAL("path") +
      kPathSeparator + FILE_PATH_LITERAL("longfilename.pie"),
      "long" + kEllipsisStr + ".pie"},
    {FILE_PATH_LITERAL("http://path.com/filename.pie"), "filename.pie"},
    {FILE_PATH_LITERAL("http://path.com/longfilename.pie"),
      "long" + kEllipsisStr + ".pie"},
    {FILE_PATH_LITERAL("piesmashingtacularpants"), "pie" + kEllipsisStr},
    {FILE_PATH_LITERAL(".piesmashingtacularpants"), ".pie" + kEllipsisStr},
    {FILE_PATH_LITERAL("cheese."), "cheese."},
    {FILE_PATH_LITERAL("file name.longext"),
      "file" + kEllipsisStr + ".longext"},
    {FILE_PATH_LITERAL("fil ename.longext"),
      "fil " + kEllipsisStr + ".longext"},
    {FILE_PATH_LITERAL("filename.longext"),
      "file" + kEllipsisStr + ".longext"},
    {FILE_PATH_LITERAL("filename.middleext.longext"),
      "filename.mid" + kEllipsisStr + ".longext"},
    {FILE_PATH_LITERAL("filename.superduperextremelylongext"),
      "filename.sup" + kEllipsisStr + "emelylongext"},
    {FILE_PATH_LITERAL("filenamereallylongtext.superduperextremelylongext"),
      "filenamereall" + kEllipsisStr + "emelylongext"},
    {FILE_PATH_LITERAL("file.name.really.long.text.superduperextremelylongext"),
      "file.name.re" + kEllipsisStr + "emelylongext"}
  };

  static const FontList font_list;
  for (size_t i = 0; i < arraysize(testcases); ++i) {
    base::FilePath filepath(testcases[i].input);
    base::string16 expected = UTF8ToUTF16(testcases[i].output);
    expected = base::i18n::GetDisplayStringInLTRDirectionality(expected);
    EXPECT_EQ(expected, ElideFilename(filepath, font_list,
        GetStringWidthF(UTF8ToUTF16(testcases[i].output), font_list)));
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideTextTruncate DISABLED_ElideTextTruncate
#else
#define MAYBE_ElideTextTruncate ElideTextTruncate
#endif
TEST(TextEliderTest, MAYBE_ElideTextTruncate) {
  const FontList font_list;
  const float kTestWidth = GetStringWidthF(ASCIIToUTF16("Test"), font_list);
  struct TestData {
    const char* input;
    float width;
    const char* output;
  } cases[] = {
    { "", 0, "" },
    { "Test", 0, "" },
    { "", kTestWidth, "" },
    { "Tes", kTestWidth, "Tes" },
    { "Test", kTestWidth, "Test" },
    { "Tests", kTestWidth, "Test" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    base::string16 result = ElideText(UTF8ToUTF16(cases[i].input), font_list,
                                cases[i].width, TRUNCATE_AT_END);
    EXPECT_EQ(cases[i].output, UTF16ToUTF8(result));
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideTextEllipsis DISABLED_ElideTextEllipsis
#else
#define MAYBE_ElideTextEllipsis ElideTextEllipsis
#endif
TEST(TextEliderTest, MAYBE_ElideTextEllipsis) {
  const FontList font_list;
  const float kTestWidth = GetStringWidthF(ASCIIToUTF16("Test"), font_list);
  const char* kEllipsis = "\xE2\x80\xA6";
  const float kEllipsisWidth =
      GetStringWidthF(UTF8ToUTF16(kEllipsis), font_list);
  struct TestData {
    const char* input;
    float width;
    const char* output;
  } cases[] = {
    { "", 0, "" },
    { "Test", 0, "" },
    { "Test", kEllipsisWidth, kEllipsis },
    { "", kTestWidth, "" },
    { "Tes", kTestWidth, "Tes" },
    { "Test", kTestWidth, "Test" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    base::string16 result = ElideText(UTF8ToUTF16(cases[i].input), font_list,
                                cases[i].width, ELIDE_AT_END);
    EXPECT_EQ(cases[i].output, UTF16ToUTF8(result));
  }
}

// Checks that all occurrences of |first_char| are followed by |second_char| and
// all occurrences of |second_char| are preceded by |first_char| in |text|.
static void CheckSurrogatePairs(const base::string16& text,
                                base::char16 first_char,
                                base::char16 second_char) {
  size_t index = text.find_first_of(first_char);
  while (index != base::string16::npos) {
    EXPECT_LT(index, text.length() - 1);
    EXPECT_EQ(second_char, text[index + 1]);
    index = text.find_first_of(first_char, index + 1);
  }
  index = text.find_first_of(second_char);
  while (index != base::string16::npos) {
    EXPECT_GT(index, 0U);
    EXPECT_EQ(first_char, text[index - 1]);
    index = text.find_first_of(second_char, index + 1);
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideTextSurrogatePairs DISABLED_ElideTextSurrogatePairs
#else
#define MAYBE_ElideTextSurrogatePairs ElideTextSurrogatePairs
#endif
TEST(TextEliderTest, MAYBE_ElideTextSurrogatePairs) {
  const FontList font_list;
  // The below is 'MUSICAL SYMBOL G CLEF', which is represented in UTF-16 as
  // two characters forming a surrogate pair 0x0001D11E.
  const std::string kSurrogate = "\xF0\x9D\x84\x9E";
  const base::string16 kTestString =
      UTF8ToUTF16(kSurrogate + "ab" + kSurrogate + kSurrogate + "cd");
  const float kTestStringWidth = GetStringWidthF(kTestString, font_list);
  const base::char16 kSurrogateFirstChar = kTestString[0];
  const base::char16 kSurrogateSecondChar = kTestString[1];
  base::string16 result;

  // Elide |kTextString| to all possible widths and check that no instance of
  // |kSurrogate| was split in two.
  for (float width = 0; width <= kTestStringWidth; width++) {
    result = ElideText(kTestString, font_list, width, TRUNCATE_AT_END);
    CheckSurrogatePairs(result, kSurrogateFirstChar, kSurrogateSecondChar);

    result = ElideText(kTestString, font_list, width, ELIDE_AT_END);
    CheckSurrogatePairs(result, kSurrogateFirstChar, kSurrogateSecondChar);

    result = ElideText(kTestString, font_list, width, ELIDE_IN_MIDDLE);
    CheckSurrogatePairs(result, kSurrogateFirstChar, kSurrogateSecondChar);
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideTextLongStrings DISABLED_ElideTextLongStrings
#else
#define MAYBE_ElideTextLongStrings ElideTextLongStrings
#endif
TEST(TextEliderTest, MAYBE_ElideTextLongStrings) {
  const base::string16 kEllipsisStr = UTF8ToUTF16(kEllipsis);
  base::string16 data_scheme(UTF8ToUTF16("data:text/plain,"));
  size_t data_scheme_length = data_scheme.length();

  base::string16 ten_a(10, 'a');
  base::string16 hundred_a(100, 'a');
  base::string16 thousand_a(1000, 'a');
  base::string16 ten_thousand_a(10000, 'a');
  base::string16 hundred_thousand_a(100000, 'a');
  base::string16 million_a(1000000, 'a');

  size_t number_of_as = 156;
  base::string16 long_string_end(
      data_scheme + base::string16(number_of_as, 'a') + kEllipsisStr);
  UTF16Testcase testcases_end[] = {
     {data_scheme + ten_a,              data_scheme + ten_a},
     {data_scheme + hundred_a,          data_scheme + hundred_a},
     {data_scheme + thousand_a,         long_string_end},
     {data_scheme + ten_thousand_a,     long_string_end},
     {data_scheme + hundred_thousand_a, long_string_end},
     {data_scheme + million_a,          long_string_end},
  };

  const FontList font_list;
  float ellipsis_width = GetStringWidthF(kEllipsisStr, font_list);
  for (size_t i = 0; i < arraysize(testcases_end); ++i) {
    // Compare sizes rather than actual contents because if the test fails,
    // output is rather long.
    EXPECT_EQ(testcases_end[i].output.size(),
              ElideText(
                  testcases_end[i].input,
                  font_list,
                  GetStringWidthF(testcases_end[i].output, font_list),
                  ELIDE_AT_END).size());
    EXPECT_EQ(kEllipsisStr,
              ElideText(testcases_end[i].input, font_list, ellipsis_width,
                        ELIDE_AT_END));
  }

  size_t number_of_trailing_as = (data_scheme_length + number_of_as) / 2;
  base::string16 long_string_middle(data_scheme +
      base::string16(number_of_as - number_of_trailing_as, 'a') + kEllipsisStr +
      base::string16(number_of_trailing_as, 'a'));
  UTF16Testcase testcases_middle[] = {
     {data_scheme + ten_a,              data_scheme + ten_a},
     {data_scheme + hundred_a,          data_scheme + hundred_a},
     {data_scheme + thousand_a,         long_string_middle},
     {data_scheme + ten_thousand_a,     long_string_middle},
     {data_scheme + hundred_thousand_a, long_string_middle},
     {data_scheme + million_a,          long_string_middle},
  };

  for (size_t i = 0; i < arraysize(testcases_middle); ++i) {
    // Compare sizes rather than actual contents because if the test fails,
    // output is rather long.
    EXPECT_EQ(testcases_middle[i].output.size(),
              ElideText(
                  testcases_middle[i].input,
                  font_list,
                  GetStringWidthF(testcases_middle[i].output, font_list),
                  ELIDE_AT_END).size());
    EXPECT_EQ(kEllipsisStr,
              ElideText(testcases_middle[i].input, font_list, ellipsis_width,
                        ELIDE_AT_END));
  }
}

TEST(TextEliderTest, ElideString) {
  struct TestData {
    const char* input;
    int max_len;
    bool result;
    const char* output;
  } cases[] = {
    { "Hello", 0, true, "" },
    { "", 0, false, "" },
    { "Hello, my name is Tom", 1, true, "H" },
    { "Hello, my name is Tom", 2, true, "He" },
    { "Hello, my name is Tom", 3, true, "H.m" },
    { "Hello, my name is Tom", 4, true, "H..m" },
    { "Hello, my name is Tom", 5, true, "H...m" },
    { "Hello, my name is Tom", 6, true, "He...m" },
    { "Hello, my name is Tom", 7, true, "He...om" },
    { "Hello, my name is Tom", 10, true, "Hell...Tom" },
    { "Hello, my name is Tom", 100, false, "Hello, my name is Tom" }
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    base::string16 output;
    EXPECT_EQ(cases[i].result,
              ElideString(UTF8ToUTF16(cases[i].input),
                          cases[i].max_len, &output));
    EXPECT_EQ(cases[i].output, UTF16ToUTF8(output));
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideRectangleText DISABLED_ElideRectangleText
#else
#define MAYBE_ElideRectangleText ElideRectangleText
#endif
TEST(TextEliderTest, MAYBE_ElideRectangleText) {
  const FontList font_list;
  const int line_height = font_list.GetHeight();
  const float test_width = GetStringWidthF(ASCIIToUTF16("Test"), font_list);

  struct TestData {
    const char* input;
    float available_pixel_width;
    int available_pixel_height;
    bool truncated_y;
    const char* output;
  } cases[] = {
    { "", 0, 0, false, NULL },
    { "", 1, 1, false, NULL },
    { "Test", test_width, 0, true, NULL },
    { "Test", test_width, 1, false, "Test" },
    { "Test", test_width, line_height, false, "Test" },
    { "Test Test", test_width, line_height, true, "Test" },
    { "Test Test", test_width, line_height + 1, false, "Test|Test" },
    { "Test Test", test_width, line_height * 2, false, "Test|Test" },
    { "Test Test", test_width, line_height * 3, false, "Test|Test" },
    { "Test Test", test_width * 2, line_height * 2, false, "Test|Test" },
    { "Test Test", test_width * 3, line_height, false, "Test Test" },
    { "Test\nTest", test_width * 3, line_height * 2, false, "Test|Test" },
    { "Te\nst Te", test_width, line_height * 3, false, "Te|st|Te" },
    { "\nTest", test_width, line_height * 2, false, "|Test" },
    { "\nTest", test_width, line_height, true, "" },
    { "\n\nTest", test_width, line_height * 3, false, "||Test" },
    { "\n\nTest", test_width, line_height * 2, true, "|" },
    { "Test\n", 2 * test_width, line_height * 5, false, "Test|" },
    { "Test\n\n", 2 * test_width, line_height * 5, false, "Test||" },
    { "Test\n\n\n", 2 * test_width, line_height * 5, false, "Test|||" },
    { "Test\nTest\n\n", 2 * test_width, line_height * 5, false, "Test|Test||" },
    { "Test\n\nTest\n", 2 * test_width, line_height * 5, false, "Test||Test|" },
    { "Test\n\n\nTest", 2 * test_width, line_height * 5, false, "Test|||Test" },
    { "Te ", test_width, line_height, false, "Te" },
    { "Te  Te Test", test_width, 3 * line_height, false, "Te|Te|Test" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<base::string16> lines;
    EXPECT_EQ(cases[i].truncated_y ? INSUFFICIENT_SPACE_VERTICAL : 0,
              ElideRectangleText(UTF8ToUTF16(cases[i].input),
                                 font_list,
                                 cases[i].available_pixel_width,
                                 cases[i].available_pixel_height,
                                 TRUNCATE_LONG_WORDS,
                                 &lines));
    if (cases[i].output) {
      const std::string result = UTF16ToUTF8(JoinString(lines, '|'));
      EXPECT_EQ(cases[i].output, result) << "Case " << i << " failed!";
    } else {
      EXPECT_TRUE(lines.empty()) << "Case " << i << " failed!";
    }
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideRectangleTextPunctuation \
    DISABLED_ElideRectangleTextPunctuation
#else
#define MAYBE_ElideRectangleTextPunctuation ElideRectangleTextPunctuation
#endif
TEST(TextEliderTest, MAYBE_ElideRectangleTextPunctuation) {
  const FontList font_list;
  const int line_height = font_list.GetHeight();
  const float test_width = GetStringWidthF(ASCIIToUTF16("Test"), font_list);
  const float test_t_width = GetStringWidthF(ASCIIToUTF16("Test T"), font_list);

  struct TestData {
    const char* input;
    float available_pixel_width;
    int available_pixel_height;
    bool wrap_words;
    bool truncated_x;
    const char* output;
  } cases[] = {
    { "Test T.", test_t_width, line_height * 2, false, false, "Test|T." },
    { "Test T ?", test_t_width, line_height * 2, false, false, "Test|T ?" },
    { "Test. Test", test_width, line_height * 3, false, true, "Test|Test" },
    { "Test. Test", test_width, line_height * 3, true, false, "Test|.|Test" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<base::string16> lines;
    const WordWrapBehavior wrap_behavior =
        (cases[i].wrap_words ? WRAP_LONG_WORDS : TRUNCATE_LONG_WORDS);
    EXPECT_EQ(cases[i].truncated_x ? INSUFFICIENT_SPACE_HORIZONTAL : 0,
              ElideRectangleText(UTF8ToUTF16(cases[i].input),
                                 font_list,
                                 cases[i].available_pixel_width,
                                 cases[i].available_pixel_height,
                                 wrap_behavior,
                                 &lines));
    if (cases[i].output) {
      const std::string result = UTF16ToUTF8(JoinString(lines, '|'));
      EXPECT_EQ(cases[i].output, result) << "Case " << i << " failed!";
    } else {
      EXPECT_TRUE(lines.empty()) << "Case " << i << " failed!";
    }
  }
}

// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideRectangleTextLongWords DISABLED_ElideRectangleTextLongWords
#else
#define MAYBE_ElideRectangleTextLongWords ElideRectangleTextLongWords
#endif
TEST(TextEliderTest, MAYBE_ElideRectangleTextLongWords) {
  const FontList font_list;
  const int kAvailableHeight = 1000;
  const base::string16 kElidedTesting =
      UTF8ToUTF16(std::string("Tes") + kEllipsis);
  const float elided_width = GetStringWidthF(kElidedTesting, font_list);
  const float test_width = GetStringWidthF(ASCIIToUTF16("Test"), font_list);

  struct TestData {
    const char* input;
    float available_pixel_width;
    WordWrapBehavior wrap_behavior;
    bool truncated_x;
    const char* output;
  } cases[] = {
    { "Testing", test_width, IGNORE_LONG_WORDS, false, "Testing" },
    { "X Testing", test_width, IGNORE_LONG_WORDS, false, "X|Testing" },
    { "Test Testing", test_width, IGNORE_LONG_WORDS, false, "Test|Testing" },
    { "Test\nTesting", test_width, IGNORE_LONG_WORDS, false, "Test|Testing" },
    { "Test Tests ", test_width, IGNORE_LONG_WORDS, false, "Test|Tests" },
    { "Test Tests T", test_width, IGNORE_LONG_WORDS, false, "Test|Tests|T" },

    { "Testing", elided_width, ELIDE_LONG_WORDS, true, "Tes..." },
    { "X Testing", elided_width, ELIDE_LONG_WORDS, true, "X|Tes..." },
    { "Test Testing", elided_width, ELIDE_LONG_WORDS, true, "Test|Tes..." },
    { "Test\nTesting", elided_width, ELIDE_LONG_WORDS, true, "Test|Tes..." },

    { "Testing", test_width, TRUNCATE_LONG_WORDS, true, "Test" },
    { "X Testing", test_width, TRUNCATE_LONG_WORDS, true, "X|Test" },
    { "Test Testing", test_width, TRUNCATE_LONG_WORDS, true, "Test|Test" },
    { "Test\nTesting", test_width, TRUNCATE_LONG_WORDS, true, "Test|Test" },
    { "Test Tests ", test_width, TRUNCATE_LONG_WORDS, true, "Test|Test" },
    { "Test Tests T", test_width, TRUNCATE_LONG_WORDS, true, "Test|Test|T" },

    { "Testing", test_width, WRAP_LONG_WORDS, false, "Test|ing" },
    { "X Testing", test_width, WRAP_LONG_WORDS, false, "X|Test|ing" },
    { "Test Testing", test_width, WRAP_LONG_WORDS, false, "Test|Test|ing" },
    { "Test\nTesting", test_width, WRAP_LONG_WORDS, false, "Test|Test|ing" },
    { "Test Tests ", test_width, WRAP_LONG_WORDS, false, "Test|Test|s" },
    { "Test Tests T", test_width, WRAP_LONG_WORDS, false, "Test|Test|s T" },
    { "TestTestTest", test_width, WRAP_LONG_WORDS, false, "Test|Test|Test" },
    { "TestTestTestT", test_width, WRAP_LONG_WORDS, false, "Test|Test|Test|T" },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::vector<base::string16> lines;
    EXPECT_EQ(cases[i].truncated_x ? INSUFFICIENT_SPACE_HORIZONTAL : 0,
              ElideRectangleText(UTF8ToUTF16(cases[i].input),
                                 font_list,
                                 cases[i].available_pixel_width,
                                 kAvailableHeight,
                                 cases[i].wrap_behavior,
                                 &lines));
    std::string expected_output(cases[i].output);
    ReplaceSubstringsAfterOffset(&expected_output, 0, "...", kEllipsis);
    const std::string result = UTF16ToUTF8(JoinString(lines, '|'));
    EXPECT_EQ(expected_output, result) << "Case " << i << " failed!";
  }
}

// This test is to make sure that the width of each wrapped line does not
// exceed the available width. On some platform like Mac, this test used to
// fail because the truncated integer width is returned for the string
// and the accumulation of the truncated values causes the elide function
// to wrap incorrectly.
// TODO(338784): Enable this on android.
#if defined(OS_ANDROID)
#define MAYBE_ElideRectangleTextCheckLineWidth \
    DISABLED_ElideRectangleTextCheckLineWidth
#else
#define MAYBE_ElideRectangleTextCheckLineWidth ElideRectangleTextCheckLineWidth
#endif
TEST(TextEliderTest, MAYBE_ElideRectangleTextCheckLineWidth) {
  FontList font_list;
#if defined(OS_MACOSX) && !defined(OS_IOS)
  // Use a specific font to expose the line width exceeding problem.
  font_list = FontList(Font("LucidaGrande", 12));
#endif
  const float kAvailableWidth = 235;
  const int kAvailableHeight = 1000;
  const char text[] = "that Russian place we used to go to after fencing";
  std::vector<base::string16> lines;
  EXPECT_EQ(0, ElideRectangleText(UTF8ToUTF16(text),
                                  font_list,
                                  kAvailableWidth,
                                  kAvailableHeight,
                                  WRAP_LONG_WORDS,
                                  &lines));
  ASSERT_EQ(2u, lines.size());
  EXPECT_LE(GetStringWidthF(lines[0], font_list), kAvailableWidth);
  EXPECT_LE(GetStringWidthF(lines[1], font_list), kAvailableWidth);
}

TEST(TextEliderTest, ElideRectangleString) {
  struct TestData {
    const char* input;
    int max_rows;
    int max_cols;
    bool result;
    const char* output;
  } cases[] = {
    { "", 0, 0, false, "" },
    { "", 1, 1, false, "" },
    { "Hi, my name is\nTom", 0, 0,  true,  "..." },
    { "Hi, my name is\nTom", 1, 0,  true,  "\n..." },
    { "Hi, my name is\nTom", 0, 1,  true,  "..." },
    { "Hi, my name is\nTom", 1, 1,  true,  "H\n..." },
    { "Hi, my name is\nTom", 2, 1,  true,  "H\ni\n..." },
    { "Hi, my name is\nTom", 3, 1,  true,  "H\ni\n,\n..." },
    { "Hi, my name is\nTom", 4, 1,  true,  "H\ni\n,\n \n..." },
    { "Hi, my name is\nTom", 5, 1,  true,  "H\ni\n,\n \nm\n..." },
    { "Hi, my name is\nTom", 0, 2,  true,  "..." },
    { "Hi, my name is\nTom", 1, 2,  true,  "Hi\n..." },
    { "Hi, my name is\nTom", 2, 2,  true,  "Hi\n, \n..." },
    { "Hi, my name is\nTom", 3, 2,  true,  "Hi\n, \nmy\n..." },
    { "Hi, my name is\nTom", 4, 2,  true,  "Hi\n, \nmy\n n\n..." },
    { "Hi, my name is\nTom", 5, 2,  true,  "Hi\n, \nmy\n n\nam\n..." },
    { "Hi, my name is\nTom", 0, 3,  true,  "..." },
    { "Hi, my name is\nTom", 1, 3,  true,  "Hi,\n..." },
    { "Hi, my name is\nTom", 2, 3,  true,  "Hi,\n my\n..." },
    { "Hi, my name is\nTom", 3, 3,  true,  "Hi,\n my\n na\n..." },
    { "Hi, my name is\nTom", 4, 3,  true,  "Hi,\n my\n na\nme \n..." },
    { "Hi, my name is\nTom", 5, 3,  true,  "Hi,\n my\n na\nme \nis\n..." },
    { "Hi, my name is\nTom", 1, 4,  true,  "Hi, \n..." },
    { "Hi, my name is\nTom", 2, 4,  true,  "Hi, \nmy n\n..." },
    { "Hi, my name is\nTom", 3, 4,  true,  "Hi, \nmy n\name \n..." },
    { "Hi, my name is\nTom", 4, 4,  true,  "Hi, \nmy n\name \nis\n..." },
    { "Hi, my name is\nTom", 5, 4,  false, "Hi, \nmy n\name \nis\nTom" },
    { "Hi, my name is\nTom", 1, 5,  true,  "Hi, \n..." },
    { "Hi, my name is\nTom", 2, 5,  true,  "Hi, \nmy na\n..." },
    { "Hi, my name is\nTom", 3, 5,  true,  "Hi, \nmy na\nme \n..." },
    { "Hi, my name is\nTom", 4, 5,  true,  "Hi, \nmy na\nme \nis\n..." },
    { "Hi, my name is\nTom", 5, 5,  false, "Hi, \nmy na\nme \nis\nTom" },
    { "Hi, my name is\nTom", 1, 6,  true,  "Hi, \n..." },
    { "Hi, my name is\nTom", 2, 6,  true,  "Hi, \nmy \n..." },
    { "Hi, my name is\nTom", 3, 6,  true,  "Hi, \nmy \nname \n..." },
    { "Hi, my name is\nTom", 4, 6,  true,  "Hi, \nmy \nname \nis\n..." },
    { "Hi, my name is\nTom", 5, 6,  false, "Hi, \nmy \nname \nis\nTom" },
    { "Hi, my name is\nTom", 1, 7,  true,  "Hi, \n..." },
    { "Hi, my name is\nTom", 2, 7,  true,  "Hi, \nmy \n..." },
    { "Hi, my name is\nTom", 3, 7,  true,  "Hi, \nmy \nname \n..." },
    { "Hi, my name is\nTom", 4, 7,  true,  "Hi, \nmy \nname \nis\n..." },
    { "Hi, my name is\nTom", 5, 7,  false, "Hi, \nmy \nname \nis\nTom" },
    { "Hi, my name is\nTom", 1, 8,  true,  "Hi, my \n..." },
    { "Hi, my name is\nTom", 2, 8,  true,  "Hi, my \nname \n..." },
    { "Hi, my name is\nTom", 3, 8,  true,  "Hi, my \nname \nis\n..." },
    { "Hi, my name is\nTom", 4, 8,  false, "Hi, my \nname \nis\nTom" },
    { "Hi, my name is\nTom", 1, 9,  true,  "Hi, my \n..." },
    { "Hi, my name is\nTom", 2, 9,  true,  "Hi, my \nname is\n..." },
    { "Hi, my name is\nTom", 3, 9,  false, "Hi, my \nname is\nTom" },
    { "Hi, my name is\nTom", 1, 10, true,  "Hi, my \n..." },
    { "Hi, my name is\nTom", 2, 10, true,  "Hi, my \nname is\n..." },
    { "Hi, my name is\nTom", 3, 10, false, "Hi, my \nname is\nTom" },
    { "Hi, my name is\nTom", 1, 11, true,  "Hi, my \n..." },
    { "Hi, my name is\nTom", 2, 11, true,  "Hi, my \nname is\n..." },
    { "Hi, my name is\nTom", 3, 11, false, "Hi, my \nname is\nTom" },
    { "Hi, my name is\nTom", 1, 12, true,  "Hi, my \n..." },
    { "Hi, my name is\nTom", 2, 12, true,  "Hi, my \nname is\n..." },
    { "Hi, my name is\nTom", 3, 12, false, "Hi, my \nname is\nTom" },
    { "Hi, my name is\nTom", 1, 13, true,  "Hi, my name \n..." },
    { "Hi, my name is\nTom", 2, 13, true,  "Hi, my name \nis\n..." },
    { "Hi, my name is\nTom", 3, 13, false, "Hi, my name \nis\nTom" },
    { "Hi, my name is\nTom", 1, 20, true,  "Hi, my name is\n..." },
    { "Hi, my name is\nTom", 2, 20, false, "Hi, my name is\nTom" },
    { "Hi, my name is Tom",  1, 40, false, "Hi, my name is Tom" },
  };
  base::string16 output;
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].result,
              ElideRectangleString(UTF8ToUTF16(cases[i].input),
                                   cases[i].max_rows, cases[i].max_cols,
                                   true, &output));
    EXPECT_EQ(cases[i].output, UTF16ToUTF8(output));
  }
}

TEST(TextEliderTest, ElideRectangleStringNotStrict) {
  struct TestData {
    const char* input;
    int max_rows;
    int max_cols;
    bool result;
    const char* output;
  } cases[] = {
    { "", 0, 0, false, "" },
    { "", 1, 1, false, "" },
    { "Hi, my name_is\nDick", 0, 0,  true,  "..." },
    { "Hi, my name_is\nDick", 1, 0,  true,  "\n..." },
    { "Hi, my name_is\nDick", 0, 1,  true,  "..." },
    { "Hi, my name_is\nDick", 1, 1,  true,  "H\n..." },
    { "Hi, my name_is\nDick", 2, 1,  true,  "H\ni\n..." },
    { "Hi, my name_is\nDick", 3, 1,  true,  "H\ni\n,\n..." },
    { "Hi, my name_is\nDick", 4, 1,  true,  "H\ni\n,\n \n..." },
    { "Hi, my name_is\nDick", 5, 1,  true,  "H\ni\n,\n \nm\n..." },
    { "Hi, my name_is\nDick", 0, 2,  true,  "..." },
    { "Hi, my name_is\nDick", 1, 2,  true,  "Hi\n..." },
    { "Hi, my name_is\nDick", 2, 2,  true,  "Hi\n, \n..." },
    { "Hi, my name_is\nDick", 3, 2,  true,  "Hi\n, \nmy\n..." },
    { "Hi, my name_is\nDick", 4, 2,  true,  "Hi\n, \nmy\n n\n..." },
    { "Hi, my name_is\nDick", 5, 2,  true,  "Hi\n, \nmy\n n\nam\n..." },
    { "Hi, my name_is\nDick", 0, 3,  true,  "..." },
    { "Hi, my name_is\nDick", 1, 3,  true,  "Hi,\n..." },
    { "Hi, my name_is\nDick", 2, 3,  true,  "Hi,\n my\n..." },
    { "Hi, my name_is\nDick", 3, 3,  true,  "Hi,\n my\n na\n..." },
    { "Hi, my name_is\nDick", 4, 3,  true,  "Hi,\n my\n na\nme_\n..." },
    { "Hi, my name_is\nDick", 5, 3,  true,  "Hi,\n my\n na\nme_\nis\n..." },
    { "Hi, my name_is\nDick", 1, 4,  true,  "Hi, ..." },
    { "Hi, my name_is\nDick", 2, 4,  true,  "Hi, my n\n..." },
    { "Hi, my name_is\nDick", 3, 4,  true,  "Hi, my n\name_\n..." },
    { "Hi, my name_is\nDick", 4, 4,  true,  "Hi, my n\name_\nis\n..." },
    { "Hi, my name_is\nDick", 5, 4,  false, "Hi, my n\name_\nis\nDick" },
    { "Hi, my name_is\nDick", 1, 5,  true,  "Hi, ..." },
    { "Hi, my name_is\nDick", 2, 5,  true,  "Hi, my na\n..." },
    { "Hi, my name_is\nDick", 3, 5,  true,  "Hi, my na\nme_is\n..." },
    { "Hi, my name_is\nDick", 4, 5,  true,  "Hi, my na\nme_is\n\n..." },
    { "Hi, my name_is\nDick", 5, 5,  false, "Hi, my na\nme_is\n\nDick" },
    { "Hi, my name_is\nDick", 1, 6,  true,  "Hi, ..." },
    { "Hi, my name_is\nDick", 2, 6,  true,  "Hi, my nam\n..." },
    { "Hi, my name_is\nDick", 3, 6,  true,  "Hi, my nam\ne_is\n..." },
    { "Hi, my name_is\nDick", 4, 6,  false, "Hi, my nam\ne_is\nDick" },
    { "Hi, my name_is\nDick", 5, 6,  false, "Hi, my nam\ne_is\nDick" },
    { "Hi, my name_is\nDick", 1, 7,  true,  "Hi, ..." },
    { "Hi, my name_is\nDick", 2, 7,  true,  "Hi, my name\n..." },
    { "Hi, my name_is\nDick", 3, 7,  true,  "Hi, my name\n_is\n..." },
    { "Hi, my name_is\nDick", 4, 7,  false, "Hi, my name\n_is\nDick" },
    { "Hi, my name_is\nDick", 5, 7,  false, "Hi, my name\n_is\nDick" },
    { "Hi, my name_is\nDick", 1, 8,  true,  "Hi, my n\n..." },
    { "Hi, my name_is\nDick", 2, 8,  true,  "Hi, my n\name_is\n..." },
    { "Hi, my name_is\nDick", 3, 8,  false, "Hi, my n\name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 9,  true,  "Hi, my ..." },
    { "Hi, my name_is\nDick", 2, 9,  true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 3, 9,  false, "Hi, my name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 10, true,  "Hi, my ..." },
    { "Hi, my name_is\nDick", 2, 10, true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 3, 10, false, "Hi, my name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 11, true,  "Hi, my ..." },
    { "Hi, my name_is\nDick", 2, 11, true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 3, 11, false, "Hi, my name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 12, true,  "Hi, my ..." },
    { "Hi, my name_is\nDick", 2, 12, true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 3, 12, false, "Hi, my name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 13, true,  "Hi, my ..." },
    { "Hi, my name_is\nDick", 2, 13, true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 3, 13, false, "Hi, my name_is\nDick" },
    { "Hi, my name_is\nDick", 1, 20, true,  "Hi, my name_is\n..." },
    { "Hi, my name_is\nDick", 2, 20, false, "Hi, my name_is\nDick" },
    { "Hi, my name_is Dick",  1, 40, false, "Hi, my name_is Dick" },
  };
  base::string16 output;
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].result,
              ElideRectangleString(UTF8ToUTF16(cases[i].input),
                                   cases[i].max_rows, cases[i].max_cols,
                                   false, &output));
    EXPECT_EQ(cases[i].output, UTF16ToUTF8(output));
  }
}

TEST(TextEliderTest, ElideRectangleWide16) {
  // Two greek words separated by space.
  const base::string16 str(WideToUTF16(
      L"\x03a0\x03b1\x03b3\x03ba\x03cc\x03c3\x03bc\x03b9"
      L"\x03bf\x03c2\x0020\x0399\x03c3\x03c4\x03cc\x03c2"));
  const base::string16 out1(WideToUTF16(
      L"\x03a0\x03b1\x03b3\x03ba\n"
      L"\x03cc\x03c3\x03bc\x03b9\n"
      L"..."));
  const base::string16 out2(WideToUTF16(
      L"\x03a0\x03b1\x03b3\x03ba\x03cc\x03c3\x03bc\x03b9\x03bf\x03c2\x0020\n"
      L"\x0399\x03c3\x03c4\x03cc\x03c2"));
  base::string16 output;
  EXPECT_TRUE(ElideRectangleString(str, 2, 4, true, &output));
  EXPECT_EQ(out1, output);
  EXPECT_FALSE(ElideRectangleString(str, 2, 12, true, &output));
  EXPECT_EQ(out2, output);
}

TEST(TextEliderTest, ElideRectangleWide32) {
  // Four U+1D49C MATHEMATICAL SCRIPT CAPITAL A followed by space "aaaaa".
  const base::string16 str(UTF8ToUTF16(
      "\xF0\x9D\x92\x9C\xF0\x9D\x92\x9C\xF0\x9D\x92\x9C\xF0\x9D\x92\x9C"
      " aaaaa"));
  const base::string16 out(UTF8ToUTF16(
      "\xF0\x9D\x92\x9C\xF0\x9D\x92\x9C\xF0\x9D\x92\x9C\n"
      "\xF0\x9D\x92\x9C \naaa\n..."));
  base::string16 output;
  EXPECT_TRUE(ElideRectangleString(str, 3, 3, true, &output));
  EXPECT_EQ(out, output);
}

TEST(TextEliderTest, TruncateString) {
  base::string16 string = ASCIIToUTF16("foooooey    bxxxar baz");

  // Make sure it doesn't modify the string if length > string length.
  EXPECT_EQ(string, TruncateString(string, 100));

  // Test no characters.
  EXPECT_EQ(L"", UTF16ToWide(TruncateString(string, 0)));

  // Test 1 character.
  EXPECT_EQ(L"\x2026", UTF16ToWide(TruncateString(string, 1)));

  // Test adds ... at right spot when there is enough room to break at a
  // word boundary.
  EXPECT_EQ(L"foooooey\x2026", UTF16ToWide(TruncateString(string, 14)));

  // Test adds ... at right spot when there is not enough space in first word.
  EXPECT_EQ(L"f\x2026", UTF16ToWide(TruncateString(string, 2)));

  // Test adds ... at right spot when there is not enough room to break at a
  // word boundary.
  EXPECT_EQ(L"foooooey\x2026", UTF16ToWide(TruncateString(string, 11)));

  // Test completely truncates string if break is on initial whitespace.
  EXPECT_EQ(L"\x2026", UTF16ToWide(TruncateString(ASCIIToUTF16("   "), 2)));
}

}  // namespace gfx
