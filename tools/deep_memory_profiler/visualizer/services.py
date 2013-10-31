# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is expected to be used under another directory to use,
# so we disable checking import path of GAE tools from this directory.
# pylint: disable=F0401,E0611,W0232

import json

from google.appengine.ext import blobstore
from google.appengine.ext import ndb


class Profiler(ndb.Model):
  """Profiler entity to store json data. Use run_id as its key.
  Json data will be stored at blobstore, but can be referred by BlobKey."""
  blob_key = ndb.BlobKeyProperty()


class Template(ndb.Model):
  """Template to breakdown profiler with multiple tags.
  Use content as its key."""
  content = ndb.JsonProperty()


def CreateProfiler(blob_info):
  """Create Profiler entity in database of uploaded file. Return run_id."""
  json_str = blob_info.open().read()
  json_obj = json.loads(json_str)

  # Check the uniqueness of data run_id and store new one.
  run_id = json_obj['run_id']
  prof_key = ndb.Key('Profiler', run_id)
  if not prof_key.get():
    # Profile for this run_id does not exist
    profiler = Profiler(id=run_id, blob_key=blob_info.key())
    profiler.put()

  return run_id


def GetProfiler(run_id):
  """Get Profiler entity from database of given run_id."""
  # Get entity key.
  profiler = ndb.Key('Profiler', run_id).get()
  return blobstore.BlobReader(profiler.blob_key).read()


def CreateTemplates(blob_info):
  """Create Template entities for all templates of uploaded file. Return ndb.Key
  of default template or None if not indicated or found in templates."""
  json_str = blob_info.open().read()
  json_obj = json.loads(json_str)

  # Return None when no default template indicated.
  if 'default_template' not in json_obj:
    return None
  # Return None when no default template found in templates.
  if json_obj['default_template'] not in json_obj['templates']:
    return None

  # Check the uniqueness of template content and store new one.
  for tag, content in json_obj['templates'].iteritems():
    content_str = json.dumps(content)
    tmpl_key = ndb.Key('Template', content_str)
    if tag == json_obj['default_template']:
      default_key = tmpl_key
    if not tmpl_key.get():
      # Template of the same content does not exist.
      template = Template(id=content_str, content=content)
      template.put()

  return default_key


def CreateTemplate(content):
  """Create Template entity for user to share."""
  content_str = json.dumps(content)
  tmpl_key = ndb.Key('Template', content_str)
  if not tmpl_key.get():
    # Template of the same content does not exist.
    template = Template(id=content_str, content=content)
    template.put()

  return tmpl_key


def GetTemplate(tmpl_id):
  """Get Template entity of given tmpl_id generated by ndb.Key."""
  # Get entity key.
  template = ndb.Key(urlsafe=tmpl_id).get()
  return json.dumps(template.content)
