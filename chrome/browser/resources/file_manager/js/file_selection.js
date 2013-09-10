// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * The current selection object.
 *
 * @param {FileManager} fileManager FileManager instance.
 * @param {Array.<number>} indexes Selected indexes.
 * @constructor
 */
function FileSelection(fileManager, indexes) {
  this.fileManager_ = fileManager;
  this.computeBytesSequence_ = 0;
  this.indexes = indexes;
  this.entries = [];
  this.urls = [];
  this.totalCount = 0;
  this.fileCount = 0;
  this.directoryCount = 0;
  this.bytes = 0;
  this.showBytes = false;
  this.allDriveFilesPresent = false,
  this.iconType = null;
  this.bytesKnown = false;
  this.mustBeHidden_ = false;
  this.mimeTypes = null;

  // Synchronously compute what we can.
  for (var i = 0; i < this.indexes.length; i++) {
    var entry = fileManager.getFileList().item(this.indexes[i]);
    if (!entry)
      continue;

    this.entries.push(entry);
    this.urls.push(entry.toURL());

    if (this.iconType == null) {
      this.iconType = FileType.getIcon(entry);
    } else if (this.iconType != 'unknown') {
      var iconType = FileType.getIcon(entry);
      if (this.iconType != iconType)
        this.iconType = 'unknown';
    }

    if (entry.isFile) {
      this.fileCount += 1;
    } else {
      this.directoryCount += 1;
    }
    this.totalCount++;
  }

  this.tasks = new FileTasks(this.fileManager_);

  Object.seal(this);
}

/**
 * Computes data required to get file tasks and requests the tasks.
 *
 * @param {function} callback The callback.
 */
FileSelection.prototype.createTasks = function(callback) {
  if (!this.fileManager_.isOnDrive()) {
    this.tasks.init(this.urls);
    callback();
    return;
  }

  this.fileManager_.metadataCache_.get(this.urls, 'drive', function(props) {
    var present = props.filter(function(p) { return p && p.availableOffline });
    this.allDriveFilesPresent = present.length == props.length;

    // Collect all of the mime types and push that info into the selection.
    this.mimeTypes = props.map(function(value) {
      return (value && value.contentMimeType) || '';
    });

    this.tasks.init(this.urls, this.mimeTypes);
    callback();
  }.bind(this));
};

/**
 * Computes the total size of selected files.
 *
 * @param {function} callback Completion callback. Not called when cancelled,
 *     or a new call has been invoked in the meantime.
 */
FileSelection.prototype.computeBytes = function(callback) {
  if (this.entries.length == 0) {
    this.bytesKnown = true;
    this.showBytes = false;
    this.bytes = 0;
    return;
  }

  var computeBytesSequence = ++this.computeBytesSequence_;
  var pendingMetadataCount = 0;

  var maybeDone = function() {
    if (pendingMetadataCount == 0) {
      this.bytesKnown = true;
      callback();
    }
  }.bind(this);

  var onProps = function(properties) {
    // Ignore if the call got cancelled, or there is another new one fired.
    if (computeBytesSequence != this.computeBytesSequence_)
      return;

    // It may happen that the metadata is not available because a file has been
    // deleted in the meantime.
    if (properties)
      this.bytes += properties.size;
    pendingMetadataCount--;
    maybeDone();
  }.bind(this);

  for (var index = 0; index < this.entries.length; index++) {
    var entry = this.entries[index];
    if (entry.isFile) {
      this.showBytes |= !FileType.isHosted(entry);
      pendingMetadataCount++;
      this.fileManager_.metadataCache_.get(entry, 'filesystem', onProps);
    } else if (entry.isDirectory) {
      // Don't compute the directory size as it's expensive.
      // crbug.com/179073.
      this.showBytes = false;
      break;
    }
  }
  maybeDone();
};

/**
 * Cancels any async computation by increasing the sequence number. Results
 * of any previous call to computeBytes() will be discarded.
 *
 * @private
 */
FileSelection.prototype.cancelComputing_ = function() {
  this.computeBytesSequence_++;
};

/**
 * This object encapsulates everything related to current selection.
 *
 * @param {FileManager} fileManager File manager instance.
 * @extends {cr.EventTarget}
 * @constructor
 */
function FileSelectionHandler(fileManager) {
  this.fileManager_ = fileManager;
  // TODO(dgozman): create a shared object with most of UI elements.
  this.okButton_ = fileManager.okButton_;
  this.filenameInput_ = fileManager.filenameInput_;
  this.previewPanel_ = fileManager.previewPanel_;
  this.previewPanelElement_ =
      fileManager.dialogDom_.querySelector('.preview-panel');
  this.previewThumbnails_ = this.previewPanelElement_.
      querySelector('.preview-thumbnails');
  this.previewSummary_ =
      this.previewPanelElement_.querySelector('.preview-summary');
  this.previewText_ = this.previewSummary_.querySelector('.preview-text');
  this.calculatingSize_ = this.previewSummary_.
      querySelector('.calculating-size');
  this.calculatingSize_.textContent = str('CALCULATING_SIZE');

  this.searchBreadcrumbs_ = fileManager.searchBreadcrumbs_;
  this.taskItems_ = fileManager.taskItems_;

  this.animationTimeout_ = null;
}

/**
 * FileSelectionHandler extends cr.EventTarget.
 */
FileSelectionHandler.prototype.__proto__ = cr.EventTarget.prototype;

/**
 * Maximum amount of thumbnails in the preview pane.
 *
 * @const
 * @type {number}
 */
FileSelectionHandler.MAX_PREVIEW_THUMBNAIL_COUNT = 4;

/**
 * Maximum width or height of an image what pops up when the mouse hovers
 * thumbnail in the bottom panel (in pixels).
 *
 * @const
 * @type {number}
 */
FileSelectionHandler.IMAGE_HOVER_PREVIEW_SIZE = 200;

/**
 * Update the UI when the selection model changes.
 *
 * @param {cr.Event} event The change event.
 */
FileSelectionHandler.prototype.onFileSelectionChanged = function(event) {
  var indexes =
      this.fileManager_.getCurrentList().selectionModel.selectedIndexes;
  if (this.selection) this.selection.cancelComputing_();
  var selection = new FileSelection(this.fileManager_, indexes);
  this.selection = selection;

  if (this.fileManager_.dialogType == DialogType.SELECT_SAVEAS_FILE) {
    // If this is a save-as dialog, copy the selected file into the filename
    // input text box.
    if (this.selection.totalCount == 1 &&
        this.selection.entries[0].isFile &&
        this.filenameInput_.value != this.selection.entries[0].name) {
      this.filenameInput_.value = this.selection.entries[0].name;
    }
  }

  this.updateOkButton();

  if (this.selectionUpdateTimer_) {
    clearTimeout(this.selectionUpdateTimer_);
    this.selectionUpdateTimer_ = null;
  }

  // The rest of the selection properties are computed via (sometimes lengthy)
  // asynchronous calls. We initiate these calls after a timeout. If the
  // selection is changing quickly we only do this once when it slows down.

  var updateDelay = 200;
  var now = Date.now();
  if (now > (this.lastFileSelectionTime_ || 0) + updateDelay) {
    // The previous selection change happened a while ago. Update the UI soon.
    updateDelay = 0;
  }
  this.lastFileSelectionTime_ = now;

  this.selectionUpdateTimer_ = setTimeout(function() {
    this.selectionUpdateTimer_ = null;
    if (this.selection == selection)
      this.updateFileSelectionAsync(selection);
  }.bind(this), updateDelay);
};

/**
 * Updates the Ok button enabled state.
 *
 * @return {boolean} Whether button is enabled.
 */
FileSelectionHandler.prototype.updateOkButton = function() {
  var selectable;
  var dialogType = this.fileManager_.dialogType;

  if (dialogType == DialogType.SELECT_FOLDER ||
      dialogType == DialogType.SELECT_UPLOAD_FOLDER) {
    // In SELECT_FOLDER mode, we allow to select current directory
    // when nothing is selected.
    selectable = this.selection.directoryCount <= 1 &&
        this.selection.fileCount == 0;
  } else if (dialogType == DialogType.SELECT_OPEN_FILE) {
    selectable = (this.isFileSelectionAvailable() &&
                  this.selection.directoryCount == 0 &&
                  this.selection.fileCount == 1);
  } else if (dialogType == DialogType.SELECT_OPEN_MULTI_FILE) {
    selectable = (this.isFileSelectionAvailable() &&
                  this.selection.directoryCount == 0 &&
                  this.selection.fileCount >= 1);
  } else if (dialogType == DialogType.SELECT_SAVEAS_FILE) {
    if (this.fileManager_.isOnReadonlyDirectory()) {
      selectable = false;
    } else {
      selectable = !!this.filenameInput_.value;
    }
  } else if (dialogType == DialogType.FULL_PAGE) {
    // No "select" buttons on the full page UI.
    selectable = true;
  } else {
    throw new Error('Unknown dialog type');
  }

  this.okButton_.disabled = !selectable;
  return selectable;
};

/**
  * Check if all the files in the current selection are available. The only
  * case when files might be not available is when the selection contains
  * uncached Drive files and the browser is offline.
  *
  * @return {boolean} True if all files in the current selection are
  *                   available.
  */
FileSelectionHandler.prototype.isFileSelectionAvailable = function() {
  return !this.fileManager_.isOnDrive() ||
      !this.fileManager_.isDriveOffline() ||
      this.selection.allDriveFilesPresent;
};

/**
 * Calculates async selection stats and updates secondary UI elements.
 *
 * @param {FileSelection} selection The selection object.
 */
FileSelectionHandler.prototype.updateFileSelectionAsync = function(selection) {
  if (this.selection != selection) return;

  // Update the file tasks.
  if (this.fileManager_.dialogType == DialogType.FULL_PAGE &&
      selection.directoryCount == 0 && selection.fileCount > 0) {
    selection.createTasks(function() {
      if (this.selection != selection)
        return;
      selection.tasks.display(this.taskItems_);
      selection.tasks.updateMenuItem();
    }.bind(this));
  } else {
    this.taskItems_.hidden = true;
  }

  // Update preview panels.
  var wasVisible = this.previewPanel_.visible;
  var thumbnailSelection;
  if (selection.totalCount != 0) {
    thumbnailSelection = selection;
  } else {
    thumbnailSelection = {
      entries: [this.fileManager_.getCurrentDirectoryEntry()]
    };
  }
  this.previewPanel_.setSelection(selection);
  this.previewPanel_.thumbnails.selection = thumbnailSelection;

  // Update breadcrums.
  var updateTarget = null;
  var path = this.fileManager_.getCurrentDirectory();
  if (selection.totalCount == 1) {
    // Shows the breadcrumb list when a file is selected.
    updateTarget = selection.entries[0].fullPath;
  } else if (selection.totalCount == 0 &&
             this.previewPanel_.visible) {
    // Shows the breadcrumb list when no file is selected and the preview
    // panel is visible.
    updateTarget = path;
  }
  this.updatePreviewPanelBreadcrumbs_(updateTarget);

  // Scroll to item
  if (!wasVisible && this.selection.totalCount == 1) {
    var list = this.fileManager_.getCurrentList();
    list.scrollIndexIntoView(list.selectionModel.selectedIndex);
  }

  // Sync the commands availability.
  if (selection.totalCount != 0)
    this.fileManager_.updateCommands();

  // Update context menu.
  this.fileManager_.updateContextMenuActionItems(null, false);

  // Inform tests it's OK to click buttons now.
  if (selection.totalCount > 0) {
    chrome.test.sendMessage('selection-change-complete');
  }
};

/**
 * Updates the breadcrumbs in the preview panel.
 *
 * @param {?string} path Path to be shown in the breadcrumbs list
 * @private
 */
FileSelectionHandler.prototype.updatePreviewPanelBreadcrumbs_ = function(path) {
  if (!path)
    this.searchBreadcrumbs_.hide();
  else
    this.searchBreadcrumbs_.show(PathUtil.getRootPath(path), path);
};

/**
 * Updates the search breadcrumbs. This method should not be used in the new ui.
 *
 * @private
 */
FileSelectionHandler.prototype.updateSearchBreadcrumbs_ = function() {
  var selectedIndexes =
      this.fileManager_.getCurrentList().selectionModel.selectedIndexes;
  if (selectedIndexes.length !== 1 ||
      !this.fileManager_.directoryModel_.isSearching()) {
    this.searchBreadcrumbs_.hide();
    return;
  }

  var entry = this.fileManager_.getFileList().item(
      selectedIndexes[0]);
  this.searchBreadcrumbs_.show(
      PathUtil.getRootPath(entry.fullPath),
      entry.fullPath);
};
