var editorShell;
var misspelledWord;

// dialog initialization code
function Startup()
{
  dump("Doing Startup...\n");

  // get the editor shell from the parent window
  editorShell = window.opener.editorShell;
  editorShell = editorShell.QueryInterface(Components.interfaces.nsISpellCheck);
  if(!editorShell) {
    dump("EditoreditorShell not found!!!\n");
    window.close();
  }
  dump("EditoreditorShell found for Spell Checker dialog\n");

  // Create dialog object to store controls for easy access
  dialog = new Object;
  if (!dialog)
  {
    dump("Failed to create dialog object!!!\n");
    window.close();
  }

  dialog.wordInput = document.getElementById("Word");
  dialog.suggestedList = document.getElementById("SuggestedList");
  dialog.languageList = document.getElementById("");
  if (!dialog.wordInput ||
      !dialog.suggestedList  ||
      !dialog.languageList )
  {
    dump("Not all dialog controls were found!!!\n");
  }
  // NOTE: We shouldn't have been created if there was no misspelled word
  
  // The first misspelled word is passed as the 2nd extra parameter in window.openDialog()
  misspelledWord = window.arguments[1];
  
  if (misspelledWord != "") {
    dump("First misspelled word = "+misspelledWord+"\n");
    // Put word in input field
    dialog.wordInput.value = misspelledWord;
    // Get the list of suggested replacements
    FillSuggestedList();
  } else {
    dump("No misspelled word found\n");
  }

  dialog.suggestedList.focus();  
}

function NextWord()
{
  misspelledWord = editorShell.GetNextMisspelledWord();
  dialog.wordInput.value = misspelledWord;
  if (misspelledWord == "") {
    dump("FINISHED SPELL CHECKING\n");
    FillSuggestedList("(no suggestions)");
  } else {
    FillSuggestedList();
  }
}

function CheckWord()
{
  dump("SpellCheck: CheckWord\n");
  word = dialog.wordInput.value;
  if (word != "") {
    dump("CheckWord: Word in edit field="+word+"\n");
    isMisspelled = editorShell.CheckCurrentWord(word);
    if (isMisspelled) {
      dump("CheckWord says word was misspelled\n");
      misspelledWord = word;
      FillSuggestedList();
    } else {
      ClearList(dialog.suggestedList);
      AppendStringToList(dialog.suggestedList, "(correct spelling)");
    }
  }
}

function SelectSuggestedWord()
{
  dump("SpellCheck: SelectSuggestedWord\n");
  dialog.wordInput.value = dialog.suggestedList.options[dialog.suggestedList.selectedIndex].value;
}

function Ignore()
{
  dump("SpellCheck: Ignore\n");
  NextWord();
}

function IgnoreAll()
{
  dump("SpellCheck: IgnoreAll\n");
  if (misspelledWord != "") {
    editorShell.IgnoreWordAllOccurrences(misspelledWord);
  }
  NextWord();
}

function Replace()
{
  dump("SpellCheck: Replace\n");
  newWord = dialog.wordInput.value;
  if (misspelledWord != "" && misspelledWord != newWord) {
    isMisspelled = editorShell.ReplaceWord(misspelledWord, newWord, false);
  }
  NextWord();
}

function ReplaceAll()
{
  dump("SpellCheck: ReplaceAll\n");
  newWord = dialog.wordInput.value;
  if (misspelledWord != "" && misspelledWord != newWord) {
    isMisspelled = editorShell.ReplaceWord(misspelledWord, newWord, true);
  }
  NextWord();
}

function AddToDictionary()
{
  dump("SpellCheck: AddToDictionary\n");
  if (misspelledWord != "") {
    editorShell.AddWordToDictionary(misspelledWord);
  }
}

function EditDictionary()
{
  dump("SpellCheck: EditDictionary TODO: IMPLEMENT DIALOG\n");
  window.width = 700;
  window.height = 700;
}

function SelectLanguage()
{
  // A bug in combobox prevents this from working
  dump("SpellCheck: SelectLanguage.\n");
}

function Close()
{
  dump("SpellCheck: Spell Checker Closed\n");
  // Shutdown the spell check and close the dialog
  editorShell.CloseSpellChecking();
  window.close();
}

function FillSuggestedList(firstWord)
{
  list = dialog.suggestedList;

  // Clear the current contents of the list
  ClearList(list);

  // We may have the initial word
  if (firstWord && firstWord != "") {
    dump("First Word = "+firstWord+"\n");
    AppendStringToList(list, firstWord);
  }

  // Get suggested words until an empty string is returned
  do {
    word = editorShell.GetSuggestedWord();
    dump("Suggested Word = "+word+"\n");
    if (word != "") {
      AppendStringToList(list, word);
    }
  } while (word != "");
}

