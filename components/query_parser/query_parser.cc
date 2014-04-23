// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/query_parser/query_parser.h"

#include <algorithm>

#include "base/compiler_specific.h"
#include "base/i18n/break_iterator.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"

namespace query_parser {
namespace {

// Returns true if |mp1.first| is less than |mp2.first|. This is used to
// sort match positions.
int CompareMatchPosition(const Snippet::MatchPosition& mp1,
                         const Snippet::MatchPosition& mp2) {
  return mp1.first < mp2.first;
}

// Returns true if |mp2| intersects |mp1|. This is intended for use by
// CoalesceMatchesFrom and isn't meant as a general intersection comparison
// function.
bool SnippetIntersects(const Snippet::MatchPosition& mp1,
                       const Snippet::MatchPosition& mp2) {
  return mp2.first >= mp1.first && mp2.first <= mp1.second;
}

// Coalesces match positions in |matches| after index that intersect the match
// position at |index|.
void CoalesceMatchesFrom(size_t index, Snippet::MatchPositions* matches) {
  Snippet::MatchPosition& mp = (*matches)[index];
  for (Snippet::MatchPositions::iterator i = matches->begin() + index + 1;
       i != matches->end(); ) {
    if (SnippetIntersects(mp, *i)) {
      mp.second = std::max(mp.second, i->second);
      i = matches->erase(i);
    } else {
      return;
    }
  }
}

// Returns true if the character is considered a quote.
bool IsQueryQuote(wchar_t ch) {
  return ch == '"' ||
         ch == 0xab ||    // left pointing double angle bracket
         ch == 0xbb ||    // right pointing double angle bracket
         ch == 0x201c ||  // left double quotation mark
         ch == 0x201d ||  // right double quotation mark
         ch == 0x201e;    // double low-9 quotation mark
}

}  // namespace

// Inheritance structure:
// Queries are represented as trees of QueryNodes.
// QueryNodes are either a collection of subnodes (a QueryNodeList)
// or a single word (a QueryNodeWord).

// A QueryNodeWord is a single word in the query.
class QueryNodeWord : public QueryNode {
 public:
  explicit QueryNodeWord(const base::string16& word);
  virtual ~QueryNodeWord();

  const base::string16& word() const { return word_; }

  void set_literal(bool literal) { literal_ = literal; }

  // QueryNode:
  virtual int AppendToSQLiteQuery(base::string16* query) const OVERRIDE;
  virtual bool IsWord() const OVERRIDE;
  virtual bool Matches(const base::string16& word, bool exact) const OVERRIDE;
  virtual bool HasMatchIn(
      const QueryWordVector& words,
      Snippet::MatchPositions* match_positions) const OVERRIDE;
  virtual bool HasMatchIn(
      const QueryWordVector& words) const OVERRIDE;
  virtual void AppendWords(std::vector<base::string16>* words) const OVERRIDE;

 private:
  base::string16 word_;
  bool literal_;

  DISALLOW_COPY_AND_ASSIGN(QueryNodeWord);
};

QueryNodeWord::QueryNodeWord(const base::string16& word)
    : word_(word),
      literal_(false) {}

QueryNodeWord::~QueryNodeWord() {}

int QueryNodeWord::AppendToSQLiteQuery(base::string16* query) const {
  query->append(word_);

  // Use prefix search if we're not literal and long enough.
  if (!literal_ && QueryParser::IsWordLongEnoughForPrefixSearch(word_))
    *query += L'*';
  return 1;
}

bool QueryNodeWord::IsWord() const {
  return true;
}

bool QueryNodeWord::Matches(const base::string16& word, bool exact) const {
  if (exact || !QueryParser::IsWordLongEnoughForPrefixSearch(word_))
    return word == word_;
  return word.size() >= word_.size() &&
         (word_.compare(0, word_.size(), word, 0, word_.size()) == 0);
}

bool QueryNodeWord::HasMatchIn(const QueryWordVector& words,
                               Snippet::MatchPositions* match_positions) const {
  bool matched = false;
  for (size_t i = 0; i < words.size(); ++i) {
    if (Matches(words[i].word, false)) {
      size_t match_start = words[i].position;
      match_positions->push_back(
          Snippet::MatchPosition(match_start,
                                 match_start + static_cast<int>(word_.size())));
      matched = true;
    }
  }
  return matched;
}

bool QueryNodeWord::HasMatchIn(const QueryWordVector& words) const {
  for (size_t i = 0; i < words.size(); ++i) {
    if (Matches(words[i].word, false))
      return true;
  }
  return false;
}

void QueryNodeWord::AppendWords(std::vector<base::string16>* words) const {
  words->push_back(word_);
}

// A QueryNodeList has a collection of QueryNodes which are deleted in the end.
class QueryNodeList : public QueryNode {
 public:
  QueryNodeList();
  virtual ~QueryNodeList();

  QueryNodeStarVector* children() { return &children_; }

  void AddChild(QueryNode* node);

  // Remove empty subnodes left over from other parsing.
  void RemoveEmptySubnodes();

  // QueryNode:
  virtual int AppendToSQLiteQuery(base::string16* query) const OVERRIDE;
  virtual bool IsWord() const OVERRIDE;
  virtual bool Matches(const base::string16& word, bool exact) const OVERRIDE;
  virtual bool HasMatchIn(
      const QueryWordVector& words,
      Snippet::MatchPositions* match_positions) const OVERRIDE;
  virtual bool HasMatchIn(const QueryWordVector& words) const OVERRIDE;
  virtual void AppendWords(std::vector<base::string16>* words) const OVERRIDE;

 protected:
  int AppendChildrenToString(base::string16* query) const;

  QueryNodeStarVector children_;

 private:
  DISALLOW_COPY_AND_ASSIGN(QueryNodeList);
};

QueryNodeList::QueryNodeList() {}

QueryNodeList::~QueryNodeList() {
  STLDeleteElements(&children_);
}

void QueryNodeList::AddChild(QueryNode* node) {
  children_.push_back(node);
}

void QueryNodeList::RemoveEmptySubnodes() {
  for (size_t i = 0; i < children_.size(); ++i) {
    if (children_[i]->IsWord())
      continue;

    QueryNodeList* list_node = static_cast<QueryNodeList*>(children_[i]);
    list_node->RemoveEmptySubnodes();
    if (list_node->children()->empty()) {
      children_.erase(children_.begin() + i);
      --i;
      delete list_node;
    }
  }
}

int QueryNodeList::AppendToSQLiteQuery(base::string16* query) const {
  return AppendChildrenToString(query);
}

bool QueryNodeList::IsWord() const {
  return false;
}

bool QueryNodeList::Matches(const base::string16& word, bool exact) const {
  NOTREACHED();
  return false;
}

bool QueryNodeList::HasMatchIn(const QueryWordVector& words,
                               Snippet::MatchPositions* match_positions) const {
  NOTREACHED();
  return false;
}

bool QueryNodeList::HasMatchIn(const QueryWordVector& words) const {
  NOTREACHED();
  return false;
}

void QueryNodeList::AppendWords(std::vector<base::string16>* words) const {
  for (size_t i = 0; i < children_.size(); ++i)
    children_[i]->AppendWords(words);
}

int QueryNodeList::AppendChildrenToString(base::string16* query) const {
  int num_words = 0;
  for (QueryNodeStarVector::const_iterator node = children_.begin();
       node != children_.end(); ++node) {
    if (node != children_.begin())
      query->push_back(L' ');
    num_words += (*node)->AppendToSQLiteQuery(query);
  }
  return num_words;
}

// A QueryNodePhrase is a phrase query ("quoted").
class QueryNodePhrase : public QueryNodeList {
 public:
  QueryNodePhrase();
  virtual ~QueryNodePhrase();

  // QueryNodeList:
  virtual int AppendToSQLiteQuery(base::string16* query) const OVERRIDE;
  virtual bool HasMatchIn(
      const QueryWordVector& words,
      Snippet::MatchPositions* match_positions) const OVERRIDE;
  virtual bool HasMatchIn(const QueryWordVector& words) const OVERRIDE;

 private:
  bool MatchesAll(const QueryWordVector& words,
                  const QueryWord** first_word,
                  const QueryWord** last_word) const;
  DISALLOW_COPY_AND_ASSIGN(QueryNodePhrase);
};

QueryNodePhrase::QueryNodePhrase() {}

QueryNodePhrase::~QueryNodePhrase() {}

int QueryNodePhrase::AppendToSQLiteQuery(base::string16* query) const {
  query->push_back(L'"');
  int num_words = AppendChildrenToString(query);
  query->push_back(L'"');
  return num_words;
}

bool QueryNodePhrase::MatchesAll(const QueryWordVector& words,
                                 const QueryWord** first_word,
                                 const QueryWord** last_word) const {
  if (words.size() < children_.size())
    return false;

  for (size_t i = 0, max = words.size() - children_.size() + 1; i < max; ++i) {
    bool matched_all = true;
    for (size_t j = 0; j < children_.size(); ++j) {
      if (!children_[j]->Matches(words[i + j].word, true)) {
        matched_all = false;
        break;
      }
    }
    if (matched_all) {
      *first_word = &words[i];
      *last_word = &words[i + children_.size() - 1];
      return true;
    }
  }
  return false;
}

bool QueryNodePhrase::HasMatchIn(
    const QueryWordVector& words,
    Snippet::MatchPositions* match_positions) const {
  const QueryWord* first_word;
  const QueryWord* last_word;

  if (MatchesAll(words, &first_word, &last_word)) {
    match_positions->push_back(
        Snippet::MatchPosition(first_word->position,
                               last_word->position + last_word->word.length()));
      return true;
  }
  return false;
}

bool QueryNodePhrase::HasMatchIn(const QueryWordVector& words) const {
  const QueryWord* first_word;
  const QueryWord* last_word;
  return MatchesAll(words, &first_word, &last_word);
}

QueryParser::QueryParser() {}

// static
bool QueryParser::IsWordLongEnoughForPrefixSearch(const base::string16& word) {
  DCHECK(!word.empty());
  size_t minimum_length = 3;
  // We intentionally exclude Hangul Jamos (both Conjoining and compatibility)
  // because they 'behave like' Latin letters. Moreover, we should
  // normalize the former before reaching here.
  if (0xAC00 <= word[0] && word[0] <= 0xD7A3)
    minimum_length = 2;
  return word.size() >= minimum_length;
}

int QueryParser::ParseQuery(const base::string16& query,
                            base::string16* sqlite_query) {
  QueryNodeList root;
  if (!ParseQueryImpl(query, &root))
    return 0;
  return root.AppendToSQLiteQuery(sqlite_query);
}

void QueryParser::ParseQueryWords(const base::string16& query,
                                  std::vector<base::string16>* words) {
  QueryNodeList root;
  if (!ParseQueryImpl(query, &root))
    return;
  root.AppendWords(words);
}

void QueryParser::ParseQueryNodes(const base::string16& query,
                                  QueryNodeStarVector* nodes) {
  QueryNodeList root;
  if (ParseQueryImpl(base::i18n::ToLower(query), &root))
    nodes->swap(*root.children());
}

bool QueryParser::DoesQueryMatch(const base::string16& text,
                                 const QueryNodeStarVector& query_nodes,
                                 Snippet::MatchPositions* match_positions) {
  if (query_nodes.empty())
    return false;

  QueryWordVector query_words;
  base::string16 lower_text = base::i18n::ToLower(text);
  ExtractQueryWords(lower_text, &query_words);

  if (query_words.empty())
    return false;

  Snippet::MatchPositions matches;
  for (size_t i = 0; i < query_nodes.size(); ++i) {
    if (!query_nodes[i]->HasMatchIn(query_words, &matches))
      return false;
  }
  if (lower_text.length() != text.length()) {
    // The lower case string differs from the original string. The matches are
    // meaningless.
    // TODO(sky): we need a better way to align the positions so that we don't
    // completely punt here.
    match_positions->clear();
  } else {
    SortAndCoalesceMatchPositions(&matches);
    match_positions->swap(matches);
  }
  return true;
}

bool QueryParser::DoesQueryMatch(const QueryWordVector& query_words,
                                 const QueryNodeStarVector& query_nodes) {
  if (query_nodes.empty() || query_words.empty())
    return false;

  for (size_t i = 0; i < query_nodes.size(); ++i) {
    if (!query_nodes[i]->HasMatchIn(query_words))
      return false;
  }
  return true;
}

bool QueryParser::ParseQueryImpl(const base::string16& query,
                                 QueryNodeList* root) {
  base::i18n::BreakIterator iter(query, base::i18n::BreakIterator::BREAK_WORD);
  // TODO(evanm): support a locale here
  if (!iter.Init())
    return false;

  // To handle nesting, we maintain a stack of QueryNodeLists.
  // The last element (back) of the stack contains the current, deepest node.
  std::vector<QueryNodeList*> query_stack;
  query_stack.push_back(root);

  bool in_quotes = false;  // whether we're currently in a quoted phrase
  while (iter.Advance()) {
    // Just found a span between 'prev' (inclusive) and 'pos' (exclusive). It
    // is not necessarily a word, but could also be a sequence of punctuation
    // or whitespace.
    if (iter.IsWord()) {
      QueryNodeWord* word_node = new QueryNodeWord(iter.GetString());
      if (in_quotes)
        word_node->set_literal(true);
      query_stack.back()->AddChild(word_node);
    } else {  // Punctuation.
      if (IsQueryQuote(query[iter.prev()])) {
        if (!in_quotes) {
          QueryNodeList* quotes_node = new QueryNodePhrase;
          query_stack.back()->AddChild(quotes_node);
          query_stack.push_back(quotes_node);
          in_quotes = true;
        } else {
          query_stack.pop_back();  // Stop adding to the quoted phrase.
          in_quotes = false;
        }
      }
    }
  }

  root->RemoveEmptySubnodes();
  return true;
}

void QueryParser::ExtractQueryWords(const base::string16& text,
                                    QueryWordVector* words) {
  base::i18n::BreakIterator iter(text, base::i18n::BreakIterator::BREAK_WORD);
  // TODO(evanm): support a locale here
  if (!iter.Init())
    return;

  while (iter.Advance()) {
    // Just found a span between 'prev' (inclusive) and 'pos' (exclusive). It
    // is not necessarily a word, but could also be a sequence of punctuation
    // or whitespace.
    if (iter.IsWord()) {
      base::string16 word = iter.GetString();
      if (!word.empty()) {
        words->push_back(QueryWord());
        words->back().word = word;
        words->back().position = iter.prev();
     }
    }
  }
}

// static
void QueryParser::SortAndCoalesceMatchPositions(
    Snippet::MatchPositions* matches) {
  std::sort(matches->begin(), matches->end(), &CompareMatchPosition);
  // WARNING: we don't use iterator here as CoalesceMatchesFrom may remove
  // from matches.
  for (size_t i = 0; i < matches->size(); ++i)
    CoalesceMatchesFrom(i, matches);
}

}  // namespace query_parser
