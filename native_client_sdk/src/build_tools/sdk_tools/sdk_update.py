#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''A simple tool to update the Native Client SDK to the latest version'''

import cStringIO
import cygtar
import errno
import exceptions
import hashlib
import json
import manifest_util
import optparse
import os
import shutil
import subprocess
import sys
import tempfile
from third_party import fancy_urllib
import time
import urllib2
import urlparse


#------------------------------------------------------------------------------
# Constants

# Bump the MINOR_REV every time you check this file in.
MAJOR_REV = 2
MINOR_REV = 17

GLOBAL_HELP = '''Usage: naclsdk [options] command [command_options]

naclsdk is a simple utility that updates the Native Client (NaCl)
Software Developer's Kit (SDK).  Each component is kept as a 'bundle' that
this utility can download as as subdirectory into the SDK.

Commands:
  help [command] - Get either general or command-specific help
  list - Lists the available bundles
  update/install - Updates/installs bundles in the SDK
  sources - Manage external package sources

Example Usage:
  naclsdk list
  naclsdk update --force pepper_17
  naclsdk install recommended
  naclsdk help update
  naclsdk sources --list'''

CONFIG_FILENAME='naclsdk_config.json'
MANIFEST_FILENAME='naclsdk_manifest2.json'
SDK_TOOLS='sdk_tools'  # the name for this tools directory
USER_DATA_DIR='sdk_cache'

HTTP_CONTENT_LENGTH = 'Content-Length'  # HTTP Header field for content length


#------------------------------------------------------------------------------
# General Utilities


_debug_mode = False
_quiet_mode = False


def DebugPrint(msg):
  '''Display a message to stderr if debug printing is enabled

  Note: This function appends a newline to the end of the string

  Args:
    msg: A string to send to stderr in debug mode'''
  if _debug_mode:
    sys.stderr.write("%s\n" % msg)
    sys.stderr.flush()


def InfoPrint(msg):
  '''Display an informational message to stdout if not in quiet mode

  Note: This function appends a newline to the end of the string

  Args:
    mgs: A string to send to stdio when not in quiet mode'''
  if not _quiet_mode:
    sys.stdout.write("%s\n" % msg)
    sys.stdout.flush()


def WarningPrint(msg):
  '''Display an informational message to stderr.

  Note: This function appends a newline to the end of the string

  Args:
    mgs: A string to send to stderr.'''
  sys.stderr.write("WARNING: %s\n" % msg)
  sys.stderr.flush()


class Error(Exception):
  '''Generic error/exception for sdk_update module'''
  pass


def UrlOpen(url):
  request = fancy_urllib.FancyRequest(url)
  ca_certs = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          'cacerts.txt')
  request.set_ssl_info(ca_certs=ca_certs)
  url_opener = urllib2.build_opener(
      fancy_urllib.FancyProxyHandler(),
      fancy_urllib.FancyRedirectHandler(),
      fancy_urllib.FancyHTTPSHandler())
  return url_opener.open(request)

def ExtractInstaller(installer, outdir):
  '''Extract the SDK installer into a given directory

  If the outdir already exists, then this function deletes it

  Args:
    installer: full path of the SDK installer
    outdir: output directory where to extract the installer

  Raises:
    CalledProcessError - if the extract operation fails'''
  RemoveDir(outdir)

  if os.path.splitext(installer)[1] == '.exe':
    # If the installer has extension 'exe', assume it's a Windows NSIS-style
    # installer that handles silent (/S) and relocated (/D) installs.
    command = [installer, '/S', '/D=%s' % outdir]
    subprocess.check_call(command)
  else:
    os.mkdir(outdir)
    tar_file = None
    curpath = os.getcwd()
    try:
      tar_file = cygtar.CygTar(installer, 'r', verbose=True)
      if outdir: os.chdir(outdir)
      tar_file.Extract()
    finally:
      if tar_file:
        tar_file.Close()
      os.chdir(curpath)


def RemoveDir(outdir):
  '''Removes the given directory

  On Unix systems, this just runs shutil.rmtree, but on Windows, this doesn't
  work when the directory contains junctions (as does our SDK installer).
  Therefore, on Windows, it runs rmdir /S /Q as a shell command.  This always
  does the right thing on Windows. If the directory already didn't exist,
  RemoveDir will return successfully without taking any action.

  Args:
    outdir: The directory to delete

  Raises:
    CalledProcessError - if the delete operation fails on Windows
    OSError - if the delete operation fails on Linux
  '''

  DebugPrint('Removing %s' % outdir)
  try:
    shutil.rmtree(outdir)
  except:
    if not os.path.exists(outdir):
      return
    # On Windows this could be an issue with junctions, so try again with rmdir
    if sys.platform == 'win32':
      subprocess.check_call(['rmdir', '/S', '/Q', outdir], shell=True)


def RenameDir(srcdir, destdir):
  '''Renames srcdir to destdir. Removes destdir before doing the
     rename if it already exists.'''

  max_tries = 5
  for num_tries in xrange(max_tries):
    try:
      RemoveDir(destdir)
      os.rename(srcdir, destdir)
      return
    except OSError as err:
      if err.errno != errno.EACCES:
        raise err
      # If we are here, we didn't exit due to raised exception, so we are
      # handling a Windows flaky access error.  Sleep one second and try
      # again.
      time.sleep(num_tries + 1)
  # end of while loop -- could not RenameDir
  raise Error('Could not RenameDir %s => %s after %d tries.\n' %
              'Please check that no shells or applications '
              'are accessing files in %s.'
              % (srcdir, destdir, num_tries, destdir))


class ProgressFunction(object):
  '''Create a progress function for a file with a given size'''

  def __init__(self, file_size=0):
    '''Constructor

    Args:
      file_size: number of bytes in file.  0 indicates unknown'''
    self.dots = 0
    self.file_size = int(file_size)

  def GetProgressFunction(self):
    '''Returns a progress function based on a known file size'''
    def ShowKnownProgress(progress):
      if progress == 0:
        sys.stdout.write('|%s|\n' % ('=' * 48))
      else:
        new_dots = progress * 50 / self.file_size - self.dots
        sys.stdout.write('.' * new_dots)
        self.dots += new_dots
        if progress == self.file_size:
          sys.stdout.write('\n')
      sys.stdout.flush()

    return ShowKnownProgress


def DownloadArchiveToFile(archive, dest_path):
  '''Download the archive's data to a file at dest_path.

  As a side effect, computes the sha1 hash and data size, both returned as a
  tuple. Raises an Error if the url can't be opened, or an IOError exception if
  dest_path can't be opened.

  Args:
    dest_path: Path for the file that will receive the data.
  Return:
    A tuple (sha1, size) with the sha1 hash and data size respectively.'''
  sha1 = None
  size = 0
  with open(dest_path, 'wb') as to_stream:
    from_stream = None
    try:
      from_stream = UrlOpen(archive.url)
    except urllib2.URLError:
      raise Error('Cannot open "%s" for archive %s' %
          (archive.url, archive.host_os))
    try:
      content_length = int(from_stream.info()[HTTP_CONTENT_LENGTH])
      progress_function = ProgressFunction(content_length).GetProgressFunction()
      InfoPrint('Downloading %s' % archive.url)
      sha1, size = manifest_util.DownloadAndComputeHash(
          from_stream,
          to_stream=to_stream,
          progress_func=progress_function)
      if size != content_length:
        raise Error('Download size mismatch for %s.\n'
                    'Expected %s bytes but got %s' %
                    (archive.url, content_length, size))
    finally:
      if from_stream: from_stream.close()
  return sha1, size


def LoadFromFile(path, obj):
  '''Returns a manifest loaded from the JSON file at |path|.

  If the path does not exist or is invalid, returns unmodified object.'''
  methodlist = [m for m in dir(obj) if callable(getattr(obj, m))]
  if 'LoadDataFromString' not in methodlist:
    return obj
  if not os.path.exists(path):
    return obj

  with open(path, 'r') as f:
    json_string = f.read()
  if not json_string:
    return obj

  obj.LoadDataFromString(json_string)
  return obj


def LoadManifestFromURLs(urls):
  '''Returns a manifest loaded from |urls|, merged into one manifest.'''
  manifest = manifest_util.SDKManifest()
  for url in urls:
    try:
      url_stream = UrlOpen(url)
    except urllib2.URLError as e:
      raise Error('Unable to open %s. [%s]' % (url, e))

    manifest_stream = cStringIO.StringIO()
    sha1, size = manifest_util.DownloadAndComputeHash(url_stream,
                                                      manifest_stream)
    temp_manifest = manifest_util.SDKManifest()
    temp_manifest.LoadDataFromString(manifest_stream.getvalue())

    manifest.MergeManifest(temp_manifest)

  def BundleFilter(bundle):
    # Only add this bundle if it's supported on this platform.
    return bundle.GetHostOSArchive()

  manifest.FilterBundles(BundleFilter)
  return manifest


def WriteToFile(path, obj):
  '''Write |manifest| to a JSON file at |path|.'''
  methodlist = [m for m in dir(obj) if callable(getattr(obj, m))]
  if 'GetDataAsString' not in methodlist:
    raise Error('Unable to write object to file')
  json_string = obj.GetDataAsString()

  # Write the JSON data to a temp file.
  temp_file_name = None
  # TODO(dspringer): Use file locks here so that multiple sdk_updates can
  # run at the same time.
  with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
    f.write(json_string)
    temp_file_name = f.name
  # Move the temp file to the actual file.
  if os.path.exists(path):
    os.remove(path)
  shutil.move(temp_file_name, path)


class SDKConfig(object):
  '''This class contains utilities for manipulating an SDK config
  '''

  def __init__(self):
    '''Create a new SDKConfig object with default contents'''
    self._data = {
        'sources': [],
        }

  def AddSource(self, string):
    '''Add a source file to load packages from.

    Args:
      string: a URL to an external package manifest file.'''
    # For now whitelist only the following location for external sources:
    # https://commondatastorage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk
    (scheme, host, path, _, _, _) = urlparse.urlparse(string)
    if (host != 'commondatastorage.googleapis.com' or
        scheme != 'https' or
        not path.startswith('/nativeclient-mirror/nacl/nacl_sdk')):
      WarningPrint('Only whitelisted sources from '
                   '\'https://commondatastorage.googleapis.com/nativeclient-'
                   'mirror/nacl/nacl_sdk\' are currently allowed.')
      return
    if string in self._data['sources']:
      WarningPrint('source \''+string+'\' already exists in config.')
      return
    try:
      url_stream = UrlOpen(string)
    except urllib2.URLError:
      WarningPrint('Unable to fetch manifest URL \'%s\'. Exiting...' % string)
      return

    self._data['sources'].append(string)
    InfoPrint('source \''+string+'\' added to config.')

  def RemoveSource(self, string):
    '''Remove a source file to load packages from.

    Args:
      string: a URL to an external SDK manifest file.'''
    if string not in self._data['sources']:
      WarningPrint('source \''+string+'\' doesn\'t exist in config.')
    else:
      self._data['sources'].remove(string)
      InfoPrint('source \''+string+'\' removed from config.')

  def RemoveAllSources(self):
    if len(self.GetSources()) == 0:
      InfoPrint('There are no external sources to remove.')
    # Copy the list because RemoveSource modifies the underlying list
    sources = list(self.GetSources())
    for source in sources:
      self.RemoveSource(source)


  def ListSources(self):
    '''List all external sources in config.'''
    if len(self._data['sources']):
      InfoPrint('Installed sources:')
      for s in self._data['sources']:
        InfoPrint('  '+s)
    else:
      InfoPrint('No external sources installed')

  def GetSources(self):
    '''Return a list of external sources'''
    return self._data['sources']

  def LoadDataFromString(self, string):
    ''' Load a JSON config string. Raises an exception if string
        is not well-formed JSON.

    Args:
      string: a JSON-formatted string containing the previous config'''
    self._data = json.loads(string)


  def GetDataAsString(self):
    '''Returns the current JSON manifest object, pretty-printed'''
    pretty_string = json.dumps(self._data, sort_keys=False, indent=2)
    # json.dumps sometimes returns trailing whitespace and does not put
    # a newline at the end.  This code fixes these problems.
    pretty_lines = pretty_string.split('\n')
    return '\n'.join([line.rstrip() for line in pretty_lines]) + '\n'


#------------------------------------------------------------------------------
# Commands


def List(options, argv, config):
  '''Usage: %prog [options] list

  Lists the available SDK bundles that are available for download.'''
  def PrintBundles(bundles):
    for bundle in bundles:
      InfoPrint('  %s' % bundle.name)
      for key, value in bundle.iteritems():
        if key not in (manifest_util.ARCHIVES_KEY, manifest_util.NAME_KEY):
          InfoPrint('    %s: %s' % (key, value))

  DebugPrint("Running List command with: %s, %s" %(options, argv))

  parser = optparse.OptionParser(usage=List.__doc__)
  (list_options, args) = parser.parse_args(argv)
  manifest = LoadManifestFromURLs([options.manifest_url] + config.GetSources())
  InfoPrint('Available bundles:')
  PrintBundles(manifest.GetBundles())
  # Print the local information.
  manifest_path = os.path.join(options.user_data_dir, options.manifest_filename)
  local_manifest = LoadFromFile(manifest_path, manifest_util.SDKManifest())
  InfoPrint('\nCurrently installed bundles:')
  PrintBundles(local_manifest.GetBundles())


def Update(options, argv, config):
  '''Usage: %prog [options] update [target]

  Updates the Native Client SDK to a specified version.  By default, this
  command updates all the recommended components.  The update process works
  like this:
    1. Fetch the manifest from the mirror.
    2. Load manifest from USER_DATA_DIR - if there is no local manifest file,
       make an empty manifest object.
    3. Update each the bundle:
      for bundle in bundles:
        # Compare bundle versions & revisions.
        # Test if local version.revision < mirror OR local doesn't exist.
        if local_manifest < mirror_manifest:
          update(bundle)
          update local_manifest with mirror_manifest for bundle
          write manifest to disk.  Use locks.
        else:
          InfoPrint('bundle is up-to-date')

  Targets:
    recommended: (default) Install/Update all recommended components
    all:         Install/Update all available components
    bundle_name: Install/Update only the given bundle
  '''
  DebugPrint("Running Update command with: %s, %s" % (options, argv))
  ALL='all'  # Update all bundles
  RECOMMENDED='recommended'  # Only update the bundles with recommended=yes

  parser = optparse.OptionParser(usage=Update.__doc__)
  parser.add_option(
      '-F', '--force', dest='force',
      default=False, action='store_true',
      help='Force updating existing components that already exist')
  (update_options, args) = parser.parse_args(argv)
  if len(args) == 0:
    args = [RECOMMENDED]
  manifest = LoadManifestFromURLs([options.manifest_url] + config.GetSources())
  bundles = manifest.GetBundles()
  local_manifest_path = os.path.join(options.user_data_dir,
                                     options.manifest_filename)
  local_manifest = LoadFromFile(local_manifest_path,
                                manifest_util.SDKManifest())

  # Validate the arg list against the available bundle names.  Raises an
  # error if any invalid bundle names or args are detected.
  valid_args = set([ALL, RECOMMENDED] + [bundle.name for bundle in bundles])
  bad_args = set(args) - valid_args
  if len(bad_args) > 0:
    raise Error("Unrecognized bundle name or argument: '%s'" %
                ', '.join(bad_args))

  for bundle in bundles:
    bundle_path = os.path.join(options.sdk_root_dir, bundle.name)
    bundle_update_path = '%s_update' % bundle_path
    if not (bundle.name in args or
            ALL in args or (RECOMMENDED in args and
                            bundle[RECOMMENDED] == 'yes')):
      continue
    def UpdateBundle():
      '''Helper to install a bundle'''
      archive = bundle.GetHostOSArchive()
      (scheme, host, path, _, _, _) = urlparse.urlparse(archive['url'])
      dest_filename = os.path.join(options.user_data_dir, path.split('/')[-1])
      sha1, size = DownloadArchiveToFile(archive, dest_filename)
      if sha1 != archive.GetChecksum():
        raise Error("SHA1 checksum mismatch on '%s'.  Expected %s but got %s" %
                    (bundle.name, archive.GetChecksum(), sha1))
      if size != archive.size:
        raise Error("Size mismatch on Archive.  Expected %s but got %s bytes" %
                    (archive.size, size))
      InfoPrint('Updating bundle %s to version %s, revision %s' % (
                (bundle.name, bundle.version, bundle.revision)))
      ExtractInstaller(dest_filename, bundle_update_path)
      if bundle.name != SDK_TOOLS:
        repath = bundle.get('repath', None)
        if repath:
          bundle_move_path = os.path.join(bundle_update_path, repath)
        else:
          bundle_move_path = bundle_update_path
        RenameDir(bundle_move_path, bundle_path)
        if os.path.exists(bundle_update_path):
          RemoveDir(bundle_update_path)
      os.remove(dest_filename)
      local_manifest.MergeBundle(bundle)
      WriteToFile(local_manifest_path, local_manifest)
    # Test revision numbers, update the bundle accordingly.
    # TODO(dspringer): The local file should be refreshed from disk each
    # iteration thought this loop so that multiple sdk_updates can run at the
    # same time.
    if local_manifest.BundleNeedsUpdate(bundle):
      if (not update_options.force and os.path.exists(bundle_path) and
          bundle.name != SDK_TOOLS):
        WarningPrint('%s already exists, but has an update available.\n'
                     'Run update with the --force option to overwrite the '
                     'existing directory.\nWarning: This will overwrite any '
                     'modifications you have made within this directory.'
                     % bundle.name)
      else:
        UpdateBundle()
    else:
      InfoPrint('%s is already up-to-date.' % bundle.name)

def Sources(options, argv, config):
  '''Usage: %prog [options] sources [--list,--add URL,--remove URL]

  Manage additional package sources.  URL should point to a valid package
  manifest file for download.
  '''
  DebugPrint("Running Sources command with: %s, %s" % (options, argv))

  parser = optparse.OptionParser(usage=Sources.__doc__)
  parser.add_option(
      '-a', '--add', dest='url_to_add',
      default=None,
      help='Add additional package source')
  parser.add_option(
      '-r', '--remove', dest='url_to_remove',
      default=None,
      help='Remove package source (use \'all\' for all additional sources)')
  parser.add_option(
      '-l', '--list', dest='do_list',
      default=False, action='store_true',
      help='List additional package sources')
  (source_options, args) = parser.parse_args(argv)

  write_config = False
  if source_options.url_to_add:
    config.AddSource(source_options.url_to_add)
    write_config = True
  elif source_options.url_to_remove:
    if source_options.url_to_remove == 'all':
      config.RemoveAllSources()
    else:
      config.RemoveSource(source_options.url_to_remove)
    write_config = True
  elif source_options.do_list:
    config.ListSources()
  else:
    parser.print_help()

  if write_config:
    WriteToFile(os.path.join(options.user_data_dir, options.config_filename),
                config)

#------------------------------------------------------------------------------
# Command-line interface


def main(argv):
  '''Main entry for the sdk_update utility'''
  parser = optparse.OptionParser(usage=GLOBAL_HELP)
  DEFAULT_SDK_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

  parser.add_option(
      '-U', '--manifest-url', dest='manifest_url',
      default='https://commondatastorage.googleapis.com/nativeclient-mirror/'
              'nacl/nacl_sdk/%s' % MANIFEST_FILENAME,
      help='override the default URL for the NaCl manifest file')
  parser.add_option(
      '-d', '--debug', dest='debug',
      default=False, action='store_true',
      help='enable displaying debug information to stderr')
  parser.add_option(
      '-q', '--quiet', dest='quiet',
      default=False, action='store_true',
      help='suppress displaying informational prints to stdout')
  parser.add_option(
      '-u', '--user-data-dir', dest='user_data_dir',
      # TODO(mball): the default should probably be in something like
      # ~/.naclsdk (linux), or ~/Library/Application Support/NaClSDK (mac),
      # or %HOMEPATH%\Application Data\NaClSDK (i.e., %APPDATA% on windows)
      default=os.path.join(DEFAULT_SDK_ROOT, USER_DATA_DIR),
      help="specify location of NaCl SDK's data directory")
  parser.add_option(
      '-s', '--sdk-root-dir', dest='sdk_root_dir',
      default=DEFAULT_SDK_ROOT,
      help="location where the SDK bundles are installed")
  parser.add_option(
      '-v', '--version', dest='show_version',
      action='store_true',
      help='show version information and exit')
  parser.add_option(
      '-m', '--manifest', dest='manifest_filename',
      default=MANIFEST_FILENAME,
      help="name of local manifest file relative to user-data-dir")
  parser.add_option(
      '-c', '--config', dest='config_filename',
      default=CONFIG_FILENAME,
      help="name of the local config file relative to user-data-dir")


  COMMANDS = {
      'list': List,
      'update': Update,
      'install': Update,
      'sources': Sources,
      }

  # Separate global options from command-specific options
  global_argv = argv
  command_argv = []
  for index, arg in enumerate(argv):
    if arg in COMMANDS:
      global_argv = argv[:index]
      command_argv = argv[index:]
      break

  (options, args) = parser.parse_args(global_argv)
  args += command_argv

  global _debug_mode, _quiet_mode
  _debug_mode = options.debug
  _quiet_mode = options.quiet

  def PrintHelpAndExit(unused_options=None, unused_args=None):
    parser.print_help()
    exit(1)

  if options.show_version:
    print "Native Client SDK Updater, version %s.%s" % (MAJOR_REV, MINOR_REV)
    exit(0)

  if not args:
    print "Need to supply a command"
    PrintHelpAndExit()

  def DefaultHandler(unused_options=None, unused_args=None):
    print "Unknown Command: %s" % args[0]
    PrintHelpAndExit()

  def InvokeCommand(args):
    command = COMMANDS.get(args[0], DefaultHandler)
    # Load the config file before running commands
    config = LoadFromFile(os.path.join(options.user_data_dir,
                                       options.config_filename),
                          SDKConfig())
    command(options, args[1:], config)

  if args[0] == 'help':
    if len(args) == 1:
      PrintHelpAndExit()
    else:
      InvokeCommand([args[1], '-h'])
  else:
    # Make sure the user_data_dir exists.
    if not os.path.exists(options.user_data_dir):
      os.makedirs(options.user_data_dir)
    InvokeCommand(args)

  return 0  # Success


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as error:
    print "Error: %s" % error
    sys.exit(1)
