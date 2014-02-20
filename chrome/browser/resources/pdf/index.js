// Copyright (c) 2012 The Polymer Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// @version: 0.1.3
Polymer={},"function"==typeof window.Polymer&&(Polymer={}),function(a){function b(a,b){return a&&b&&Object.getOwnPropertyNames(b).forEach(function(c){var d=Object.getOwnPropertyDescriptor(b,c);d&&(Object.defineProperty(a,c,d),"function"==typeof d.value&&(d.value.nom=c))}),a}a.extend=b}(Polymer),function(a){function b(a,b,d){return a?a.stop():a=new c(this),a.go(b,d),a}var c=function(a){this.context=a};c.prototype={go:function(a,b){this.callback=a,this.handle=setTimeout(this.complete.bind(this),b)},stop:function(){this.handle&&(clearTimeout(this.handle),this.handle=null)},complete:function(){this.handle&&(this.stop(),this.callback.call(this.context))}},a.job=b}(Polymer),function(){var a={};HTMLElement.register=function(b,c){a[b]=c},HTMLElement.getPrototypeForTag=function(b){var c=b?a[b]:HTMLElement.prototype;return c||Object.getPrototypeOf(document.createElement(b))};var b=Event.prototype.stopPropagation;Event.prototype.stopPropagation=function(){this.cancelBubble=!0,b.apply(this,arguments)},HTMLImports.importer.preloadSelectors+=", polymer-element link[rel=stylesheet]"}(Polymer),function(a){function b(a){var c=b.caller,g=c.nom,h=c._super;if(h||(g||(g=c.nom=e.call(this,c)),g||console.warn("called super() on a method not installed declaratively (has no .nom property)"),h=d(c,g,f(this))),h){var i=h[g];return i._super||d(i,g,h),i.apply(this,a||[])}}function c(a,b,c){for(;a;){if(a[b]!==c&&a[b])return a;a=f(a)}}function d(a,b,d){return a._super=c(d,b,a),a._super&&(a._super[b].nom=b),a._super}function e(a){for(var b=this.__proto__;b&&b!==HTMLElement.prototype;){for(var c,d=Object.getOwnPropertyNames(b),e=0,f=d.length;f>e&&(c=d[e]);e++){var g=Object.getOwnPropertyDescriptor(b,c);if("function"==typeof g.value&&g.value===a)return c}b=b.__proto__}}function f(a){return a.__proto__}a.super=b}(Polymer),function(a){function b(a,b){var d=typeof b;return b instanceof Date&&(d="date"),c[d](a,b)}var c={string:function(a){return a},date:function(a){return new Date(Date.parse(a)||Date.now())},"boolean":function(a){return""===a?!0:"false"===a?!1:!!a},number:function(a){var b=parseFloat(a);return 0===b&&(b=parseInt(a)),isNaN(b)?a:b},object:function(a,b){if(null===b)return a;try{return JSON.parse(a.replace(/'/g,'"'))}catch(c){return a}},"function":function(a,b){return b}};a.deserializeValue=b}(Polymer),function(a){var b={};b.declaration={},b.instance={},a.api=b}(Polymer),function(a){var b={async:function(a,b,c){Platform.flush(),b=b&&b.length?b:[b];var d=function(){(this[a]||a).apply(this,b)}.bind(this);return c?setTimeout(d,c):requestAnimationFrame(d)},fire:function(a,b,c,d){var e=c||this;return e.dispatchEvent(new CustomEvent(a,{bubbles:void 0!==d?d:!0,detail:b})),b},asyncFire:function(){this.async("fire",arguments)},classFollows:function(a,b,c){b&&b.classList.remove(c),a&&a.classList.add(c)}},c=function(){},d={};b.asyncMethod=b.async,a.api.instance.utils=b,a.nop=c,a.nob=d}(Polymer),function(a){function b(a){for(;a.parentNode;)a=a.parentNode;return a.host}var c=window.logFlags||{},d="on-",e={EVENT_PREFIX:d,hasEventPrefix:function(a){return a&&"o"===a[0]&&"n"===a[1]&&"-"===a[2]},removeEventPrefix:function(a){return a.slice(f)},addHostListeners:function(){var a=this.eventDelegates;c.events&&Object.keys(a).length>0&&console.log("[%s] addHostListeners:",this.localName,a),this.addNodeListeners(this,a,this.hostEventListener)},addNodeListeners:function(a,b,c){var d;for(var e in b)d||(d=c.bind(this)),this.addNodeListener(a,e,d)},addNodeListener:function(a,b,c){a.addEventListener(b,c)},hostEventListener:function(a){if(!a.cancelBubble){c.events&&console.group("[%s]: hostEventListener(%s)",this.localName,a.type);var b=this.findEventDelegate(a);b&&(c.events&&console.log("[%s] found host handler name [%s]",this.localName,b),this.dispatchMethod(this,b,[a,a.detail,this])),c.events&&console.groupEnd()}},findEventDelegate:function(a){return this.eventDelegates[a.type]},dispatchMethod:function(a,b,d){if(a){c.events&&console.group("[%s] dispatch [%s]",a.localName,b);var e="function"==typeof b?b:a[b];e&&e[d?"apply":"call"](a,d),c.events&&console.groupEnd(),Platform.flush()}},prepareBinding:function(a,d){return e.hasEventPrefix(d)?function(f,g){c.events&&console.log('event: [%s].%s => [%s].%s()"',g.localName,f.localName,a);var h=function(c){var d=b(g);if(d&&d.dispatchMethod){var e=d,h=a;"@"==a[0]&&(e=f,h=Path.get(a.slice(1)).getValueFrom(f)),d.dispatchMethod(e,h,[c,c.detail,g])}},i=e.removeEventPrefix(d);return{open:function(){g.addEventListener(i,h,!1)},close:function(){c.events&&console.log('event.remove: [%s].%s => [%s].%s()"',g.localName,d,f.localName,a),g.removeEventListener(i,h,!1)},discardChanges:function(){}}}:void 0}},f=d.length;a.api.instance.events=e}(Polymer),function(a){var b={copyInstanceAttributes:function(){var a=this._instanceAttributes;for(var b in a)this.hasAttribute(b)||this.setAttribute(b,a[b])},takeAttributes:function(){if(this._publishLC)for(var a,b=0,c=this.attributes,d=c.length;(a=c[b])&&d>b;b++)this.attributeToProperty(a.name,a.value)},attributeToProperty:function(b,c){var b=this.propertyForAttribute(b);if(b){if(c&&c.search(a.bindPattern)>=0)return;var d=this[b],c=this.deserializeValue(c,d);c!==d&&(this[b]=c)}},propertyForAttribute:function(a){var b=this._publishLC&&this._publishLC[a];return b},deserializeValue:function(b,c){return a.deserializeValue(b,c)},serializeValue:function(a,b){return"boolean"===b?a?"":void 0:"object"!==b&&"function"!==b&&void 0!==a?a:void 0},reflectPropertyToAttribute:function(a){var b=typeof this[a],c=this.serializeValue(this[a],b);void 0!==c?this.setAttribute(a,c):"boolean"===b&&this.removeAttribute(a)}};a.api.instance.attributes=b}(Polymer),function(a){function b(a,b,d){c.bind&&console.log(e,inB.localName||"object",inPath,a.localName,b);var f=d.discardChanges();return(null===f||void 0===f)&&d.setValue(a[b]),Observer.defineComputedProperty(a,b,d)}var c=window.logFlags||{},d={observeProperties:function(){var a=this._observeNames,b=this._publishNames;if(a&&a.length||b&&b.length){for(var c,d=this._propertyObserver=new CompoundObserver,e=0,f=a.length;f>e&&(c=a[e]);e++){d.addPath(this,c);var g=Object.getOwnPropertyDescriptor(this.__proto__,c);g&&g.value&&this.observeArrayValue(c,g.value,null)}for(var c,e=0,f=b.length;f>e&&(c=b[e]);e++)this.observe&&void 0!==this.observe[c]||d.addPath(this,c);d.open(this.notifyPropertyChanges,this)}},notifyPropertyChanges:function(a,b,c){var d,e,f={};for(var g in b)d=c[2*g+1],void 0!==this.publish[d]&&this.reflectPropertyToAttribute(d),e=this.observe[d],e&&(this.observeArrayValue(d,a[g],b[g]),f[e]||(f[e]=!0,this.invokeMethod(e,[b[g],a[g],arguments])))},observeArrayValue:function(a,b,d){var e=this.observe[a];if(e&&(Array.isArray(d)&&(c.observe&&console.log("[%s] observeArrayValue: unregister observer [%s]",this.localName,a),this.unregisterObserver(a+"__array")),Array.isArray(b))){c.observe&&console.log("[%s] observeArrayValue: register observer [%s]",this.localName,a,b);var f=new ArrayObserver(b);f.open(function(a,b){this.invokeMethod(e,[b])},this),this.registerObserver(a+"__array",f)}},bindProperty:function(a,c){return b(this,a,c)},unbindAllProperties:function(){this._propertyObserver&&this._propertyObserver.close(),this.unregisterObservers()},unbindProperty:function(a){return this.unregisterObserver(a)},invokeMethod:function(a,b){var c=this[a]||a;"function"==typeof c&&c.apply(this,b)},registerObserver:function(a,b){var c=this._observers||(this._observers={});c[a]=b},unregisterObserver:function(a){var b=this._observers;return b&&b[a]?(b[a].close(),b[a]=null,!0):void 0},unregisterObservers:function(){if(this._observers){for(var a,b,c=Object.keys(this._observers),d=0,e=c.length;e>d&&(a=c[d]);d++)b=this._observers[a],b.close();this._observers={}}}},e="[%s]: bindProperties: [%s] to [%s].[%s]";a.api.instance.properties=d}(Polymer),function(a){function b(a){d(a,c)}function c(a){a.unbindAll()}function d(a,b){if(a){b(a);for(var c=a.firstChild;c;c=c.nextSibling)d(c,b)}}var e=window.logFlags||0,f=a.api.instance.events,g=PolymerExpressions.prototype.prepareBinding;PolymerExpressions.prototype.prepareBinding=function(a,b,c){return f.prepareBinding(a,b,c)||g.call(this,a,b,c)};var h=new PolymerExpressions,i={syntax:h,instanceTemplate:function(a){return a.createInstance(this,this.syntax)},bind:function(a,b){this._elementPrepared||this.prepareElement();var c=this.propertyForAttribute(a);if(c){this.unbind(a);var d=this.bindProperty(c,b);return d.path=b.path_,this.reflectPropertyToAttribute(c),this.bindings[a]=d}return this.super(arguments)},asyncUnbindAll:function(){this._unbound||(e.unbind&&console.log("[%s] asyncUnbindAll",this.localName),this._unbindAllJob=this.job(this._unbindAllJob,this.unbindAll,0))},unbindAll:function(){if(!this._unbound){this.unbindAllProperties(),this.super();for(var a=this.shadowRoot;a;)b(a),a=a.olderShadowRoot;this._unbound=!0}},cancelUnbindAll:function(a){return this._unbound?(e.unbind&&console.warn("[%s] already unbound, cannot cancel unbindAll",this.localName),void 0):(e.unbind&&console.log("[%s] cancelUnbindAll",this.localName),this._unbindAllJob&&(this._unbindAllJob=this._unbindAllJob.stop()),a||d(this.shadowRoot,function(a){a.cancelUnbindAll&&a.cancelUnbindAll()}),void 0)}},j=/\{\{([^{}]*)}}/;a.bindPattern=j,a.api.instance.mdv=i}(Polymer),function(a){function b(a){return a.hasOwnProperty("PolymerBase")}function c(){}var d=0,e={PolymerBase:!0,job:Polymer.job,"super":Polymer.super,created:function(){},ready:function(){},createdCallback:function(){this.created(),(this.ownerDocument.defaultView||this.alwaysPrepare||d>0)&&this.prepareElement()},prepareElement:function(){this._elementPrepared=!0,this.observeProperties(),this.copyInstanceAttributes(),this.takeAttributes(),this.addHostListeners(),d++,this.parseDeclarations(this.__proto__),d--,this.ready()},attachedCallback:function(){this._elementPrepared||this.prepareElement(),this.cancelUnbindAll(!0),this.attached&&this.attached(),this.enteredView&&this.enteredView()},detachedCallback:function(){this.preventDispose||this.asyncUnbindAll(),this.detached&&this.detached(),this.leftView&&this.leftView()},enteredViewCallback:function(){this.attachedCallback()},leftViewCallback:function(){this.detachedCallback()},enteredDocumentCallback:function(){this.attachedCallback()},leftDocumentCallback:function(){this.detachedCallback()},parseDeclarations:function(a){a&&a.element&&(this.parseDeclarations(a.__proto__),a.parseDeclaration.call(this,a.element))},parseDeclaration:function(a){var b=this.fetchTemplate(a);b&&(this.element.hasAttribute("lightdom")?this.lightFromTemplate(b):this.shadowFromTemplate(b))},fetchTemplate:function(a){return a.querySelector("template")},shadowFromTemplate:function(a){if(a){var b=(this.shadowRoot,this.createShadowRoot());b.applyAuthorStyles=this.applyAuthorStyles,b.resetStyleInheritance=this.resetStyleInheritance;var c=this.instanceTemplate(a);return b.appendChild(c),this.shadowRootReady(b,a),b}},lightFromTemplate:function(a){if(a){var b=this.instanceTemplate(a);return this.appendChild(b),this.shadowRootReady(this,a),b}},shadowRootReady:function(a){this.marshalNodeReferences(a),PointerGestures.register(a)},marshalNodeReferences:function(a){var b=this.$=this.$||{};if(a)for(var c,d=a.querySelectorAll("[id]"),e=0,f=d.length;f>e&&(c=d[e]);e++)b[c.id]=c},attributeChangedCallback:function(a){"class"!==a&&"style"!==a&&this.attributeToProperty(a,this.getAttribute(a)),this.attributeChanged&&this.attributeChanged.apply(this,arguments)},onMutation:function(a,b){var c=new MutationObserver(function(a){b.call(this,c,a),c.disconnect()}.bind(this));c.observe(a,{childList:!0,subtree:!0})}};c.prototype=e,e.constructor=c,a.Base=c,a.isBase=b,a.api.instance.base=e}(Polymer),function(a){function b(a){return a.__proto__}var c=(window.logFlags||{},"element"),d="controller",e={STYLE_SCOPE_ATTRIBUTE:c,installControllerStyles:function(){var a=this.findStyleController();if(a&&!this.scopeHasElementStyle(a,d)){for(var c=b(this),e="";c&&c.element;)e+=c.element.cssTextForScope(d),c=b(c);if(e){var f=this.element.cssTextToScopeStyle(e,d);Polymer.applyStyleToScope(f,a)}}},findStyleController:function(){if(window.ShadowDOMPolyfill)return wrap(document.head);for(var a=this;a.parentNode;)a=a.parentNode;return a===document?document.head:a},scopeHasElementStyle:function(a,b){var d=c+"="+this.localName+"-"+b;return a.querySelector("style["+d+"]")}};a.api.instance.styles=e}(Polymer),function(a){var b={addResolvePathApi:function(){var a=this.elementPath(),b=this.getAttribute("assetpath")||"",c=this.relPath;this.prototype.resolvePath=function(d){var e=d;if(b){var f=b.slice(0,-1);e=c(f,e)}return a+b+e}},elementPath:function(){return this.urlToPath(HTMLImports.getDocumentUrl(this.ownerDocument))},relPath:function(a,b){for(var c=a.split("/"),d=b.split("/"),e=!1;c.length&&d.length&&c[0]===d[0];)c.shift(),d.shift(),e=!0;if(e)for(var f=0;f<c.length;f++)d.unshift("..");return d.join("/")},urlToPath:function(a){if(a){var b=a.split("/");return b.pop(),b.push(""),b.join("/")}return""}};a.api.declaration.path=b}(Polymer),function(a){function b(a,b){if(a){var d=c(a.textContent),e=a.getAttribute(g);e&&d.setAttribute(g,e),b.appendChild(d)}}function c(a){var b=document.createElement("style");return b.textContent=a,b}function d(a){return a&&a.__resource||""}function e(a,b){return n?n.call(a,b):void 0}var f=(window.logFlags||{},a.api.instance.styles),g=f.STYLE_SCOPE_ATTRIBUTE,h="style",i="[rel=stylesheet]",j="global",k="polymer-scope",l={installSheets:function(){this.cacheSheets(),this.cacheStyles(),this.installLocalSheets(),this.installGlobalStyles()},cacheSheets:function(){this.sheets=this.findNodes(i),this.sheets.forEach(function(a){a.parentNode&&a.parentNode.removeChild(a)})},cacheStyles:function(){this.styles=this.findNodes(h+"["+k+"]"),this.styles.forEach(function(a){a.parentNode&&a.parentNode.removeChild(a)})},installLocalSheets:function(){var a=this.sheets.filter(function(a){return!a.hasAttribute(k)}),b=this.templateContent();if(b){var e="";a.forEach(function(a){e+=d(a)+"\n"}),e&&b.insertBefore(c(e),b.firstChild)}},findNodes:function(a,b){var c=this.querySelectorAll(a).array(),d=this.templateContent();if(d){var e=d.querySelectorAll(a).array();c=c.concat(e)}return b?c.filter(b):c},templateContent:function(){var a=this.querySelector("template");return a&&templateContent(a)},installGlobalStyles:function(){var a=this.styleForScope(j);b(a,document.head)},cssTextForScope:function(a){var b="",c="["+k+"="+a+"]",f=function(a){return e(a,c)},g=this.sheets.filter(f);g.forEach(function(a){b+=d(a)+"\n\n"});var h=this.styles.filter(f);return h.forEach(function(a){b+=a.textContent+"\n\n"}),b},styleForScope:function(a){var b=this.cssTextForScope(a);return this.cssTextToScopeStyle(b,a)},cssTextToScopeStyle:function(a,b){if(a){var d=c(a);return d.setAttribute(g,this.getAttribute("name")+"-"+b),d}}},m=HTMLElement.prototype,n=m.matches||m.matchesSelector||m.webkitMatchesSelector||m.mozMatchesSelector;a.api.declaration.styles=l,a.applyStyleToScope=b}(Polymer),function(a){var b=a.api.instance.events,c=(window.logFlags||{},{parseHostEvents:function(){var a=this.prototype.eventDelegates;this.addAttributeDelegates(a)},addAttributeDelegates:function(a){for(var c,d=0;c=this.attributes[d];d++)b.hasEventPrefix(c.name)&&(a[b.removeEventPrefix(c.name)]=c.value.replace("{{","").replace("}}","").trim())},event_translations:{webkitanimationstart:"webkitAnimationStart",webkitanimationend:"webkitAnimationEnd",webkittransitionend:"webkitTransitionEnd",domfocusout:"DOMFocusOut",domfocusin:"DOMFocusIn",dommousescroll:"DOMMouseScroll"}});a.api.declaration.events=c}(Polymer),function(a){var b={inferObservers:function(a){var b,c=a.observe;for(var d in a)"Changed"===d.slice(-7)&&(c||(c=a.observe={}),b=d.slice(0,-7),c[b]=c[b]||d)},explodeObservers:function(a){var b=a.observe;if(b){var c={};for(var d in b)for(var e,f=d.split(" "),g=0;e=f[g];g++)c[e]=b[d];a.observe=c}},optimizePropertyMaps:function(a){if(a.observe){var b=a._observeNames=[];for(var c in a.observe)for(var d,e=c.split(" "),f=0;d=e[f];f++)b.push(d)}if(a.publish){var b=a._publishNames=[];for(var c in a.publish)b.push(c)}},publishProperties:function(a,b){var c=a.publish;c&&(this.requireProperties(c,a,b),a._publishLC=this.lowerCaseMap(c))},requireProperties:function(a,b,c){for(var d in a)void 0===b[d]&&void 0===c[d]&&(b[d]=a[d])},lowerCaseMap:function(a){var b={};for(var c in a)b[c.toLowerCase()]=c;return b}};a.api.declaration.properties=b}(Polymer),function(a){var b="attributes",c=/\s|,/,d={inheritAttributesObjects:function(a){this.inheritObject(a,"publishLC"),this.inheritObject(a,"_instanceAttributes")},publishAttributes:function(a,d){var e=this.getAttribute(b);if(e)for(var f,g=a.publish||(a.publish={}),h=e.split(c),i=0,j=h.length;j>i;i++)f=h[i].trim(),f&&void 0===g[f]&&void 0===d[f]&&(g[f]=null)},accumulateInstanceAttributes:function(){for(var a,b=this.prototype._instanceAttributes,c=this.attributes,d=0,e=c.length;e>d&&(a=c[d]);d++)this.isInstanceAttribute(a.name)&&(b[a.name]=a.value)},isInstanceAttribute:function(a){return!this.blackList[a]&&"on-"!==a.slice(0,3)},blackList:{name:1,"extends":1,constructor:1,noscript:1}};d.blackList[b]=1,a.api.declaration.attributes=d}(Polymer),function(a){function b(a){if(!Object.__proto__){var b=Object.getPrototypeOf(a);a.__proto__=b,d(b)&&(b.__proto__=Object.getPrototypeOf(b))}}var c=a.api,d=a.isBase,e=a.extend,f={register:function(a,b){this.prototype=this.buildPrototype(a,b),this.prototype.element=this,this.desugar(a,b),this.registerPrototype(a,b),this.publishConstructor()},buildPrototype:function(c,d){var e=a.getRegisteredPrototype(c),f=this.generateBasePrototype(d);return this.publishAttributes(e,f),this.publishProperties(e,f),this.inferObservers(e),this.explodeObservers(e),this.inheritMetaData(e,f),e=this.chainObject(e,f),this.optimizePropertyMaps(e),b(e),e},inheritMetaData:function(a,b){this.inheritObject("observe",a,b),this.inheritObject("publish",a,b),this.inheritObject("_publishLC",a,b),this.inheritObject("_instanceAttributes",a,b),this.inheritObject("eventDelegates",a,b)},desugar:function(a,b){this.accumulateInstanceAttributes(),this.parseHostEvents(),this.installSheets(),this.adjustShadowElement(),this.addResolvePathApi(),window.ShadowDOMPolyfill&&Platform.ShadowCSS.shimStyling(this.templateContent(),a,b),this.prototype.registerCallback&&this.prototype.registerCallback(this)},adjustShadowElement:function(){if(!window.ShadowDOMPolyfill){var a=this.templateContent();if(a)for(var b,c=a.querySelectorAll("shadow"),d=0,e=c.length;e>d&&(b=c[d]);d++)b.children.length||b.appendChild(document.createElement("content"))}},publishConstructor:function(){var a=this.getAttribute("constructor");a&&(window[a]=this.ctor)},generateBasePrototype:function(a){var b=this.findBasePrototype(a);if(!b){var b=HTMLElement.getPrototypeForTag(a);b=this.ensureBaseApi(b),memoizedBases[a]=b}return b},findBasePrototype:function(a){return memoizedBases[a]},ensureBaseApi:function(a){if(a.PolymerBase)return a;var b=Object.create(a);for(var d in c.instance)e(b,c.instance[d]);return this.mixinMethod(b,a,c.instance.mdv,"bind"),b},mixinMethod:function(a,b,c,d){var e=function(a){return b[d].apply(this,a)};a[d]=function(){this.super=e;var b=c[d].apply(this,arguments);return this.super=a.super,b}},inheritObject:function(a,b,c){var d=b[a]||{};b[a]=this.chainObject(d,c[a])},registerPrototype:function(a,b){var c={prototype:this.prototype},d=this.findTypeExtension(b);d&&(c.extends=d),this.ctor=document.registerElement(a,c),this.prototype.constructor=this.ctor,HTMLElement.register(a,this.prototype)},findTypeExtension:function(a){if(a&&a.indexOf("-")<0)return a;var b=this.findBasePrototype(a);return b.element?this.findTypeExtension(b.element.extends):void 0}};f.chainObject=Object.__proto__?function(a,b){return a&&b&&a!==b&&(a.__proto__=b),a}:function(a,b){if(a&&b&&a!==b){var c=Object.create(b);a=e(c,a)}return a},memoizedBases={},c.declaration.prototype=f}(Polymer),function(a){function b(a,b){k[a]=b||{},d(a)}function c(a){return k[a]}function d(a){l[a]&&(l[a].registerWhenReady(),delete l[a])}function e(a){n[a]=!0;var b=m[a];b&&(b.forEach(function(a){a.registerWhenReady()}),delete m[a])}function f(a){return n[a]}function g(a){window.HTMLImports&&!HTMLImports.readyTime?addEventListener("HTMLImportsLoaded",a):a()}var h=a.extend,i=a.api.declaration,j=h(Object.create(HTMLElement.prototype),{createdCallback:function(){this.name=this.getAttribute("name"),this.extends=this.getAttribute("extends"),this.registerWhenReady()},registerWhenReady:function(){if(!this.waitingForPrototype(this.name)){var a=this.extends;this.waitingForExtendee(a)||(document.contains(this)?g(function(){this._register(a)}.bind(this)):this._register(a))}},_register:function(a){this.register(this.name,a),e(this.name)},waitingForPrototype:function(a){if(!c(a)){if(l[a]=this,this.hasAttribute("noscript"))if(window.CustomElements&&!CustomElements.useNative)b(a);else{var d=document.createElement("script");d.textContent="Polymer('"+a+"');",this.appendChild(d)}return!0}},waitingForExtendee:function(a){return a&&a.indexOf("-")>=0&&!f(a)?((m[a]=m[a]||[]).push(this),!0):void 0}});Object.keys(i).forEach(function(a){h(j,i[a])});var k={},l={},m={},n={};a.getRegisteredPrototype=c,h(b,a),window.Polymer=b;var o=Platform.deliverDeclarations();if(o)for(var p,q=0,r=o.length;r>q&&(p=o[q]);q++)b.apply(null,p);document.registerElement("polymer-element",{prototype:j})}(Polymer);
//# sourceMappingURL=polymer.js.map;

    Polymer('polymer-selection', {
      /**
       * If true, multiple selections are allowed.
       *
       * @attribute multi
       * @type boolean
       * @default false
       */
      multi: false,
      ready: function() {
        this.clear();
      },
      clear: function() {
        this.selection = [];
      },
      /**
       * Retrieves the selected item(s).
       * @method getSelection
       * @returns Returns the selected item(s). If the multi property is true,
       * getSelection will return an array, otherwise it will return 
       * the selected item or undefined if there is no selection.
      */
      getSelection: function() {
        return this.multi ? this.selection : this.selection[0];
      },
      /**
       * Indicates if a given item is selected.
       * @method isSelected
       * @param {any} item The item whose selection state should be checked.
       * @returns Returns true if `item` is selected.
      */
      isSelected: function(item) {
        return this.selection.indexOf(item) >= 0;
      },
      setItemSelected: function(item, isSelected) {
        if (item !== undefined && item !== null) {
          if (isSelected) {
            this.selection.push(item);
          } else {
            var i = this.selection.indexOf(item);
            if (i >= 0) {
              this.selection.splice(i, 1);
            }
          }
          this.fire("polymer-select", {isSelected: isSelected, item: item});
        }
      },
      /**
       * Set the selection state for a given `item`. If the multi property
       * is true, then the selected state of `item` will be toggled; otherwise
       * the `item` will be selected.
       * @method select
       * @param {any} item: The item to select.
      */
      select: function(item) {
        if (this.multi) {
          this.toggle(item);
        } else if (this.getSelection() !== item) {
          this.setItemSelected(this.getSelection(), false);
          this.setItemSelected(item, true);
        }
      },
      /**
       * Toggles the selection state for `item`.
       * @method toggle
       * @param {any} item: The item to toggle.
      */
      toggle: function(item) {
        this.setItemSelected(item, !this.isSelected(item));
      }
    });
  ;

    Polymer('polymer-selector', {
      /**
       * Gets or sets the selected element.  Default to use the index
       * of the item element.
       *
       * If you want a specific attribute value of the element to be
       * used instead of index, set "valueattr" to that attribute name.
       *
       * Example:
       *
       *     <polymer-selector valueattr="label" selected="foo">
       *       <div label="foo"></div>
       *       <div label="bar"></div>
       *       <div label="zot"></div>
       *     </polymer-selector>
       *
       * In multi-selection this should be an array of values.
       *
       * Example:
       *
       *     <polymer-selector id="selector" valueattr="label" multi>
       *       <div label="foo"></div>
       *       <div label="bar"></div>
       *       <div label="zot"></div>
       *     </polymer-selector>
       *
       *     this.$.selector.selected = ['foo', 'zot'];
       *
       * @attribute selected
       * @type Object
       * @default null
       */
      selected: null,
      /**
       * If true, multiple selections are allowed.
       *
       * @attribute multi
       * @type boolean
       * @default false
       */
      multi: false,
      /**
       * Specifies the attribute to be used for "selected" attribute.
       *
       * @attribute valueattr
       * @type string
       * @default 'name'
       */
      valueattr: 'name',
      /**
       * Specifies the CSS class to be used to add to the selected element.
       * 
       * @attribute selectedClass
       * @type string
       * @default 'polymer-selected'
       */
      selectedClass: 'polymer-selected',
      /**
       * Specifies the property to be used to set on the selected element
       * to indicate its active state.
       *
       * @attribute selectedProperty
       * @type string
       * @default 'active'
       */
      selectedProperty: 'active',
      /**
       * Returns the currently selected element. In multi-selection this returns
       * an array of selected elements.
       * 
       * @attribute selectedItem
       * @type Object
       * @default null
       */
      selectedItem: null,
      /**
       * In single selection, this returns the model associated with the
       * selected element.
       * 
       * @attribute selectedModel
       * @type Object
       * @default null
       */
      selectedModel: null,
      /**
       * In single selection, this returns the selected index.
       *
       * @attribute selectedIndex
       * @type number
       * @default -1
       */
      selectedIndex: -1,
      /**
       * The target element that contains items.  If this is not set 
       * polymer-selector is the container.
       * 
       * @attribute target
       * @type Object
       * @default null
       */
      target: null,
      /**
       * This can be used to query nodes from the target node to be used for 
       * selection items.  Note this only works if the 'target' property is set.
       *
       * Example:
       *
       *     <polymer-selector target="{{$.myForm}}" itemsSelector="input[type=radio]"></polymer-selector>
       *     <form id="myForm">
       *       <label><input type="radio" name="color" value="red"> Red</label> <br>
       *       <label><input type="radio" name="color" value="green"> Green</label> <br>
       *       <label><input type="radio" name="color" value="blue"> Blue</label> <br>
       *       <p>color = {{color}}</p>
       *     </form>
       * 
       * @attribute itemSelector
       * @type string
       * @default ''
       */
      itemsSelector: '',
      /**
       * The event that would be fired from the item element to indicate
       * it is being selected.
       *
       * @attribute activateEvent
       * @type string
       * @default 'tap'
       */
      activateEvent: 'tap',
      notap: false,
      ready: function() {
        this.activateListener = this.activateHandler.bind(this);
        this.observer = new MutationObserver(this.updateSelected.bind(this));
        if (!this.target) {
          this.target = this;
        }
      },
      get items() {
        var nodes = this.target !== this ? (this.itemsSelector ? 
            this.target.querySelectorAll(this.itemsSelector) : 
                this.target.children) : this.$.items.getDistributedNodes();
        return Array.prototype.filter.call(nodes || [], function(n) {
          return n && n.localName !== 'template';
        });
      },
      targetChanged: function(old) {
        if (old) {
          this.removeListener(old);
          this.observer.disconnect();
        }
        if (this.target) {
          this.addListener(this.target);
          this.observer.observe(this.target, {childList: true});
        }
      },
      addListener: function(node) {
        node.addEventListener(this.activateEvent, this.activateListener);
      },
      removeListener: function(node) {
        node.removeEventListener(this.activateEvent, this.activateListener);
      },
      get selection() {
        return this.$.selection.getSelection();
      },
      selectedChanged: function() {
        this.updateSelected();
      },
      updateSelected: function() {
        this.validateSelected();
        if (this.multi) {
          this.clearSelection();
          this.selected && this.selected.forEach(function(s) {
            this.valueToSelection(s);
          }, this);
        } else {
          this.valueToSelection(this.selected);
        }
      },
      validateSelected: function() {
        // convert to an array for multi-selection
        if (this.multi && !Array.isArray(this.selected) && 
            this.selected !== null && this.selected !== undefined) {
          this.selected = [this.selected];
        }
      },
      clearSelection: function() {
        if (this.multi) {
          this.selection.slice().forEach(function(s) {
            this.$.selection.setItemSelected(s, false);
          }, this);
        } else {
          this.$.selection.setItemSelected(this.selection, false);
        }
        this.selectedItem = null;
        this.$.selection.clear();
      },
      valueToSelection: function(value) {
        var item = (value === null || value === undefined) ? 
            null : this.items[this.valueToIndex(value)];
        this.$.selection.select(item);
      },
      updateSelectedItem: function() {
        this.selectedItem = this.selection;
      },
      selectedItemChanged: function() {
        if (this.selectedItem) {
          var t = this.selectedItem.templateInstance;
          this.selectedModel = t ? t.model : undefined;
        } else {
          this.selectedModel = null;
        }
        this.selectedIndex = this.selectedItem ? 
            parseInt(this.valueToIndex(this.selected)) : -1;
      },
      valueToIndex: function(value) {
        // find an item with value == value and return it's index
        for (var i=0, items=this.items, c; (c=items[i]); i++) {
          if (this.valueForNode(c) == value) {
            return i;
          }
        }
        // if no item found, the value itself is probably the index
        return value;
      },
      valueForNode: function(node) {
        return node[this.valueattr] || node.getAttribute(this.valueattr);
      },
      // events fired from <polymer-selection> object
      selectionSelect: function(e, detail) {
        this.updateSelectedItem();
        if (detail.item) {
          this.applySelection(detail.item, detail.isSelected)
        }
      },
      applySelection: function(item, isSelected) {
        if (this.selectedClass) {
          item.classList.toggle(this.selectedClass, isSelected);
        }
        if (this.selectedProperty) {
          item[this.selectedProperty] = isSelected;
        }
      },
      // event fired from host
      activateHandler: function(e) {
        if (!this.notap) {
          var i = this.findDistributedTarget(e.target, this.items);
          if (i >= 0) {
            var item = this.items[i];
            var s = this.valueForNode(item) || i;
            if (this.multi) {
              if (this.selected) {
                this.addRemoveSelected(s);
              } else {
                this.selected = [s];
              }
            } else {
              this.selected = s;
            }
            this.asyncFire('polymer-activate', {item: item});
          }
        }
      },
      addRemoveSelected: function(value) {
        var i = this.selected.indexOf(value);
        if (i >= 0) {
          this.selected.splice(i, 1);
        } else {
          this.selected.push(value);
        }
        this.valueToSelection(value);
      },
      findDistributedTarget: function(target, nodes) {
        // find first ancestor of target (including itself) that
        // is in nodes, if any
        while (target && target != this) {
          var i = Array.prototype.indexOf.call(nodes, target);
          if (i >= 0) {
            return i;
          }
          target = target.parentNode;
        }
      }
    });
  ;

  Polymer('viewer-toolbar', {
    fadingIn: false,
    timerId: undefined,
    ready: function() {
      this.fadingInChanged();
    },
    fadeIn: function() {
      this.fadingIn = true;
    },
    fadeOut: function() {
      this.fadingIn = false;
    },
    fadingInChanged: function() {
      if (this.fadingIn) {
        this.style.opacity = 1;
        if (this.timerId !== undefined) {
          clearTimeout(this.timerId);
          this.timerId = undefined;
        }
      } else {
        if (this.timerId === undefined) {
          this.timerId = setTimeout(
            function() {
              this.style.opacity = 0;
              this.timerId = undefined;
            }.bind(this), 3000);
        }
      }
    }
  });
;

  (function() {
    var dpi = '';

    Polymer('viewer-button', {
      src: '',
      latchable: false,
      ready: function() {
        if (!dpi) {
          var mql = window.matchMedia('(-webkit-min-device-pixel-ratio: 1.3');
          dpi = mql.matches ? 'hi' : 'low';
        }
      },
      srcChanged: function() {
        if (this.src) {
          this.$.icon.style.backgroundImage =
              'url(' + this.getAttribute('assetpath') + 'img/' + dpi +
              'DPI/' + this.src + ')';
        } else {
          this.$.icon.style.backgroundImage = '';
        }
      },
      latchableChanged: function() {
        if (this.latchable)
          this.classList.add('latchable');
        else
          this.classList.remove('latchable');
      },
    });
  })();
;

  Polymer('viewer-page-indicator', {
    text: '1',
    timerId: undefined,
    ready: function() {
      var scrollCallback = function() {
        var percent = window.scrollY /
            (document.body.scrollHeight -
             document.documentElement.clientHeight);
        this.style.top = percent *
            (document.documentElement.clientHeight - this.offsetHeight) + 'px';
        this.style.opacity = 1;
        clearTimeout(this.timerId);

        this.timerId = setTimeout(function() {
          this.style.opacity = 0;
          this.timerId = undefined;
        }.bind(this), 2000);
      }.bind(this);
      window.addEventListener('scroll', function() {
        requestAnimationFrame(scrollCallback);
      });

      scrollCallback();
    },
  });
;

  Polymer('viewer-progress-bar', {
    progress: 0,
    text: 'Loading',
    numSegments: 8,
    segments: [],
    ready: function() {
      this.numSegmentsChanged();
    },
    progressChanged: function() {
      var numVisible = this.progress * this.segments.length / 100.0;
      for (var i = 0; i < this.segments.length; i++) {
        this.segments[i].style.visibility =
            i < numVisible ? 'visible' : 'hidden';
      }

      if (this.progress >= 100 || this.progress < 0)
        this.style.opacity = 0;
    },
    numSegmentsChanged: function() {
      // Clear the existing segments.
      this.segments = [];
      var segmentsElement = this.$.segments;
      segmentsElement.innerHTML = '';

      // Create the new segments.
      var segment = document.createElement('li');
      segment.classList.add('segment');
      var angle = 360 / this.numSegments;
      for (var i = 0; i < this.numSegments; ++i) {
        var segmentCopy = segment.cloneNode(true);
        segmentCopy.style.webkitTransform =
            'rotate(' + (i * angle) + 'deg) skewY(' +
            -1 * (90 - angle) + 'deg)';
        segmentsElement.appendChild(segmentCopy);
        this.segments.push(segmentCopy);
      }
      this.progressChanged();
    }
  });
