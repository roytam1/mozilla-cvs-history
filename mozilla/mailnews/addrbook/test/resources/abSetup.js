/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Sets up the directory service provider to return the app dir as the profile
 * directory for the address book to use for locating its files during the
 * tests.
 *
 * Note there are further configuration setup items below this.
 */

/**
 * General Configuration Data that applies to the address book.
 */

// Personal Address Book configuration items.
var kPABData =
{
  URI: "moz-abmdbdirectory://abook.mab",
  fileName: "abook.mab",
  dirName: "Personal Address Book",
  dirType: 2,
  dirPrefID: "ldap_2.servers.pab",
  operations: Components.interfaces.nsIAbDirectory.opRead |
              Components.interfaces.nsIAbDirectory.opWrite |
              Components.interfaces.nsIAbDirectory.opSearch,
  position: 1
};

// Collected Address Book configuration items.
var kCABData =
{
  URI: "moz-abmdbdirectory://history.mab",
  fileName: "history.mab",
  dirName: "Collected Addresses",
  dirType: 2,
  dirPrefID: "ldap_2.servers.history",
  operations: Components.interfaces.nsIAbDirectory.opRead |
              Components.interfaces.nsIAbDirectory.opWrite |
              Components.interfaces.nsIAbDirectory.opSearch,
  position: 2
};

// Mac OSX Address Book configurations items
var kOSXData =
{
  URI: "moz-abosxdirectory:///",
  fileName: "",
  dirName: "Mac OS X Address Book",
  dirType: 3,
  dirPrefID: "ldap_2.servers.osx",
  operations: Components.interfaces.nsIAbDirectory.opRead |
              Components.interfaces.nsIAbDirectory.opSearch,
  position: 1
};

// This currently applies to all address books of local type.
const kNormalPropertiesURI =
  "chrome://messenger/content/addressbook/abAddressBookNameDialog.xul";
