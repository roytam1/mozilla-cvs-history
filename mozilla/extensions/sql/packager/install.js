// this function verifies disk space in kilobytes
function verifyDiskSpace(dirPath, spaceRequired)
{
  var spaceAvailable;

  // Get the available disk space on the given path
  spaceAvailable = fileGetDiskSpaceAvailable(dirPath);

  // Convert the available disk space into kilobytes
  spaceAvailable = parseInt(spaceAvailable / 1024);

  // do the verification
  if(spaceAvailable < spaceRequired)
  {
    logComment("Insufficient disk space: " + dirPath);
    logComment("  required : " + spaceRequired + " K");
    logComment("  available: " + spaceAvailable + " K");
    return(false);
  }

  return(true);
}

// this function deletes a file if it exists
function deleteThisFile(dirKey, file)
{
  var fFileToDelete;

  fFileToDelete = getFolder(dirKey, file);
  logComment("File to delete: " + fFileToDelete);
  if(File.isFile(fFileToDelete))
  {
    File.remove(fFileToDelete);
    return(true);
  }
  else
    return(false);
}

var srDest = 200;

var err = initInstall("SQL Support", "SQL", "0.1"); 
logComment("initInstall: " + err);

var fProgram = getFolder("Program");
logComment("fProgram: " + fProgram);

if (verifyDiskSpace(fProgram, srDest))
{
  err = addDirectory("Program", "0.1", "bin", fProgram, "", true);

  logComment("addDirectory() returned: " + err);

  var chromeFolder = getFolder("Chrome", "sql.jar");
  registerChrome(CONTENT | DELAYED_CHROME, chromeFolder, "content/sql/");
  registerChrome(LOCALE | DELAYED_CHROME, chromeFolder, "locale/en-US/sql/");

  err = getLastError();
  if (err == ACCESS_DENIED) {
    alert("Unable to write to program directory " + fProgram + ".\n You will need to restart the browser with administrator/root privileges to install this software. After installing as root (or administrator), you will need to restart the browser one more time to register the installed software.\n After the second restart, you can go back to running the browser without privileges!");
    cancelInstall(err);
    logComment("cancelInstall() due to error: " + err);
  }
  else if (err != SUCCESS) {
    cancelInstall(err);
    logComment("cancelInstall() due to error: " + err);
  }
  else {
    performInstall(); 
    logComment("performInstall() returned: " + err);
  }
}
else
  cancelInstall(INSUFFICIENT_DISK_SPACE);
