function NewBrowserWindow() {}
function NewBlankPage() {} 
function TemplatePage() {}
function WizardPage() {}
function PageSetup() {}
function PrintPreview() {}
function Print() {}
function Close() {}
function Exit()
{
  dump("\nExit from XUL\n");
  var appCore = FindMsgAppCore();
  if (appCore != null) {
    dump("\nAppcore isn't null in Exit\n");
    appCore.SetWindow(window);
    appCore.exit();
  }
}

function CharacterSet(){}
function SetDocumentCharacterSet(aCharset)
{
  var appCore = FindMsgAppCore(); 
  if (appCore != null) {
    dump(aCharset);dump("\n");
  } else {
    dump("MsgAppCore has not been created!\n");
  }
}

function NavigatorWindow() {}
function MessengerWindow() {}
function ComposerWindow() {}
function AIMService() {}
function AddBookmark() {}
function FileBookmark() {}
function EditBookmark() {}
function Newsgroups() {}
function AddressBook() {}
function History() {}
function SecurityInfo() {}
function MessengerCenter() {}
function JavaConsole() {}
function PageService() {}
function MailAccount() {}
function MaillingList() {}
function FolderPermission() {}
function ManageNewsgroup() {}
function WindowList() {}
function Help() {}
function About() {}
