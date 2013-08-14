# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from PIL import Image
import sets
import sys
import unittest


sys.path.append(os.path.join(os.path.dirname(__file__), os.pardir))
from tests.rendering_test_manager import mock_cloud_bucket
from tests.rendering_test_manager import cloud_bucket
from tools import rendering_test_manager
from tools import image_tools


class BucketRenderingTestManagerUnitTest(unittest.TestCase):

  def setUp(self):
    # Set up structures that will be reused throughout testing.
    self.bucket = mock_cloud_bucket.MockCloudBucket()
    self.manager = rendering_test_manager.RenderingTestManager(self.bucket)
    self.white = Image.new('RGBA', (25, 25), (255, 255, 255, 255))
    self.red = Image.new('RGBA', (25, 25), (255, 0, 0, 255))
    self.black = Image.new('RGBA', (25, 25), (0, 0, 0, 255))

  def testUploadImage(self):
    self.bucket.Reset()
    # Upload some images to the datastore.
    self.manager.UploadImage('path/to/white.png', self.white)
    self.manager.UploadImage('path/to/black.png', self.black)
    self.manager.UploadImage('path/to/red.png', self.red)
    # Confirm that the images actually got uploaded.
    self.assertEquals(self.bucket.datastore['path/to/white.png'],
                      image_tools.SerializeImage(self.white).decode('base64'))
    self.assertEquals(self.bucket.datastore['path/to/black.png'],
                      image_tools.SerializeImage(self.black).decode('base64'))
    self.assertEquals(self.bucket.datastore['path/to/red.png'],
                      image_tools.SerializeImage(self.red).decode('base64'))

  def testDownloadImage(self):
    self.bucket.Reset()
    # Upload some images to the datastore.
    self.manager.UploadImage('path/to/white.png', self.white)
    self.manager.UploadImage('path/to/black.png', self.black)
    self.manager.UploadImage('path/to/red.png', self.red)
    # Check that the DownloadImage function gets the correct images.
    self.assertEquals(
        image_tools.SerializeImage(
            self.manager.DownloadImage('path/to/white.png')),
        image_tools.SerializeImage(self.white))
    self.assertEquals(
        image_tools.SerializeImage(
            self.manager.DownloadImage('path/to/black.png')),
        image_tools.SerializeImage(self.black))
    self.assertEquals(
        image_tools.SerializeImage(
            self.manager.DownloadImage('path/to/red.png')),
        image_tools.SerializeImage(self.red))
    # Check that the DownloadImage function throws an error for a
    #  nonexistant image.
    self.assertRaises(cloud_bucket.FileNotFoundError,
                      self.manager.DownloadImage,
                      'path/to/yellow.png')

  def testUploadTest(self):
    self.bucket.Reset()
    # Upload some tests to the datastore.
    self.manager.UploadTest('batch', 'test1', [self.white, self.black])
    self.manager.UploadTest('batch', 'test2', [self.black, self.black])
    # Confirm that the tests were successfully uploaded.
    self.assertEquals(self.bucket.datastore['tests/batch/test1/expected.png'],
                      image_tools.SerializeImage(self.white).decode('base64'))
    self.assertEquals(self.bucket.datastore['tests/batch/test1/mask.png'],
                      image_tools.SerializeImage(self.white).decode('base64'))
    self.assertEquals(self.bucket.datastore['tests/batch/test2/expected.png'],
                      image_tools.SerializeImage(self.black).decode('base64'))
    self.assertEquals(self.bucket.datastore['tests/batch/test2/mask.png'],
                      image_tools.SerializeImage(self.black).decode('base64'))

  def testRunTest(self):
    self.bucket.Reset()

    self.manager.UploadTest('batch', 'test1', [self.red, self.red])
    self.manager.RunTest('batch', 'test1', 'run1', self.black)
    self.assertEquals(
        self.bucket.datastore['failures/batch/test1/run1/actual.png'],
        image_tools.SerializeImage(self.black).decode('base64'))
    self.manager.RunTest('batch', 'test1', 'run2', self.red)
    self.assertFalse(
        self.bucket.datastore.has_key('failures/batch/test1/run2/actual.png'))
    self.assertRaises(cloud_bucket.FileNotFoundError, self.manager.RunTest,
                      'batch', 'test2', 'run1', self.black)

  def testGetTest(self):
    self.bucket.Reset()
    # Upload some tests to the datastore
    self.manager.UploadTest('batch', 'test1', [self.white, self.black])
    self.manager.UploadTest('batch', 'test2', [self.red, self.white])
    test1 = self.manager.GetTest('batch', 'test1')
    test2 = self.manager.GetTest('batch', 'test2')
    # Check that GetTest gets the appropriate tests.
    self.assertEquals(image_tools.SerializeImage(test1.expected),
                      image_tools.SerializeImage(self.white))
    self.assertEquals(image_tools.SerializeImage(test1.mask),
                      image_tools.SerializeImage(self.white))
    self.assertEquals(image_tools.SerializeImage(test2.expected),
                      image_tools.SerializeImage(self.red))
    self.assertEquals(image_tools.SerializeImage(test2.mask),
                      image_tools.SerializeImage(self.white))
    # Check that GetTest throws an error for a nonexistant test.
    self.assertRaises(
        cloud_bucket.FileNotFoundError, self.manager.GetTest, 'batch', 'test3')

  def testTestExists(self):
    self.bucket.Reset()
    self.manager.UploadTest('batch', 'test1', [self.white, self.black])
    self.manager.UploadTest('batch', 'test2', [self.white, self.black])
    self.assertTrue(self.manager.TestExists('batch', 'test1'))
    self.assertTrue(self.manager.TestExists('batch', 'test2'))
    self.assertFalse(self.manager.TestExists('batch', 'test3'))

  def testFailureExists(self):
    self.bucket.Reset()
    self.manager.UploadTest('batch', 'test1', [self.white, self.white])
    self.manager.RunTest('batch', 'test1', 'run1', self.black)
    self.manager.RunTest('batch', 'test1', 'run2', self.white)
    self.assertTrue(self.manager.FailureExists('batch', 'test1', 'run1'))
    self.assertFalse(self.manager.FailureExists('batch', 'test1', 'run2'))

  def testRemoveTest(self):
    self.bucket.Reset()
    self.manager.UploadTest('batch', 'test1', [self.white, self.white])
    self.manager.UploadTest('batch', 'test2', [self.white, self.white])
    self.assertTrue(self.manager.TestExists('batch', 'test1'))
    self.assertTrue(self.manager.TestExists('batch', 'test2'))
    self.manager.RemoveTest('batch', 'test1')
    self.assertFalse(self.manager.TestExists('batch', 'test1'))
    self.assertTrue(self.manager.TestExists('batch', 'test2'))
    self.manager.RemoveTest('batch', 'test2')
    self.assertFalse(self.manager.TestExists('batch', 'test1'))
    self.assertFalse(self.manager.TestExists('batch', 'test2'))

  def testRemoveFailure(self):
    self.bucket.Reset()
    self.manager.UploadTest('batch', 'test1', [self.white, self.white])
    self.manager.UploadTest('batch', 'test2', [self.white, self.white])
    self.manager.RunTest('batch', 'test1', 'run1', self.black)
    self.manager.RunTest('batch', 'test1', 'run2', self.black)
    self.manager.RemoveFailure('batch', 'test1', 'run1')
    self.assertFalse(self.manager.FailureExists('batch', 'test1', 'run1'))
    self.assertTrue(self.manager.TestExists('batch', 'test1'))
    self.assertTrue(self.manager.FailureExists('batch', 'test1', 'run2'))
    self.assertTrue(self.manager.TestExists('batch', 'test2'))

  def testGetFailure(self):
    self.bucket.Reset()
    # Upload a result
    self.manager.UploadTest('batch', 'test1', [self.red, self.red])
    self.manager.RunTest('batch', 'test1', 'run1', self.black)
    res = self.manager.GetFailure('batch', 'test1', 'run1')
    # Check that the function correctly got the result.
    self.assertEquals(image_tools.SerializeImage(res.expected),
                      image_tools.SerializeImage(self.red))
    self.assertEquals(image_tools.SerializeImage(res.diff),
                      image_tools.SerializeImage(self.white))
    self.assertEquals(image_tools.SerializeImage(res.actual),
                      image_tools.SerializeImage(self.black))
    # Check that the function raises an error when given non-existant results.
    self.assertRaises(cloud_bucket.FileNotFoundError,
                      self.manager.GetFailure,
                      'batch', 'test1', 'run2')
    self.assertRaises(cloud_bucket.FileNotFoundError,
                      self.manager.GetFailure,
                      'batch', 'test2', 'run1')

  def testGetAllPaths(self):
    self.bucket.Reset()
    # Upload some tests.
    self.manager.UploadTest('batch', 'test1', [self.white, self.black])
    self.manager.UploadTest('batch', 'test2', [self.red, self.white])
    # Check that the function gets all urls matching the prefix.
    self.assertEquals(
        sets.Set(self.manager.GetAllPaths('tests/batch/test')),
        sets.Set(['tests/batch/test1/expected.png',
                  'tests/batch/test1/mask.png',
                  'tests/batch/test2/expected.png',
                  'tests/batch/test2/mask.png']))
    self.assertEquals(sets.Set(self.manager.GetAllPaths('tests/batch/test1')),
                      sets.Set(['tests/batch/test1/expected.png',
                                'tests/batch/test1/mask.png']))
    self.assertEquals(sets.Set(self.manager.GetAllPaths('tests/batch/test3')),
                      sets.Set())


if __name__ == '__main__':
  unittest.main()
