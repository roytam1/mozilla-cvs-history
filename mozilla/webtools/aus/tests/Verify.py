"""
Python translation of fit..
which is copyright (c) 2002 Cunningham & Cunningham, Inc.
Released under the terms of the GNU General Public License version 2 or later.
"""

import feedparser # http://feedparser.org/
import urllib2
from urllib import pathname2url
from fit.ColumnFixture import ColumnFixture

auth_handler = urllib2.HTTPBasicAuthHandler()
auth_handler.add_password('khan!', 'khan.mozilla.org', 'morgamic', 'carebears')
opener = urllib2.build_opener(auth_handler)
urllib2.install_opener(opener)

class Verify(ColumnFixture):

    _typeDict={"host": "String",
               "updateVersion": "String",
               "product": "String",
               "version": "String",
               "build": "String",
               "platform": "String",
               "locale": "String",
               "channel": "String",
               "complete": "String",
               "partial": "String",
               "updateType": "String",
               "osVersion": "String",
               "dist": "String",
               "distVersion": "String",
               "force": "String",
               "newchannel": "String",
               "licenseUrl": "String",
               "lastURI": "String",
               "newURI": "String",
               "hasUpdate": "Boolean",
               "hasComplete": "Boolean",
               "hasPartial":  "Boolean",
               "hasLicenseUrl": "Boolean",
               "isValidXml": "Boolean",
               "isMinorUpdate": "Boolean",
               "isMajorUpdate": "Boolean",
               "hasForce": "Boolean"}

    def __init__(self):
        super(Verify, self).__init__()

        # Variables that are typically set in the FitNesse wiki params.
        self.updateVersion = ""
        self.host = ""
        self.product = ""
        self.version = ""
        self.platform = ""
        self.locale = ""
        self.osVersion = ""
        self.dist = ""
        self.distVersion = ""
        self.force = ""
        self.newchannel = ""

        # For storign the last retrieved AUS XML and its URI.
        self.lastURI = ""
        self.lastXML = ""

    # Checks if an update element exists.
    def hasUpdate(self):
        return ('</update>' in self.getXml())

    # Checks if the expected complete patch exists.
    def hasComplete(self):
        return (self.complete in self.getXml())

    # Check if the expected partial patch exists.
    def hasPartial(self):
        return (self.partial in self.getXml())

    # Check if the update type is "minor".
    def isMinorUpdate(self):
        return ('type="minor"' in self.getXml())

    # Check if the update type is "major".
    def isMajorUpdate(self):
        return ('type="major"' in self.getXml())

    # Check to see if the XML has a licenseURL.
    def hasLicenseUrl(self):
        return (self.licenseUrl in self.getXml())

    # Check to see if the XML has force set.
    def hasForce (self):
        return ('force=1' in self.getXml())

    # Check if the AUS XML document is well-formed.
    def isValidXml(self):
        
        # Parse our file, creating a feedparser object.
        xml = feedparser.parse(self.getXml())
        print xml.version

        # If xml.bozo==1, the feed was malformed and unparsable.
        if (xml.bozo==1):
            return False

        # We should always have an xml version.
        if ('<?xml version="1.0"?>' not in self.getXml()):
            return False
        
        # We should always have a parent 'updates' element.
        if ('<updates>' not in self.getXml()):
            return False
        
        return True

    # Gets and returns an AUS XML document.
    def getXml(self):
        newURI = self.buildUri();

        if (self.lastURI == newURI):
            return self.lastXML

        print newURI
        
        newXML = urllib2.urlopen(newURI).read()

        self.lastURI = newURI
        self.lastXML = newXML

        return newXML

    # Builds an AUS URI based on our test data.
    def buildUri(self):

        # Assign class variables from FitNesse arguments if they 
        # are not passed in from the row.
        if (not self.host): self.host = self.args[0]
        if (not self.updateVersion): self.updateVersion = self.args[1]
        if (not self.product): self.product = self.args[2]
        if (not self.version): self.version = self.args[3]
        if (not self.platform): self.platform = self.args[4]
        if (not self.locale): self.locale = self.args[5]
        if (not self.osVersion): self.osVersion = self.args[6]

        if (self.osVersion != "NULL"):
            url = '/'.join((self.host, 
                pathname2url('/'.join((self.updateVersion, self.product, self.version,
                            self.build, self.platform, self.locale,
                            self.channel, self.osVersion, self.dist, 
                            self.distVersion, "update.xml")))
                ))
        else:
            url = '/'.join((self.host, 
                pathname2url('/'.join((self.updateVersion, self.product, self.version,
                            self.build, self.platform, self.locale,
                            self.channel, "update.xml")))
                ))
        if (self.force == 'true'):
            url += '?force=1'

        if (self.newchannel != "NULL"):
            url += '?newchannel=' + self.newchannel

        return url
           

