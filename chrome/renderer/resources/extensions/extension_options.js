// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var DocumentNatives = requireNative('document_natives');
var ExtensionOptionsEvents =
    require('extensionOptionsEvents').ExtensionOptionsEvents;
var GuestViewContainer = require('guestViewContainer').GuestViewContainer;
var GuestViewInternal =
    require('binding').Binding.create('guestViewInternal').generate();
var IdGenerator = requireNative('id_generator');
var utils = require('utils');
var guestViewInternalNatives = requireNative('guest_view_internal');

// Mapping of the autosize attribute names to default values
var AUTO_SIZE_ATTRIBUTES = {
  'autosize': 'on',
  'maxheight': window.innerHeight,
  'maxwidth': window.innerWidth,
  'minheight': 32,
  'minwidth': 32
};

function ExtensionOptionsImpl(extensionoptionsElement) {
  GuestViewContainer.call(this, extensionoptionsElement)

  this.viewInstanceId = IdGenerator.GetNextId();
  this.guestInstanceId = 0;
  this.pendingGuestCreation = false;

  this.autosizeDeferred = false;

  // on* Event handlers.
  this.eventHandlers = {};

  // setupEventProperty is normally called in extension_options_events.js to
  // register events, but the createfailed event is registered here because
  // the event is fired from here instead of through
  // extension_options_events.js.
  this.setupEventProperty('createfailed');
  new ExtensionOptionsEvents(this, this.viewInstanceId);

  this.setupElementProperties();

  this.parseExtensionAttribute();

  // Once the browser plugin has been created, the guest view will be created
  // and attached. See handleBrowserPluginAttributeMutation().
  var shadowRoot = this.element.createShadowRoot();
  shadowRoot.appendChild(this.browserPluginElement);
};

ExtensionOptionsImpl.prototype.__proto__ = GuestViewContainer.prototype;

ExtensionOptionsImpl.VIEW_TYPE = 'ExtensionOptions';

// Add extra functionality to |this.element|.
ExtensionOptionsImpl.setupElement = function(proto) {
  var apiMethods = [
    'setDeferAutoSize',
    'resumeDeferredAutoSize'
  ];

  // Forward proto.foo* method calls to ExtensionOptionsImpl.foo*.
  GuestViewContainer.forwardApiMethods(proto, apiMethods);
}

ExtensionOptionsImpl.prototype.onElementDetached = function() {
  if (this.guestInstanceId) {
    GuestViewInternal.destroyGuest(this.guestInstanceId);
    this.guestInstanceId = undefined;
  }
};

ExtensionOptionsImpl.prototype.attachWindow = function() {
  return guestViewInternalNatives.AttachGuest(
      this.internalInstanceId,
      this.guestInstanceId,
      {
        'autosize': this.element.hasAttribute('autosize'),
        'instanceId': this.viewInstanceId,
        'maxheight': parseInt(this.maxheight || 0),
        'maxwidth': parseInt(this.maxwidth || 0),
        'minheight': parseInt(this.minheight || 0),
        'minwidth': parseInt(this.minwidth || 0)
      });
};

ExtensionOptionsImpl.prototype.createGuestIfNecessary = function() {
  if (!this.elementAttached || this.pendingGuestCreation) {
    return;
  }
  if (this.guestInstanceId != 0) {
    this.attachWindow();
    return;
  }
  var params = {
    'extensionId': this.extensionId,
  };
  GuestViewInternal.createGuest(
      'extensionoptions',
      params,
      function(guestInstanceId) {
        this.pendingGuestCreation = false;
        if (guestInstanceId && !this.elementAttached) {
          GuestViewInternal.destroyGuest(guestInstanceId);
          guestInstanceId = 0;
        }
        if (guestInstanceId == 0) {
          // Fire a createfailed event here rather than in ExtensionOptionsGuest
          // because the guest will not be created, and cannot fire an event.
          this.initCalled = false;
          var createFailedEvent = new Event('createfailed', { bubbles: true });
          this.dispatchEvent(createFailedEvent);
        } else {
          this.guestInstanceId = guestInstanceId;
          this.attachWindow();
        }
      }.bind(this)
  );
  this.pendingGuestCreation = true;
};

ExtensionOptionsImpl.prototype.dispatchEvent =
    function(extensionOptionsEvent) {
  return this.element.dispatchEvent(extensionOptionsEvent);
};

ExtensionOptionsImpl.prototype.handleAttributeMutation =
    function(name, oldValue, newValue) {
  // We treat null attribute (attribute removed) and the empty string as
  // one case.
  oldValue = oldValue || '';
  newValue = newValue || '';

  if (oldValue === newValue)
    return;

  if (name == 'extension' && !oldValue && newValue) {
    this.extensionId = newValue;
    // If the browser plugin is not ready then don't create the guest until
    // it is ready (in handleBrowserPluginAttributeMutation).
    if (!this.internalInstanceId)
      return;

    // If a guest view does not exist then create one.
    if (!this.guestInstanceId) {
      this.createGuestIfNecessary();
      return;
    }
    // TODO(ericzeng): Implement navigation to another guest view if we want
    // that functionality.
  } else if (AUTO_SIZE_ATTRIBUTES.hasOwnProperty(name) > -1) {
    this[name] = newValue;
    this.resetSizeConstraintsIfInvalid();

    if (!this.guestInstanceId)
      return;

    GuestViewInternal.setAutoSize(this.guestInstanceId, {
      'enableAutoSize': this.element.hasAttribute('autosize'),
      'min': {
        'width': parseInt(this.minwidth || 0),
        'height': parseInt(this.minheight || 0)
      },
      'max': {
        'width': parseInt(this.maxwidth || 0),
        'height': parseInt(this.maxheight || 0)
      }
    });
  }
};

ExtensionOptionsImpl.prototype.handleBrowserPluginAttributeMutation =
    function(name, oldValue, newValue) {
  if (name == 'internalinstanceid' && !oldValue && !!newValue) {
    this.elementAttached = true;
    this.internalInstanceId = parseInt(newValue);
    this.browserPluginElement.removeAttribute('internalinstanceid');
    if (this.extensionId)
      this.createGuestIfNecessary();
  }
};

ExtensionOptionsImpl.prototype.onSizeChanged =
    function(newWidth, newHeight, oldWidth, oldHeight) {
  if (this.autosizeDeferred) {
    this.deferredAutoSizeState = {
      newWidth: newWidth,
      newHeight: newHeight,
      oldWidth: oldWidth,
      oldHeight: oldHeight
    };
  } else {
    this.resize(newWidth, newHeight, oldWidth, oldHeight);
  }
};

ExtensionOptionsImpl.prototype.parseExtensionAttribute = function() {
  if (this.element.hasAttribute('extension')) {
    this.extensionId = this.element.getAttribute('extension');
    return true;
  }
  return false;
};

ExtensionOptionsImpl.prototype.resize =
    function(newWidth, newHeight, oldWidth, oldHeight) {
  this.browserPluginElement.style.width = newWidth + 'px';
  this.browserPluginElement.style.height = newHeight + 'px';

  // Do not allow the options page's dimensions to shrink so that the options
  // page has a consistent UI. If the new size is larger than the minimum,
  // make that the new minimum size.
  if (newWidth > this.minwidth)
    this.minwidth = newWidth;
  if (newHeight > this.minheight)
    this.minheight = newHeight;

  GuestViewInternal.setAutoSize(this.guestInstanceId, {
    'enableAutoSize': this.element.hasAttribute('autosize'),
    'min': {
      'width': parseInt(this.minwidth || 0),
      'height': parseInt(this.minheight || 0)
    },
    'max': {
      'width': parseInt(this.maxwidth || 0),
      'height': parseInt(this.maxheight || 0)
    }
  });
};

// Adds an 'on<event>' property on the view, which can be used to set/unset
// an event handler.
ExtensionOptionsImpl.prototype.setupEventProperty = function(eventName) {
  var propertyName = 'on' + eventName.toLowerCase();
  var element = this.element;
  Object.defineProperty(element, propertyName, {
    get: function() {
      return this.eventHandlers[propertyName];
    }.bind(this),
    set: function(value) {
      if (this.eventHandlers[propertyName])
        element.removeEventListener(
            eventName, this.eventHandlers[propertyName]);
      this.eventHandlers[propertyName] = value;
      if (value)
        element.addEventListener(eventName, value);
    }.bind(this),
    enumerable: true
  });
};

ExtensionOptionsImpl.prototype.setupElementProperties = function() {
  utils.forEach(AUTO_SIZE_ATTRIBUTES, function(attributeName) {
    // Get the size constraints from the <extensionoptions> tag, or use the
    // defaults if not specified
    if (this.element.hasAttribute(attributeName)) {
      this[attributeName] =
          this.element.getAttribute(attributeName);
    } else {
      this[attributeName] = AUTO_SIZE_ATTRIBUTES[attributeName];
    }

    Object.defineProperty(this.element, attributeName, {
      get: function() {
        return this[attributeName];
      }.bind(this),
      set: function(value) {
        this.element.setAttribute(attributeName, value);
      }.bind(this),
      enumerable: true
    });
  }, this);

  this.resetSizeConstraintsIfInvalid();

  Object.defineProperty(this.element, 'extension', {
    get: function() {
      return this.extensionId;
    }.bind(this),
    set: function(value) {
      this.element.setAttribute('extension', value);
    }.bind(this),
    enumerable: true
  });
};

ExtensionOptionsImpl.prototype.resetSizeConstraintsIfInvalid = function () {
  if (this.minheight > this.maxheight || this.minheight < 0) {
    this.minheight = AUTO_SIZE_ATTRIBUTES.minheight;
    this.maxheight = AUTO_SIZE_ATTRIBUTES.maxheight;
  }
  if (this.minwidth > this.maxwidth || this.minwidth < 0) {
    this.minwidth = AUTO_SIZE_ATTRIBUTES.minwidth;
    this.maxwidth = AUTO_SIZE_ATTRIBUTES.maxwidth;
  }
};

/**
 * Toggles whether the element should automatically resize to its preferred
 * size. If set to true, when the element receives new autosize dimensions,
 * it passes them to the embedder in a sizechanged event, but does not resize
 * itself to those dimensions until the embedder calls resumeDeferredAutoSize.
 * This allows the embedder to defer the resizing until it is ready.
 * When set to false, the element resizes whenever it receives new autosize
 * dimensions.
 */
ExtensionOptionsImpl.prototype.setDeferAutoSize = function(value) {
  if (!value)
    resumeDeferredAutoSize();
  this.autosizeDeferred = value;
};

/**
 * Allows the element to resize to most recent set of autosize dimensions if
 * autosizing is being deferred.
 */
ExtensionOptionsImpl.prototype.resumeDeferredAutoSize = function() {
  if (this.autosizeDeferred) {
    this.resize(this.deferredAutoSizeState.newWidth,
                this.deferredAutoSizeState.newHeight,
                this.deferredAutoSizeState.oldWidth,
                this.deferredAutoSizeState.oldHeight);
  }
};

GuestViewContainer.listenForReadyStateChange(ExtensionOptionsImpl);
