// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This file is the controller for generating extension
 * doc pages.
 *
 * It expects to have available via XHR (relative path):
 *   1) API_TEMPLATE which is the main template for the api pages.
 *   2) The files located at MODULE_SCHEMAS which are shared with the extension
 *      system and defines the methods and events contained in one api.
 *   3) (Possibly) A static version of the current page url in /static/. I.e.
 *      if called as ../foo.html, it will look for ../static/foo.html.
 *
 * The "shell" page may have a renderering already contained within it so that
 * the docs can be indexed.
 *
 */

var API_TEMPLATE_EXTENSIONS = '../template/api_template.html';
var API_TEMPLATE_APPS = '../template/api_template_apps.html';
var MODULE_SCHEMAS = [
  '../../api/alarms.json',  // autogenerated
  '../../api/app_window.json',  // autogenerated
  '../../api/bookmarks.json',
  '../../api/browser_action.json',
  '../../api/browsing_data.json',
  '../../api/chrome_auth_private.json',
  '../../api/chromeos_info_private.json',
  '../../api/content_settings.json',
  '../../api/context_menus.json',
  '../../api/cookies.json',
  '../../api/debugger.json',
  '../../api/declarative_web_request.json',
  '../../api/devtools.json',
  '../../api/downloads.json',  // autogenerated
  '../../api/events.json',
  '../../api/experimental_accessibility.json',
  '../../api/experimental_app.json',
  '../../api/experimental_bluetooth.json',  // autogenerated
  '../../api/experimental_bookmark_manager.json',
  '../../api/experimental_discovery.json',  // autogenerated
  '../../api/experimental_identity.json',  // autogenerated
  '../../api/experimental_infobars.json',
  '../../api/experimental_input_virtual_keyboard.json',
  '../../api/experimental_keybinding.json',
  '../../api/experimental_media_galleries.json',  // autogenerated
  '../../api/experimental_offscreen_tabs.json',
  '../../api/experimental_processes.json',
  '../../api/experimental_rlz.json',
  '../../api/experimental_serial.json',  // autogenerated
  '../../api/experimental_speech_input.json',
  '../../api/experimental_socket.json',  // autogenerated
  '../../api/experimental_usb.json',  // autogenerated
  '../../api/extension.json',
  '../../api/file_browser_handler.json',
  '../../api/file_browser_private.json',
  '../../api/file_system.json',  // autogenerated
  '../../api/font_settings.json',
  '../../api/history.json',
  '../../api/i18n.json',
  '../../api/idle.json',
  '../../api/input_ime.json',
  '../../api/input_method_private.json',
  '../../api/managed_mode_private.json',
  '../../api/management.json',
  '../../api/media_galleries.json',  // autogenerated
  '../../api/media_player_private.json',
  '../../api/metrics_private.json',
  '../../api/omnibox.json',
  '../../api/page_action.json',
  '../../api/page_actions.json',
  '../../api/page_capture.json',
  '../../api/permissions.json',
  '../../api/privacy.json',
  '../../api/proxy.json',
  '../../api/runtime.json',
  '../../api/script_badge.json',
  '../../api/storage.json',
  '../../api/system_private.json',
  '../../api/tabs.json',
  '../../api/test.json',
  '../../api/top_sites.json',
  '../../api/tts.json',
  '../../api/tts_engine.json',
  '../../api/types.json',
  '../../api/web_navigation.json',
  '../../api/web_request.json',
  '../../api/web_socket_proxy_private.json',
  '../../api/webstore.json',
  '../../api/webstore_private.json',
  '../../api/windows.json',
]
var PERMISSION_FEATURES = '../../api/_permission_features.json';
var DEVTOOLS_SCHEMA = '../../api/devtools_api.json';
var USE_DEVTOOLS_SCHEMA =
  /devtools[^/]*\.html/.test(location.pathname);
var API_MODULE_PREFIX = 'chrome.';
var SAMPLES = '../samples.json';
var REQUEST_TIMEOUT = 2000;

function staticResource(name) { return '../static/' + name + '.html'; }

// Base name of this page. (i.e. 'tabs', 'overview', etc...).
var pageBase;

// Data to feed as context into the template.
var pageData = {};

// The full extension api schema
var schema;

// List of Chrome extension samples.
var samples;

// Mappings of api calls to URLs
var apiMapping;

// The current module for this page (if this page is an api module);
var module;

// Mapping from typeId to module.
var typeModule = {};

// Mapping from typeId to type.
var typeIdType = {};

// Auto-created page name as default
var pageName;

// If this page is an apiModule, the name of the api module
var apiModuleName;

// Permission information according to PERMISSION_FEATURES.
var permissionFeatures = {};

// Visits each item in the list in-order. Stops when f returns any truthy
// value and returns that node.
Array.prototype.select = function(f) {
  for (var i = 0; i < this.length; i++) {
    if (f(this[i], i))
      return this[i];
  }
}

// Assigns all keys & values of |obj2| to |obj1|.
function extend(obj, obj2) {
  for (var k in obj2) {
    obj[k] = obj2[k];
  }
}

/*
 * Main entry point for composing the page. It will fetch its template,
 * the extension api, and attempt to fetch the matching static content.
 * It will insert the static content, if any, prepare its pageData then
 * render the template from |pageData|.
 */
function renderPage() {
  // The page name minus the '.html' extension.
  pageBase = document.location.href.match(/\/([^\/]*)\.html/)[1];
  if (!pageBase) {
    alert('Empty page name for: ' + document.location.href);
    return;
  }

  pageName = pageBase.replace(/([A-Z])/g, ' $1');
  pageName = pageName.substring(0, 1).toUpperCase() + pageName.substring(1);

  // TODO(aa): Ugh, this is horrible. FIXME.
  var docFamily = location.pathname.split("/");
  docFamily = docFamily[docFamily.length - 2];
  if (docFamily != "extensions" && docFamily != "apps")
    docFamily = "";

  var apiTemplate = docFamily == "extensions" ?
      API_TEMPLATE_EXTENSIONS : API_TEMPLATE_APPS;

  document.body.setAttribute("doc-family", docFamily);

  // Fetch the api template and insert into the <body>.
  fetchContent(apiTemplate, function(templateContent) {
    document.getElementsByTagName('body')[0].innerHTML = templateContent;
    fetchStatic();
  }, function(error) {
    alert('Failed to load ' + API_TEMPLATE + '. ' + error);
  });
}

function fetchStatic() {
  // Fetch the static content and insert into the 'static' <div>.
  fetchContent(staticResource(pageBase), function(overviewContent) {
    document.getElementById('static').innerHTML = overviewContent;
    fetchPermissionFeatures();
  }, function(error) {
    // Not fatal. Some api pages may not have matching static content.
    fetchPermissionFeatures();
  });
}

function fetchPermissionFeatures() {
  // Fetch the permissionFeatures of features.
  fetchContent(PERMISSION_FEATURES, function(content) {
    permissionFeatures = JSON.parse(JSON.minify(content));
    fetchSchema();
  }, function(error) {
    alert('Failed to load ' + PERMISSION_FEATURES + '. ' + error);
  });
}

function fetchSchema() {
  // Now the page is composed with the authored content, we fetch the schema
  // and populate the templates.
  var is_experimental_index = /\/experimental\.html$/.test(location.pathname);

  var schemas_to_retrieve = [];
  if (!USE_DEVTOOLS_SCHEMA || is_experimental_index)
    schemas_to_retrieve = schemas_to_retrieve.concat(MODULE_SCHEMAS);
  if (USE_DEVTOOLS_SCHEMA || is_experimental_index)
    schemas_to_retrieve.push(DEVTOOLS_SCHEMA);

  var schemas_retrieved = 0;
  schema = [];

  function qualifyRefs(namespace, obj) {
    if (typeof(obj) == "object") {
      for (var i in obj) {
        if (typeof(obj[i]) == "object") {
          obj[i] = qualifyRefs(namespace, obj[i]);
        } else if (i == "$ref") {
          if (obj[i].indexOf('.') == -1) {
            obj[i] = namespace + '.' + obj[i];
          }
        }
      }
    }
    return obj;
  }

  function qualifyTypes(schema) {
    schema.forEach(function(mod) {
      if (mod.types) {
        mod.types.forEach(function(type) {
          type.prettyId = type.id;
          if (type.prettyId.indexOf(mod.namespace) == 0) {
            type.prettyId = type.prettyId.substring(mod.namespace.length + 1);
          }
          if (type.id.indexOf(mod.namespace) != 0) {
            type.id = mod.namespace + '.' + type.id;
          }
          typeModule[type.id] = mod;
          typeIdType[type.id] = type;
        });
      }
      mod = qualifyRefs(mod.namespace, mod);
    });
    return schema;
  }

  function onSchemaContent(content) {
    if (content)
      schema = schema.concat(qualifyTypes(JSON.parse(JSON.minify(content))));
    if (++schemas_retrieved < schemas_to_retrieve.length)
      return;
    if (pageName.toLowerCase() == 'samples') {
      fetchSamples();
    } else {
      renderTemplate();
    }
  }

  schemas_to_retrieve.forEach(function(schema_path) {
    fetchContent(schema_path, onSchemaContent, function(path, error) {
      console.error('Failed to load schema', path, error);
      onSchemaContent();
    });
  });
}

function fetchSamples() {
  // If we're rendering the samples directory, fetch the samples manifest.
  fetchContent(SAMPLES, function(sampleManifest) {
    var data = JSON.parse(JSON.minify(sampleManifest));
    samples = data.samples;
    apiMapping = data.api;
    renderTemplate();
  }, function(error) {
    renderTemplate();
  });
}

/**
 * Fetches |url| and returns its text contents from the xhr.responseText in
 * onSuccess(content)
 */
function fetchContent(url, onSuccess, onError) {
  var localUrl = url;
  var xhr = new XMLHttpRequest();
  var abortTimerId = window.setTimeout(function() {
    xhr.abort();
    console.log('XHR Timed out');
  }, REQUEST_TIMEOUT);

  function handleError(error) {
    window.clearTimeout(abortTimerId);
    if (onError) {
      onError(error);
      // Some cases result in multiple error handings. Only fire the callback
      // once.
      onError = undefined;
    }
  }

  try {
    xhr.onreadystatechange = function(){
      if (xhr.readyState == 4) {
        if (xhr.status < 300 && xhr.responseText) {
          window.clearTimeout(abortTimerId);
          onSuccess(xhr.responseText);
        } else {
          handleError('Failure to fetch content: ' + xhr.status);
        }
      }
    }

    xhr.onerror = handleError;

    xhr.open('GET', url, true);
    xhr.send(null);
  } catch(e) {
    console.log('ex: ' + e);
    console.error('exception: ' + e);
    handleError();
  }
}

function renderTemplate() {
  schema.forEach(function(mod) {
    if (mod.namespace == pageBase) {
      // Do not render page for modules which have documentation disabled.
      if (disableDocs(mod))
        return;
      // This page is an api page. Setup types and apiDefinition.
      module = mod;
      apiModuleName = API_MODULE_PREFIX + module.namespace;
      pageData.apiDefinition = module;
    }

    if (mod.types) {
      mod.types.forEach(function(type) {
        typeModule[type.id] = mod;
      });
    }
  });

  /**
   * Special pages like the samples gallery may want to modify their template
   * data to include additional information.  This hook allows a page template
   * to specify code that runs in the context of the api_page_generator.js
   * file before the jstemplate is rendered.
   *
   * To specify such code, the page template should include a script block with
   * a type of "text/prerenderjs" containing the code to be executed.  Note that
   * linking to an external file is not supported - code must be accessible
   * via the script block's innerText property.
   *
   * Code that is run this way may modify the data sent to jstemplate by
   * modifying the window.pageData variable.  This code will also have access
   * to any methods declared in the api_page_generator.js file.  The code
   * does not need to return any specific value to function.
   *
   * Note that code specified in this manner will be removed before the
   * template is rendered, and will therefore not be exposed to the end user
   * in the final rendered template.
   */
  var preRender = document.querySelectorAll('script[type="text/prerenderjs"]');
  Array.prototype.slice.call(preRender).forEach(function(element) {
    element.parentElement.removeChild(element);
    eval(element.innerText);
  });

  // Render to template
  var input = new JsEvalContext(pageData);
  var output = document.getElementsByTagName('body')[0];
  jstProcess(input, output);

  selectCurrentPageOnLeftNav();

  // Set a `meta` description if the page we're currently generating has a
  // module name.
  // TODO(mkwst): Come up with something clever for the other types of pages.
  if (getModuleName()) {
    var m = document.createElement('meta');
    var desc = 'Documentation for the ' + getModuleName() +
               ' module, which is part of the Google Chrome ' +
               ' extension APIs.';
    m.setAttribute('name', 'description');
    m.setAttribute('content', desc);
    document.head.appendChild(m);
  }

  document.title = getPageTitle();
  // Show
  if (window.postRender)
    window.postRender();

  if (parent && parent.done)
    parent.done();
}

function cleanupJstemplateMess(root) {
  var jsattributes = ['jscontent', 'jsselect', 'jsdisplay', 'transclude',
                      'jsvalues', 'jsvars', 'jseval', 'jsskip', 'jstcache',
                      'jsinstance'];

  var nodes = root.getElementsByTagName('*');
  var displayNone = [];

  for (var i = 0; i < nodes.length; i++) {
    var n = nodes[i]

    // Delete nodes which are hidden. There are lots of these since jsdisplay
    // just hides nodes, not deletes them.
    if (n.style && n.style.display === 'none') {
      displayNone.push(n);
      continue;
    }

    // Delete empty style attributes (this can happen when jsdisplay causes
    // display values other than 'none').
    var styleAttribute = n.getAttribute('style');
    if (typeof(styleAttribute) === 'string' && styleAttribute.trim() === '') {
      n.removeAttribute('style');
    }

    // Remove jstemplate attributes from nodes that stick around.
    jsattributes.forEach(function(attributeName) {
      n.removeAttribute(attributeName);
    });
  }

  displayNone.forEach(function(element) {
    element.parentElement.removeChild(element);
  });
}

// Strip empty lines, primarily so that elements which are jsdisplay=false and
// removed leave no whitespace-trace.
function stripEmptyLines(string) {
  function notEmpty(s) {
    return s.trim().length > 0;
  }
  return string.split('\n').filter(notEmpty).join('\n');
}

function serializePage() {
 cleanupJstemplateMess(document);
 var s = new XMLSerializer();
 return stripEmptyLines(s.serializeToString(document));
}

function evalXPathFromNode(expression, node) {
  var results = document.evaluate(expression, node, null,
      XPathResult.ORDERED_NODE_ITERATOR_TYPE, null);
  var retval = [];
  while(n = results.iterateNext()) {
    retval.push(n);
  }

  return retval;
}

function evalXPathFromId(expression, id) {
  return evalXPathFromNode(expression, document.getElementById(id));
}

// Select the current page on the left nav. Note: if already rendered, this
// will not effect any nodes.
function selectCurrentPageOnLeftNav() {
  function finalPathPart(str) {
    var pathParts = str.split(/\//);
    var lastPart = pathParts[pathParts.length - 1];
    return lastPart.split(/\?/)[0];
  }

  var pageBase = finalPathPart(document.location.href);

  evalXPathFromId('.//li/a', 'gc-toc').select(function(node) {
    if (pageBase == finalPathPart(node.href)) {
      var parent = node.parentNode;
      if (node.firstChild.nodeName == 'DIV') {
        node.firstChild.className = 'leftNavSelected';
      } else {
        parent.className = 'leftNavSelected';
      }
      parent.removeChild(node);
      parent.insertBefore(node.firstChild, parent.firstChild);
      return true;
    }
  });
}

/*
 * Template Callout Functions
 * The jstProcess() will call out to these functions from within the page
 * template
 */

function filterDocumented(things) {
  return !things ? [] : things.filter(function(thing) {
    return !disableDocs(thing);
  });
}

function listChromeAPIs(includeExperimental) {
  var packageType = document.body.getAttribute('doc-family') == 'extensions' ?
      'extension' : 'platform_app';

  // Super ghetto to use a synchronous XHR here, but this only runs during
  // generation of docs, so I guess it's ok.
  var req = new XMLHttpRequest();
  req.open('GET', '../../api/_permission_features.json', false);
  req.send(null);

  var permissionFeatures = JSON.parse(JSON.minify(req.responseText));

  var result = schema.filter(function(module) {
    if (disableDocs(module))
      return false;

    if ((module.namespace.indexOf('experimental') > -1) !=
        includeExperimental) {
      return false;
    }

    var feature = permissionFeatures[module.namespace];
    if (feature && feature.extension_types.indexOf(packageType) == -1)
      return false;

    return true;
  }).map(function(module) {
    return module.namespace;
  }).sort();

  if (packageType == 'platform_app') {
    result = result.filter(function(item) {
      return [
        "browserAction", "extension", "fileBrowserHandler", "fontSettings",
        "input.ime", "omnibox", "pageAction", "scriptBadge", "windows",
        "experimental.devtools.audits", "experimental.devtools.console",
        "experimental.discovery", "experimental.infobars",
        "experimental.keybinding", "experimental.offscreenTabs",
        "experimental.processes", "experimental.speechInput"
      ].indexOf(item) == -1;
    });
  }

  return result;
}

function getDataFromPageHTML(id) {
  var node = document.getElementById(id);
  if (!node)
    return;
  return node.innerHTML;
}

function isArray(type) {
  return type.type == 'array';
}

function isFunction(type) {
  return type.type == 'function';
}

function getTypeRef(type) {
  return type['$ref'];
}

function getTypeByName(typeName) {
  var module = typeModule[typeName] || {};
  for (var type in module.types) {
    if (module.types.hasOwnProperty(type) && type.id === typeName)
      return type;
  }
  return undefined;
}

function getDescription(typeName) {
  var type = getTypeByName(typeName);
  if (!type)
    return undefined;
  return type.description;
}

function getEnumValues(enumList, type) {
  if (type === 'string') {
    enumList = enumList.map(function(e) { return '"' + e + '"'});
  }
  var retval = enumList.join(', ');
  return '[' + retval + ']';
}

function showPageTOC() {
  return module || getDataFromPageHTML('pageData-showTOC');
}

function showSideNav() {
  return getDataFromPageHTML('pageData-showSideNav') != 'false';
}

function getStaticTOC() {
  var staticHNodes = evalXPathFromId('.//h2|h3', 'static');
  var retval = [];
  var lastH2;

  staticHNodes.forEach(function(n, i) {
    var anchorName = n.id || n.nodeName + '-' + i;
    if (!n.id) {
      var a = document.createElement('a');
      a.name = anchorName;
      n.parentNode.insertBefore(a, n);
    }
    var dataNode = { name: n.innerHTML, href: anchorName };

    if (n.nodeName == 'H2') {
      retval.push(dataNode);
      lastH2 = dataNode;
      lastH2.children = [];
    } else {
      lastH2.children.push(dataNode);
    }
  });

  return retval;
}

// This function looks in the description for strings of the form
// "$ref:TYPE_ID" (where TYPE_ID is something like "Tab" or "HistoryItem") and
// substitutes a link to the documentation for that type.
function substituteTypeRefs(description) {
  var regexp = /\$ref\:\w+/g;
  var matches = description.match(regexp);
  if (!matches) {
    return description;
  }
  var result = description;
  for (var i = 0; i < matches.length; i++) {
    var type = matches[i].split(':')[1];
    var page = null;
    try {
      page = getTypeRefPage({'$ref': type});
    } catch (error) {
      console.log('substituteTypeRefs couldn\'t find page for type ' + type);
      continue;
    }
    var replacement = '<a href="' + page + '#type-' + type + '">' + type +
                      '</a>';
    result = result.replace(matches[i], replacement);
  }

  return result;
}

function getTypeRefPage(type) {
  return typeModule[type.$ref].namespace + '.html';
}

function getPageName() {
  var pageDataName = getDataFromPageHTML('pageData-name');
  // Allow empty string to be explitly set via pageData.
  if (pageDataName == '') {
    return pageDataName;
  }

  return pageDataName || apiModuleName || pageName;
}

function getPageTitle() {
  var pageName = getPageName();
  var pageTitleSuffix = 'Google Chrome Extensions - Google Code';
  if (pageName == '') {
    return pageTitleSuffix;
  }

  return pageName + ' - ' + pageTitleSuffix;
}

function getModuleName() {
  return (module && typeof module.namespace) ?
      API_MODULE_PREFIX + module.namespace : '';
}

function getFullyQualifiedFunctionName(scope, func) {
  return (getObjectName(scope) || getModuleName()) + '.' + func.name;
}

function getObjectName(typeName) {
  return typeName.charAt(0).toLowerCase() + typeName.substring(1);
}

function isExperimentalAPIPage() {
  return (getPageName().indexOf('.experimental.') >= 0 &&
          getPageName().indexOf('.experimental.*') < 0);
}

function getPermittedChannels() {
  var allChannels = ['trunk', 'canary', 'dev', 'beta', 'stable'];
  var maximumSupportedChannel = 4;  // index in allChannels

  // Check for all required permissions of an API, whether their governing
  // feature is limited to a specific release channel.
  var requiredPermissions = module.documentation_permissions_required || [];
  requiredPermissions.push(module.namespace);
  for (var i = 0; i < requiredPermissions.length; ++i) {
    var permissionName = requiredPermissions[i];
    var supportedChannel = (permissionFeatures[permissionName] || {}).channel ||
        'stable';
    for (var j = 0; j < allChannels.length; ++j) {
      if (supportedChannel === allChannels[j]) {
        maximumSupportedChannel = Math.min(maximumSupportedChannel, j);
        break;
      }
    }
  }

  // Return a list of all supported channels.
  return allChannels.slice(0, maximumSupportedChannel + 1);
}

function isPermittedOnChannel(channel) {
  var permittedChannels = getPermittedChannels();
  for (var i = 0; i < permittedChannels.length; ++i) {
    if (permittedChannels[i] === channel) {
      return true;
    }
  }
  return false;
}

// Returns true if the last parameter in |parameters| is a fully defined
// callback function. This requires that the callback function has a defined
// parameters list. Callback functions without parameter lists cannot be
// described in the API documentation.
function hasFullyDefinedCallback(parameters) {
  return (parameters.length > 0 &&
          parameters[parameters.length - 1].type == 'function' &&
          parameters[parameters.length - 1].hasOwnProperty('parameters'));
}

function getCallbackParameters(parameters) {
  return parameters[parameters.length - 1];
}

function getAnchorName(type, name, scope) {
  return type + '-' + (scope ? scope + '-' : '') + name;
}

function shouldExpandObject(object) {
  return object.properties ||
         (object.items && object.items.properties);
}

function getPropertyListFromObject(object) {
  var propertyList = [];
  var properties = object.properties;
  if (!properties && object.type === 'array' && object.items) {
    properties = object.items.properties;
  }
  for (var p in properties) {
    var prop = properties[p];
    // Do not render properties with documentation disabled.
    if (disableDocs(prop)) {
      continue;
    }
    prop.name = p;
    propertyList.push(prop);
  }
  return propertyList;
}

function getTypeName(schema) {
  if (schema.$ref)
    return schema.$ref;

  if (schema.choices) {
    var typeNames = [];
    schema.choices.forEach(function(c) {
      typeNames.push(getTypeName(c));
    });

    return typeNames.join(' or ');
  }

  if (schema.type == 'array')
    return 'array of ' + getTypeName(schema.items);

  if (schema.isInstanceOf)
    return schema.isInstanceOf;

  return schema.type;
}

function getPrettyTypeId(typeName) {
  return typeIdType[typeName].prettyId;
}

function hasPrimitiveValue(schema) {
  var type = typeof(schema.value);
  return type === 'string' || type === 'number';
}

function getPrimitiveValue(schema) {
  switch (typeof(schema.value)) {
    case 'string':
      return '"' + schema.value + '"';

    case 'number':
      // Comma-separate large numbers (e.g. 5,000,000), easier to read.
      var value = String(schema.value);
      var groupsOfThree = [];
      while (value.length > 3) {
        groupsOfThree.unshift(value.slice(value.length - 3));
        value = value.slice(0, value.length - 3);
      }
      groupsOfThree.unshift(value);
      return groupsOfThree.join(',');
  }

  return undefined;
}

function getSignatureString(parameters) {
  if (!parameters)
    return '';
  var retval = [];
  filterDocumented(parameters).forEach(function(param, i) {
    retval.push(getTypeName(param) + ' ' + param.name);
  });

  return retval.join(', ');
}

function getOptionalSignatureSubstring(parameters) {
  if (!parameters)
    return '';
  return ', ' + getSignatureString(parameters);
}

function sortByName(a, b) {
  if (a.name < b.name) {
    return -1;
  }
  if (a.name > b.name) {
    return 1;
  }
  return 0;
}

function disableDocs(obj) {
  return !!obj.nodoc;
}
