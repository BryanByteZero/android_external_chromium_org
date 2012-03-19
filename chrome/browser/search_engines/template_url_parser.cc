// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search_engines/template_url_parser.h"

#include <algorithm>
#include <map>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "libxml/parser.h"
#include "libxml/xmlwriter.h"

namespace {

//
// NOTE: libxml uses the UTF-8 encoding. As 0-127 of UTF-8 corresponds
// to that of char, the following names are all in terms of char. This avoids
// having to convert to wide, then do comparisons

// Defines for element names of the OSD document:
const char kURLElement[] = "Url";
const char kParamElement[] = "Param";
const char kShortNameElement[] = "ShortName";
const char kDescriptionElement[] = "Description";
const char kImageElement[] = "Image";
const char kOpenSearchDescriptionElement[] = "OpenSearchDescription";
const char kFirefoxSearchDescriptionElement[] = "SearchPlugin";
const char kLanguageElement[] = "Language";
const char kInputEncodingElement[] = "InputEncoding";

// Various XML attributes used.
const char kURLTypeAttribute[] = "type";
const char kURLTemplateAttribute[] = "template";
const char kImageTypeAttribute[] = "type";
const char kImageWidthAttribute[] = "width";
const char kImageHeightAttribute[] = "height";
const char kURLIndexOffsetAttribute[] = "indexOffset";
const char kURLPageOffsetAttribute[] = "pageOffset";
const char kParamNameAttribute[] = "name";
const char kParamValueAttribute[] = "value";
const char kParamMethodAttribute[] = "method";

// Mime type for search results.
const char kHTMLType[] = "text/html";

// Mime type for as you type suggestions.
const char kSuggestionType[] = "application/x-suggestions+json";

// Namespace identifier.
const char kOSDNS[] = "xmlns";

// The namespace for documents we understand.
const char kNameSpace[] = "http://a9.com/-/spec/opensearch/1.1/";

std::string XMLCharToString(const xmlChar* value) {
  return std::string((const char*)value);
}

// Returns true if input_encoding contains a valid input encoding string. This
// doesn't verify that we have a valid encoding for the string, just that the
// string contains characters that constitute a valid input encoding.
bool IsValidEncodingString(const std::string& input_encoding) {
  if (input_encoding.empty())
    return false;

  if (!IsAsciiAlpha(input_encoding[0]))
    return false;

  for (size_t i = 1, max = input_encoding.size(); i < max; ++i) {
    char c = input_encoding[i];
    if (!IsAsciiAlpha(c) && !IsAsciiDigit(c) && c != '.' && c != '_' &&
        c != '-') {
      return false;
    }
  }
  return true;
}

void AppendParamToQuery(const std::string& key,
                        const std::string& value,
                        std::string* query) {
  if (!query->empty())
    query->append("&");
  if (!key.empty()) {
    query->append(key);
    query->append("=");
  }
  query->append(value);
}

// Returns true if the ref is null, or the url wrapped by ref is
// valid with a spec of http/https.
bool IsHTTPRef(const TemplateURLRef* ref) {
  if (ref == NULL)
    return true;
  GURL url(ref->url());
  return (url.is_valid() && (url.SchemeIs(chrome::kHttpScheme) ||
                             url.SchemeIs(chrome::kHttpsScheme)));
}

// Returns true if the TemplateURL is legal. A legal TemplateURL is one
// where all URLs have a spec of http/https.
bool IsLegal(TemplateURL* url) {
  if (!IsHTTPRef(url->url()) || !IsHTTPRef(url->suggestions_url()))
    return false;
  // Make sure all the image refs are legal.
  const std::vector<TemplateURL::ImageRef>& image_refs = url->image_refs();
  for (size_t i = 0; i < image_refs.size(); i++) {
    GURL image_url(image_refs[i].url);
    if (!image_url.is_valid() ||
        !(image_url.SchemeIs(chrome::kHttpScheme) ||
          image_url.SchemeIs(chrome::kHttpsScheme))) {
      return false;
    }
  }
  return true;
}

}  // namespace


// TemplateURLParsingContext --------------------------------------------------

// To minimize memory overhead while parsing, a SAX style parser is used.
// TemplateURLParsingContext is used to maintain the state we're in the document
// while parsing.
class TemplateURLParsingContext {
 public:
  // Enum of the known element types.
  enum ElementType {
    UNKNOWN,
    OPEN_SEARCH_DESCRIPTION,
    URL,
    PARAM,
    SHORT_NAME,
    DESCRIPTION,
    IMAGE,
    LANGUAGE,
    INPUT_ENCODING,
  };

  enum Method {
    GET,
    POST
  };

  // Key/value of a Param node.
  typedef std::pair<std::string, std::string> Param;

  TemplateURLParsingContext(
      TemplateURLParser::ParameterFilter* parameter_filter,
      TemplateURL* url);

  static void StartElementImpl(void* ctx,
                               const xmlChar* name,
                               const xmlChar** atts);
  static void EndElementImpl(void* ctx, const xmlChar* name);
  static void CharactersImpl(void* ctx, const xmlChar* ch, int len);

  // Invoked when an element starts.
  void PushElement(const std::string& element);

  void PopElement();

  TemplateURL* template_url() { return url_; }

  void AddImageRef(const std::string& type, int width, int height);

  void EndImage();

  void SetImageURL(const GURL& url);

  void ResetString();

  void AppendString(const string16& string);

  const string16& GetString();

  void ResetExtraParams();

  void AddExtraParams(const std::string& key, const std::string& value);

  const std::vector<Param>& extra_params() const { return extra_params_; }

  void set_is_suggestion(bool value) { is_suggest_url_ = value; }
  bool is_suggestion() const { return is_suggest_url_; }

  TemplateURLParser::ParameterFilter* parameter_filter() const {
    return parameter_filter_;
  }

  void set_derive_image_from_url(bool derive_image_from_url) {
    derive_image_from_url_ = derive_image_from_url;
  }

  void set_method(Method method) { method_ = method; }
  Method method() { return method_; }

  void set_suggestion_method(Method method) { suggestion_method_ = method; }
  Method suggestion_method() { return suggestion_method_; }

  // Builds the image URL from the Template search URL if no image URL has been
  // set.
  void DeriveImageFromURL();

 private:
  // Key is UTF8 encoded.
  typedef std::map<std::string, ElementType> ElementNameToElementTypeMap;

  static void InitMapping();

  static void ParseURL(const xmlChar** atts,
                       TemplateURLParsingContext* context);
  static void ParseImage(const xmlChar** atts,
                         TemplateURLParsingContext* context);
  static void ParseParam(const xmlChar** atts,
                         TemplateURLParsingContext* context);
  static void ProcessURLParams(TemplateURLParsingContext* context);

  // Returns the current ElementType.
  ElementType GetKnownType();

  static ElementNameToElementTypeMap* kElementNameToElementTypeMap;

  // TemplateURL supplied to Read method. It's owned by the caller, so we
  // don't need to free it.
  TemplateURL* url_;

  std::vector<ElementType> elements_;
  scoped_ptr<TemplateURL::ImageRef> current_image_;

  // Character content for the current element.
  string16 string_;

  TemplateURLParser::ParameterFilter* parameter_filter_;

  // The list of parameters parsed in the Param nodes of a Url node.
  std::vector<Param> extra_params_;

  // The HTTP methods used.
  Method method_;
  Method suggestion_method_;

  // If true, we are currently parsing a suggest URL, otherwise it is an HTML
  // search.  Note that we don't need a stack as Url nodes cannot be nested.
  bool is_suggest_url_;

  // Whether we should derive the image from the URL (when images are data
  // URLs).
  bool derive_image_from_url_;

  DISALLOW_COPY_AND_ASSIGN(TemplateURLParsingContext);
};

// static
TemplateURLParsingContext::ElementNameToElementTypeMap*
    TemplateURLParsingContext::kElementNameToElementTypeMap = NULL;

TemplateURLParsingContext::TemplateURLParsingContext(
    TemplateURLParser::ParameterFilter* parameter_filter,
    TemplateURL* url)
    : url_(url),
      parameter_filter_(parameter_filter),
      method_(GET),
      suggestion_method_(GET),
      is_suggest_url_(false),
      derive_image_from_url_(false) {
  if (kElementNameToElementTypeMap == NULL)
    InitMapping();
}

// Removes the namespace from the specified |name|, ex: os:Url -> Url.
void PruneNamespace(std::string* name) {
  size_t index = name->find_first_of(":");
  if (index != std::string::npos)
    name->erase(0, index + 1);
}

// static
void TemplateURLParsingContext::StartElementImpl(void *ctx,
                                                 const xmlChar *name,
                                                 const xmlChar **atts) {
  TemplateURLParsingContext* context =
      reinterpret_cast<TemplateURLParsingContext*>(ctx);
  std::string node_name((const char*)name);
  PruneNamespace(&node_name);
  context->PushElement(node_name);
  switch (context->GetKnownType()) {
    case TemplateURLParsingContext::URL:
      context->ResetExtraParams();
      ParseURL(atts, context);
      break;
    case TemplateURLParsingContext::IMAGE:
      ParseImage(atts, context);
      break;
    case TemplateURLParsingContext::PARAM:
      ParseParam(atts, context);
      break;
    default:
      break;
  }
  context->ResetString();
}

// static
void TemplateURLParsingContext::EndElementImpl(void *ctx, const xmlChar *name) {
  TemplateURLParsingContext* context =
      reinterpret_cast<TemplateURLParsingContext*>(ctx);
  switch (context->GetKnownType()) {
    case TemplateURLParsingContext::SHORT_NAME:
      context->template_url()->set_short_name(context->GetString());
      break;
    case TemplateURLParsingContext::DESCRIPTION:
      context->template_url()->set_description(context->GetString());
      break;
    case TemplateURLParsingContext::IMAGE: {
      GURL image_url(UTF16ToUTF8(context->GetString()));
      if (image_url.SchemeIs(chrome::kDataScheme)) {
        // TODO (jcampan): bug 1169256: when dealing with data URL, we need to
        // decode the data URL in the renderer. For now, we'll just point to the
        // favicon from the URL.
        context->set_derive_image_from_url(true);
      } else {
        context->SetImageURL(image_url);
      }
      context->EndImage();
      break;
    }
    case TemplateURLParsingContext::LANGUAGE:
      context->template_url()->add_language(context->GetString());
      break;
    case TemplateURLParsingContext::INPUT_ENCODING: {
      std::string input_encoding = UTF16ToASCII(context->GetString());
      if (IsValidEncodingString(input_encoding))
        context->template_url()->add_input_encoding(input_encoding);
      break;
    }
    case TemplateURLParsingContext::URL:
      ProcessURLParams(context);
      break;
    default:
      break;
  }
  context->ResetString();
  context->PopElement();
}

string16 XMLCharToUTF16(const xmlChar* value, int length) {
  return UTF8ToUTF16(std::string((const char*)value, length));
}

// static
void TemplateURLParsingContext::CharactersImpl(void *ctx,
                                               const xmlChar *ch,
                                               int len) {
  TemplateURLParsingContext* context =
      reinterpret_cast<TemplateURLParsingContext*>(ctx);
  context->AppendString(XMLCharToUTF16(ch, len));
}

void TemplateURLParsingContext::PushElement(const std::string& element) {
  ElementType type;
  if (kElementNameToElementTypeMap->find(element) ==
      kElementNameToElementTypeMap->end()) {
    type = UNKNOWN;
  } else {
    type = (*kElementNameToElementTypeMap)[element];
  }
  elements_.push_back(type);
}

void TemplateURLParsingContext::PopElement() {
  elements_.pop_back();
}

void TemplateURLParsingContext::AddImageRef(const std::string& type,
                                            int width,
                                            int height) {
  if (width > 0 && height > 0)
    current_image_.reset(new TemplateURL::ImageRef(type, width, height));
}

void TemplateURLParsingContext::EndImage() {
  current_image_.reset();
}

void TemplateURLParsingContext::SetImageURL(const GURL& url) {
  if (current_image_.get()) {
    current_image_->url = url;
    url_->image_refs_.push_back(*current_image_);
    current_image_.reset();
  }
}

void TemplateURLParsingContext::ResetString() {
  string_.clear();
}

void TemplateURLParsingContext::AppendString(const string16& string) {
  string_ += string;
}

const string16& TemplateURLParsingContext::GetString() {
  return string_;
}

void TemplateURLParsingContext::ResetExtraParams() {
  extra_params_.clear();
}

void TemplateURLParsingContext::AddExtraParams(const std::string& key,
                                               const std::string& value) {
  if (parameter_filter_ && !parameter_filter_->KeepParameter(key, value))
    return;
  extra_params_.push_back(Param(key, value));
}

void TemplateURLParsingContext::DeriveImageFromURL() {
  if (derive_image_from_url_ &&
      url_->GetFaviconURL().is_empty() && url_->url()) {
    GURL url(url_->url()->url());  // More url's please...
    url_->SetFaviconURL(TemplateURL::GenerateFaviconURL(url));
  }
}

// static
void TemplateURLParsingContext::InitMapping() {
  kElementNameToElementTypeMap = new std::map<std::string, ElementType>;
  (*kElementNameToElementTypeMap)[kURLElement] = URL;
  (*kElementNameToElementTypeMap)[kParamElement] = PARAM;
  (*kElementNameToElementTypeMap)[kShortNameElement] = SHORT_NAME;
  (*kElementNameToElementTypeMap)[kDescriptionElement] = DESCRIPTION;
  (*kElementNameToElementTypeMap)[kImageElement] = IMAGE;
  (*kElementNameToElementTypeMap)[kOpenSearchDescriptionElement] =
      OPEN_SEARCH_DESCRIPTION;
  (*kElementNameToElementTypeMap)[kFirefoxSearchDescriptionElement] =
      OPEN_SEARCH_DESCRIPTION;
  (*kElementNameToElementTypeMap)[kLanguageElement] = LANGUAGE;
  (*kElementNameToElementTypeMap)[kInputEncodingElement] = INPUT_ENCODING;
}

// static
void TemplateURLParsingContext::ParseURL(const xmlChar** atts,
                                         TemplateURLParsingContext* context) {
  if (!atts)
    return;

  TemplateURL* turl = context->template_url();
  const xmlChar** attributes = atts;
  std::string template_url;
  bool is_post = false;
  bool is_html_url = false;
  bool is_suggest_url = false;
  int index_offset = 1;
  int page_offset = 1;

  while (*attributes) {
    std::string name(XMLCharToString(*attributes));
    const xmlChar* value = attributes[1];
    if (name == kURLTypeAttribute) {
      std::string type = XMLCharToString(value);
      is_html_url = (type == kHTMLType);
      is_suggest_url = (type == kSuggestionType);
    } else if (name == kURLTemplateAttribute) {
      template_url = XMLCharToString(value);
    } else if (name == kURLIndexOffsetAttribute) {
      base::StringToInt(XMLCharToString(value), &index_offset);
      index_offset = std::max(1, index_offset);
    } else if (name == kURLPageOffsetAttribute) {
      base::StringToInt(XMLCharToString(value), &page_offset);
      page_offset = std::max(1, page_offset);
    } else if (name == kParamMethodAttribute) {
      is_post = LowerCaseEqualsASCII(XMLCharToString(value), "post");
    }
    attributes += 2;
  }
  if (is_html_url) {
    turl->SetURL(template_url, index_offset, page_offset);
    context->set_is_suggestion(false);
    if (is_post)
      context->set_method(TemplateURLParsingContext::POST);
  } else if (is_suggest_url) {
    turl->SetSuggestionsURL(template_url, index_offset, page_offset);
    context->set_is_suggestion(true);
    if (is_post)
      context->set_suggestion_method(TemplateURLParsingContext::POST);
  }
}

// static
void TemplateURLParsingContext::ParseImage(const xmlChar** atts,
                                           TemplateURLParsingContext* context) {
  if (!atts)
    return;

  const xmlChar** attributes = atts;
  int width = 0;
  int height = 0;
  std::string type;
  while (*attributes) {
    std::string name(XMLCharToString(*attributes));
    const xmlChar* value = attributes[1];
    if (name == kImageTypeAttribute) {
      type = XMLCharToString(value);
    } else if (name == kImageWidthAttribute) {
      base::StringToInt(XMLCharToString(value), &width);
    } else if (name == kImageHeightAttribute) {
      base::StringToInt(XMLCharToString(value), &height);
    }
    attributes += 2;
  }
  if (width > 0 && height > 0 && !type.empty()) {
    // Valid Image URL.
    context->AddImageRef(type, width, height);
  }
}

// static
void TemplateURLParsingContext::ParseParam(const xmlChar** atts,
                                           TemplateURLParsingContext* context) {
  if (!atts)
    return;

  const xmlChar** attributes = atts;
  std::string key, value;
  while (*attributes) {
    std::string name(XMLCharToString(*attributes));
    const xmlChar* val = attributes[1];
    if (name == kParamNameAttribute) {
      key = XMLCharToString(val);
    } else if (name == kParamValueAttribute) {
      value = XMLCharToString(val);
    }
    attributes += 2;
  }
  if (!key.empty())
    context->AddExtraParams(key, value);
}

// static
void TemplateURLParsingContext::ProcessURLParams(
    TemplateURLParsingContext* context) {
  TemplateURL* t_url = context->template_url();
  const TemplateURLRef* t_url_ref =
      context->is_suggestion() ? t_url->suggestions_url() :
                                 t_url->url();
  if (!t_url_ref)
    return;

  if (!context->parameter_filter() && context->extra_params().empty())
    return;

  GURL url(t_url_ref->url());
  // If there is a parameter filter, parse the existing URL and remove any
  // unwanted parameter.
  TemplateURLParser::ParameterFilter* filter = context->parameter_filter();
  std::string new_query;
  bool modified = false;
  if (filter) {
    url_parse::Component query = url.parsed_for_possibly_invalid_spec().query;
    url_parse::Component key, value;
    const char* url_spec = url.spec().c_str();
    while (url_parse::ExtractQueryKeyValue(url_spec, &query, &key, &value)) {
      std::string key_str(url_spec, key.begin, key.len);
      std::string value_str(url_spec, value.begin, value.len);
      if (filter->KeepParameter(key_str, value_str)) {
        AppendParamToQuery(key_str, value_str, &new_query);
      } else {
        modified = true;
      }
    }
  }
  if (!modified)
    new_query = url.query();

  // Add the extra parameters if any.
  const std::vector<TemplateURLParsingContext::Param>& params =
      context->extra_params();
  if (!params.empty()) {
    modified = true;
    std::vector<TemplateURLParsingContext::Param>::const_iterator iter;
    for (iter = params.begin(); iter != params.end(); ++iter)
      AppendParamToQuery(iter->first, iter->second, &new_query);
  }

  if (modified) {
    GURL::Replacements repl;
    repl.SetQueryStr(new_query);
    url = url.ReplaceComponents(repl);
    if (context->is_suggestion()) {
      t_url->SetSuggestionsURL(url.spec(),
                               t_url_ref->index_offset(),
                               t_url_ref->page_offset());
    } else {
      t_url->SetURL(url.spec(),
                    t_url_ref->index_offset(),
                    t_url_ref->page_offset());
    }
  }
}

TemplateURLParsingContext::ElementType
    TemplateURLParsingContext::GetKnownType() {
  if (elements_.size() == 2 && elements_[0] == OPEN_SEARCH_DESCRIPTION)
    return elements_[1];

  // We only expect PARAM nodes under the Url node
  if (elements_.size() == 3 && elements_[0] == OPEN_SEARCH_DESCRIPTION &&
      elements_[1] == URL && elements_[2] == PARAM)
    return PARAM;

  return UNKNOWN;
}


// TemplateURLParser ----------------------------------------------------------

// static
bool TemplateURLParser::Parse(const unsigned char* data, size_t length,
                              TemplateURLParser::ParameterFilter* param_filter,
                              TemplateURL* url) {
  DCHECK(url);
  // xmlSubstituteEntitiesDefault(1) makes it so that &amp; isn't mapped to
  // &#38; . Unfortunately xmlSubstituteEntitiesDefault effects global state.
  // If this becomes problematic we'll need to provide our own entity
  // type for &amp;, or strip out &#34; by hand after parsing.
  int last_sub_entities_value = xmlSubstituteEntitiesDefault(1);
  TemplateURLParsingContext context(param_filter, url);
  xmlSAXHandler sax_handler;
  memset(&sax_handler, 0, sizeof(sax_handler));
  sax_handler.startElement = &TemplateURLParsingContext::StartElementImpl;
  sax_handler.endElement = &TemplateURLParsingContext::EndElementImpl;
  sax_handler.characters = &TemplateURLParsingContext::CharactersImpl;
  xmlSAXUserParseMemory(&sax_handler, &context,
                        reinterpret_cast<const char*>(data),
                        static_cast<int>(length));
  xmlSubstituteEntitiesDefault(last_sub_entities_value);
  // If the image was a data URL, use the favicon from the search URL instead.
  // (see TODO inEndElementImpl()).
  context.DeriveImageFromURL();

  // TODO(jcampan): http://b/issue?id=1196285 we do not support search engines
  //                that use POST yet.
  if (context.method() == TemplateURLParsingContext::POST)
    return false;
  if (context.suggestion_method() == TemplateURLParsingContext::POST)
    url->SetSuggestionsURL("", 0, 0);

  if (!url->short_name().empty() && !url->description().empty()) {
    // So far so good, make sure the urls are http.
    return IsLegal(url);
  }
  return false;
}
