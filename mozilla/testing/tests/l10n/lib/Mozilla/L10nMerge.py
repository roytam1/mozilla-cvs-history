# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is l10n test automation.
#
# The Initial Developer of the Original Code is
# Mozilla Foundation
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#	Axel Hecht <l10n@mozilla.com>
#	Toshihiro Kura
#	Tomoya Asai (dynamis)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

'Mozilla l10n merge tool'

import os
import re
import codecs
import logging
import time
import shutil
import Parser
import Paths

options = {}

class FileCollector:
  class Iter:
    def __init__(self, path):
      self.__base = path
    def __iter__(self):
      self.__w = os.walk(self.__base)
      try:
        self.__nextDir()
      except StopIteration:
        # empty dir, bad, but happens
        self.__i = [].__iter__()
      return self
    def __nextDir(self):
      self.__t = self.__w.next()
      try:
        self.__t[1].remove("CVS")
      except ValueError:
        pass
      self.__t[1].sort()
      self.__t[2].sort()
      self.__i = self.__t[2].__iter__()
    def next(self):
      try:
        leaf = self.__i.next()
        path = self.__t[0] + '/' + leaf
        key = path[len(self.__base) + 1:]
        return (key, path)
      except StopIteration:
        self.__nextDir()
        return self.next()
      print "not expected"
      raise StopIteration
  def __init__(self):
    pass
  def getFiles(self, mod, locale):
    fls = {}
    for leaf, path in self.iterateFiles(mod,locale):
      fls[leaf] = path
    return fls
  def iterateFiles(self, mod, locale):
    return FileCollector.Iter(Paths.get_base_path(mod, locale))

def collectFiles(aComparer, apps = None, locales = None):
  '''
  returns new files, files to compare, files to remove
  apps or locales need to be given, apps is a list, locales is a
  hash mapping applications to languages.
  If apps is given, it will look up all-locales for all apps for the
  languages to test.
  'toolkit' is added to the list of modules, too.
  '''
  if not apps and not locales:
    raise RuntimeError, "collectFiles needs either apps or locales"
  if apps and locales:
    raise RuntimeError, "You don't want to give both apps or locales"
  if locales:
    apps = locales.keys()
    # add toolkit, with all of the languages of all apps
    all = set()
    for locs in locales.values():
      all.update(locs)
    locales['toolkit'] = list(all)
  else:
    locales = Paths.allLocales(apps)
  modules = Paths.Modules(apps)
  en = FileCollector()
  l10n = FileCollector()
  # load filter functions for each app
  fltrs = []
  for app in apps:
    filterpath = 'mozilla/%s/locales/filter.py' % app
    if not os.path.exists(filterpath):
      continue
    l = {}
    execfile(filterpath, {}, l)
    if 'test' not in l or not callable(l['test']):
      logging.debug('%s does not define function "test"' % filterpath)
      continue
    fltrs.append(l['test'])
  
  # define fltr function to be used, calling into the app specific ones
  # if one of our apps wants us to know about a triple, make it so
  def fltr(mod, lpath, entity = None):
    for f in fltrs:
      keep  = True
      try:
        keep = f(mod, lpath, entity)
      except Exception, e:
        logging.error(str(e))
      if not keep:
        return False
    return True
  
  for cat in modules.keys():
    logging.debug(" testing " + cat+ " on " + str(modules))
    aComparer.notifyLocales(cat, locales[cat])
    for mod in modules[cat]:
      en_fls = en.getFiles(mod, 'en-US')
      for loc in locales[cat]:
        fls = dict(en_fls) # create copy for modification
        for l_fl, l_path in l10n.iterateFiles(mod, loc):
          if fls.has_key(l_fl):
            # file in both en-US and locale, compare
            aComparer.compareFile(mod, loc, l_fl)
            del fls[l_fl]
          else:
            if fltr(mod, l_fl):
              # file in locale, but not in en-US, remove
              aComparer.removeFile(mod, loc, l_fl)
            else:
              logging.debug(" ignoring %s from %s in %s" %
                            (l_fl, loc, mod))
        # all locale files dealt with, remaining fls need to be added?
        for lf in fls.keys():
          if fltr(mod, lf):
            aComparer.addFile(mod,loc,lf)
          else:
            logging.debug(" ignoring %s from %s in %s" %
                          (lf, loc, mod))

  return fltr

class CompareCollector:
  'collects files to be compared, added, removed'
  def __init__(self):
    self.cl = {}
    self.files = {}
    self.modules = {}
  def notifyLocales(self, aModule, aLocaleList):
    for loc in aLocaleList:
      if self.modules.has_key(loc):
        self.modules[loc].append(aModule)
      else:
        self.modules[loc] = [aModule]
  def addFile(self, aModule, aLocale, aLeaf):
    logging.debug(" copying %s for %s in %s" % (aLeaf, aLocale, aModule))
    if not self.files.has_key(aLocale):
      self.files[aLocale] = {'missingFiles': [(aModule, aLeaf)],
                             'obsoleteFiles': []}
    else:
      self.files[aLocale]['missingFiles'].append((aModule, aLeaf))
    l_fl = Paths.get_path(aModule, aLocale, aLeaf)
    if not os.path.isdir(os.path.dirname(l_fl)):
      os.mkdir(os.path.dirname(l_fl))
    shutil.copy2(Paths.get_path(aModule, 'en-US', aLeaf), l_fl)
    pass
  def compareFile(self, aModule, aLocale, aLeaf):
    if not self.cl.has_key((aModule, aLeaf)):
      self.cl[(aModule, aLeaf)] = [aLocale]
    else:
      self.cl[(aModule, aLeaf)].append(aLocale)
    pass
  def removeFile(self, aModule, aLocale, aLeaf):
    logging.debug(" remove %s from %s in %s" % (aLeaf, aLocale, aModule))
    if not self.files.has_key(aLocale):
      self.files[aLocale] = {'obsoleteFiles': [(aModule, aLeaf)],
                             'missingFiles':[]}
    else:
      self.files[aLocale]['obsoleteFiles'].append((aModule, aLeaf))
    if options['backup']:
      os.rename(Paths.get_path(aModule, aLocale, aLeaf), Paths.get_path(aModule, aLocale, aLeaf + '~'))
    else:
      os.remove(Paths.get_path(aModule, aLocale, aLeaf))
    pass

def merge(apps=None, testLocales=[]):
  result = {}
  c = CompareCollector()
  fltr = collectFiles(c, apps=apps, locales=testLocales)
  
  key = re.compile('[kK]ey')
  for fl, locales in c.cl.iteritems():
    (mod,path) = fl
    logging.info(" Handling " + path + " in " + mod)
    try:
      parser = Parser.getParser(path)
    except UserWarning:
      logging.warning(" Can't merge " + path + " in " + mod)
      continue
    parser.readFile(Paths.get_path(mod, 'en-US', path))
    logging.debug(" Parsing en-US " + path + " in " + mod)
    (enList, enMap) = parser.parse()
    for loc in locales:
      if not result.has_key(loc):
        result[loc] = {'missing':[],'obsolete':[],
                       'changed':0,'unchanged':0,'keys':0}
      enTmp = dict(enMap)
      parser.readFile(Paths.get_path(mod, loc, path))
      logging.debug(" Parsing " + loc + " " + path + " in " + mod)
      (l10nList, l10nMap) = parser.parse()
      l10nTmp = dict(l10nMap)
      logging.debug(" Checking existing entities of " + path + " in " + mod)
      for k,i in l10nMap.items():
        if not fltr(mod, path, k):
          if enTmp.has_key(k):
            del enTmp[k]
            del l10nTmp[k]
          continue
        if not enTmp.has_key(k):
          result[loc]['obsolete'].append((mod,path,k))
          continue
        enVal = enList[enTmp[k]]['val']
        del enTmp[k]
        del l10nTmp[k]
        if key.search(k):
          result[loc]['keys'] += 1
        else:
          if enVal == l10nList[i]['val']:
            result[loc]['unchanged'] +=1
            logging.info('%s in %s unchanged' %
                         (k, Paths.get_path(mod, loc, path)))
          else:
            result[loc]['changed'] +=1
      result[loc]['missing'].extend(filter(lambda t: fltr(*t),
                                           [(mod,path,k) for k in enTmp.keys()]))
      filename = Paths.get_path(mod, loc, path)
      # comment out obsolete entities
      if l10nTmp != {}:
        logging.info(" Commenting out obsolete entities...")
        f = codecs.open(filename, 'w', parser.encoding)
        daytime = time.asctime()
        try:
          f.write(parser.header)
          if re.search('\\.dtd', filename):
            for entity in l10nList:
              if l10nTmp.has_key(entity['key']):
                if not options['cleanobsolete']:
                  f.write(entity['prespace'] + '<!-- XXX l10n merge: obsolete entity (' + daytime + ') -->\n' + entity['precomment'] + '<!-- ' + entity['def'] + ' -->' + entity['post'])
              else:
                f.write(entity['all'])
          elif re.search('\\.(properties|inc)', filename):
            for entity in l10nList:
              if l10nTmp.has_key(entity['key']):
                if not options['cleanobsolete']:
                  f.write(entity['prespace'] + '# XXX l10n merge: obsolete entity (' + daytime + ')\n' + entity['precomment'] + '#' + entity['def'] + entity['post'])
              else:
                f.write(entity['all'])
          f.write(parser.footer)
        except UnicodeDecodeError, e:
          logging.getLogger('locales').error("Can't write file: " + file + ';' + str(e))
        f.close()
      # add new entities
      if enTmp != {}:
        logging.info(" Adding new entities...")
        f = codecs.open(filename, 'a', parser.encoding)
        daytime = time.asctime()
        try:
          if re.search('\\.dtd', filename):
            f.write('\n<!-- XXX l10n merge: new entities (' + daytime + ') -->\n')
            v = enTmp.values()
            v.sort()
            for i in v:
              f.write(enList[i]['all'])
          elif re.search('\\.(properties|inc)', filename):
            f.write('\n# XXX l10n merge: new entities (' + daytime + ')\n')
            v = enTmp.values()
            v.sort()
            for i in v:
              f.write(enList[i]['all'])
        except UnicodeDecodeError, e:
          logging.getLogger('locales').error("Can't write file: " + file + ';' + str(e))
        f.close()
  for loc,dics in c.files.iteritems():
    if not result.has_key(loc):
      result[loc] = dics
    else:
      for key, list in dics.iteritems():
        result[loc][key] = list
  for loc, mods in c.modules.iteritems():
    result[loc]['tested'] = mods
  return result
