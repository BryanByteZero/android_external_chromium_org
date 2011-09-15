#!/usr/bin/python
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import pyauto_functional
import pyauto

import chromeos.file_browser
import test_utils


class ChromeosFileBrowserTest(pyauto.PyUITest):
  """Tests for ChromeOS File Browser (full page and dialog)."""

  def _GetFullPageFileBrowser(self, tab_index=0, windex=0):
    """Display the full page file browser in the current tab.

    Returns:
      ChromeosFileBrowser object.
    """
    self.NavigateToURL('chrome://files')
    executor = pyauto.PyUITest.JavascriptExecutorInTab(self, tab_index, windex)
    file_browser = chromeos.file_browser.FileBrowser(self, executor)
    if file_browser.WaitUntilInitialized():
      return file_browser
    else:
      return None

  def _GetSaveAsDialogFileBrowser(self):
    """Display the save-as file browser dialog.

    The current tab should not be 'about:blank'.

    Returns:
      ChromeosFileBrowser object.
    """
    self.ApplyAccelerator(pyauto.IDC_SAVE_PAGE)
    dialog = self.WaitUntilExtensionViewLoaded(view_type='EXTENSION_DIALOG')
    self.assertTrue(
        dialog,
        msg='Could not find a loaded "save-as" file browser'
            'dialog (views = %s).' % self.GetBrowserInfo()['extension_views'])
    executor = \
        pyauto.PyUITest.JavascriptExecutorInRenderView(self, dialog)
    file_browser = chromeos.file_browser.FileBrowser(self, executor)
    if file_browser.WaitUntilInitialized():
      return file_browser
    else:
      return None

  def _GetOpenDialogFileBrowser(self):
    """Display the open file browser dialog.

    Returns:
      ChromeosFileBrowser object.
    """
    self.ApplyAccelerator(pyauto.IDC_OPEN_FILE)
    dialog = self.WaitUntilExtensionViewLoaded(view_type='EXTENSION_DIALOG')
    self.assertTrue(
        dialog,
        msg='Could not find a loaded "open" file browser dialog: views = %s.' %
        self.GetBrowserInfo()['extension_views'])
    executor = \
        pyauto.PyUITest.JavascriptExecutorInRenderView(self, dialog)
    file_browser = chromeos.file_browser.FileBrowser(self, executor)
    if file_browser.WaitUntilInitialized():
      return file_browser
    else:
      return None

  def testOpenWavFile(self):
    """Test we can open a 'wav' file from the file browser dialog."""
    test_utils.CopyFileFromDataDirToDownloadDir(self, 'media/bear.wav')
    file_browser = self._GetOpenDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self.assertTrue(file_browser.Select('bear.wav'),
                    msg='"bear.wav" does not exist.')
    dialog = self.WaitUntilExtensionViewLoaded(view_type='EXTENSION_DIALOG')
    file_browser.Open()
    self.assertTrue(self.WaitUntilExtensionViewClosed(dialog),
                    msg='File browser dialog was not closed.')

  def testSavePage(self):
    """Test we can save the current page using the file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    dialog = self.WaitUntilExtensionViewLoaded(view_type='EXTENSION_DIALOG')
    file_browser.Save('apple')
    self.assertTrue(self.WaitUntilExtensionViewClosed(dialog))
    file_browser = self._GetOpenDialogFileBrowser()
    self.assertTrue(file_browser.Select('apple.html'))

  def testSelectMultipleFoldersInFullPage(self):
    """Test we can select multiple folders in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    file_browser.CreateDirectory('apples')
    file_browser.CreateDirectory('oranges')
    self.assertEqual(file_browser.DirectoryContents(),
                     set(['apples', 'oranges']),
                     msg='Failed to create directories (list = %s).' %
                         file_browser.DirectoryContents())
    file_browser.Select('apples')
    file_browser.Select('oranges')
    file_browser.Delete()
    self.assertFalse(file_browser.DirectoryContents(),
                     msg='Failed to delete directories (list = %s).' %
                         file_browser.DirectoryContents())

  def _CreateFolder(self, file_browser):
    """Create folders and then change into them."""
    top_directory = file_browser.CurrentDirectory()
    tree = ['deep', 'deeper', 'deepest']
    for directory in tree:
      file_browser.CreateDirectory(directory)
      file_browser.ChangeDirectory(directory)
    self.assertEqual(file_browser.CurrentDirectory(),
                     top_directory + '/' + '/'.join(tree),
                     msg='Ended up in wrong directory (%s)' %
                         file_browser.CurrentDirectory())

  def testCreateFolderInFullPage(self):
    """Test we can create a folder in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CreateFolder(file_browser)

  def testCreateFolderInDialog(self):
    """Test we can create a folder in a save-as file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CreateFolder(file_browser)

  def _RenameFolder(self, file_browser):
    """Create a folder and then rename it."""
    file_browser.CreateDirectory('apples')
    file_browser.Select('apples')
    file_browser.Rename('oranges')
    self.assertFalse(file_browser.Select('apples'))
    self.assertTrue(file_browser.Select('oranges'))

  def testRenameFolderInFullPage(self):
    """Test we can rename a folder in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._RenameFolder(file_browser)

  def testRenameFolderInDialog(self):
    """Test we can rename a folder in a save-as file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._RenameFolder(file_browser)

  def _DeleteFolder(self, file_browser):
    """Create a folder and then delete it."""
    file_browser.CreateDirectory('apples')
    file_browser.Select('apples')
    file_browser.Delete()
    self.assertFalse(file_browser.Select('apples'))

  def testDeleteFolderInFullPage(self):
    """Test we can delete a folder in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._DeleteFolder(file_browser)

  def testDeleteFolderInDialog(self):
    """Test we can delete a folder in a save-as file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._DeleteFolder(file_browser)

  def _CopyFolder(self, file_browser):
    """Create a folder and then copy and paste it to itself."""
    file_browser.CreateDirectory('apples')
    file_browser.Select('apples')
    file_browser.Copy()
    file_browser.ChangeDirectory('apples')
    file_browser.Paste()
    self.assertTrue(file_browser.Select('apples'))

  def testCopyFolderInFullPage(self):
    """Test we can copy and paste a folder in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CopyFolder(file_browser)

  def testCopyFolderInDialog(self):
    """Test we can copy and paste a folder in a save-as file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CopyFolder(file_browser)

  def _CutFolder(self, file_browser):
    """Create two folders and cut and paste one inside the other."""
    top_directory = file_browser.CurrentDirectory()
    file_browser.CreateDirectory('apples')
    file_browser.Select('apples')
    file_browser.Cut()
    file_browser.CreateDirectory('oranges')
    file_browser.ChangeDirectory('oranges')
    file_browser.Paste()
    self.assertTrue(file_browser.Select('apples'))
    file_browser.ChangeDirectory(top_directory)
    self.assertFalse(file_browser.Select('apples'))

  def testCutFolderInFullPage(self):
    """Test we can cut and paste a folder in the full page file browser."""
    file_browser = self._GetFullPageFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CutFolder(file_browser)

  def testCutFolderInDialog(self):
    """Test we can cut and paste a folder in a save-as file browser dialog."""
    self.NavigateToURL('chrome://version')
    file_browser = self._GetSaveAsDialogFileBrowser()
    self.assertTrue(file_browser, msg='File browser failed to initialize.')
    self._CutFolder(file_browser)


if __name__ == '__main__':
  pyauto_functional.Main()
