/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is The JavaScript Debugger
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

console.doCommandStep =
function con_step()
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }

    step();
    return true;
}

console.doCommandCont =
function con_step()
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }

    cont();
    return true;
}

console.onTModeMenuCreate =
function con_tmodecreate()
{
    var check;
    
    switch (getThrowMode())
    {
        case TMODE_IGNORE:
            check = "menu_TModeIgnore";
            break;
        case TMODE_TRACE:
            check = "menu_TModeTrace";
            break;
        case TMODE_BREAK:
            check = "menu_TModeBreak";
            break;
    }
    
    var menu = document.getElementById("menu_TModeIgnore");
    menu.setAttribute("checked", "menu_TModeIgnore" == check);

    menu = document.getElementById("menu_TModeTrace");
    menu.setAttribute("checked", "menu_TModeTrace" == check);

    menu = document.getElementById("menu_TModeBreak");
    menu.setAttribute("checked", "menu_TModeBreak" == check);
}

console.displayUsageError =
function con_dusage (command)
{
    display (command.name + " " + command.usage, MT_ERROR);
}

console.onDebugTrap =
function con_ondt ()
{
    var frame = getCurrentFrame();
    var url = frame.script.fileName;
    
    if (!console._sources[url])
    {
        function loaded ()
        {
            focusSource (url, frame.line);
        }
        loadSource (url, loaded);
    }
    else
        focusSource (url, frame.line);

    console._stackOutlinerView.setStack(console.frames);
    console._stackOutlinerView.setCurrentFrame (getCurrentFrameIndex());
    enableDebugCommands()
}

console.onDebugContinue =
function con_ondc ()
{
    console._stackOutlinerView.setStack(null);
    console._sourceOutlinerView.setCurrentLine(null);
}

console.onLoad =
function con_load (e)
{
    var ex;
    
    dd ("Application venkman, 'JavaScript Debugger' loaded.");

    try
    {
        init();
    }
    catch (ex)
    {
        window.alert (getMsg (MSN_ERR_STARTUP, formatException(ex)));
    }
    
}

console.onFrameChanged =
function con_fchanged (currentFrame, currentFrameIndex)
{
    if (currentFrame)
        console._stackOutlinerView.setCurrentFrame (currentFrameIndex);
    else
        console._stackOutlinerView.setStack(null);
}

console.onInputCommand =
function con_icommand (e)
{
    
    var ary = console._commands.list (e.command);
    
    switch (ary.length)
    {            
        case 0:
            display (getMsg(MSN_ERR_NO_COMMAND, e.command), MT_ERROR);
            break;
            
        case 1:
            if (typeof console[ary[0].func] == "undefined")        
                display (getMsg(MSN_ERR_NOTIMPLEMENTED, ary[0].name), MT_ERROR);
            else
            {
                e.commandEntry = ary[0];
                console[ary[0].func](e)
            }
            break;
            
        default:
            var str = "";
            for (var i in ary)
                str += str ? MSG_COMMASP + ary[i].name : ary[i].name;
            display (getMsg (MSN_ERR_AMBIGCOMMAND, [e.command, ary.length, str]),
                     MT_ERROR);
    }

}

console.onInputBreak =
function cli_ibreak(e)
{
    if (!e.inputData)
    {  /* if no input data, just list the breakpoints */
        if (console._breakpoints.length == 0)
        {
            display (MSG_NO_BREAKPOINTS_SET);
            return true;
        }

        display (getMsg(MSN_BP_HEADER, console._breakpoints.length));
        for (var i = 0; i < console._breakpoints.length; ++i)
        {
            var bp = console._breakpoints[i];
            display (getMsg(MSN_BP_LINE, [i, bp.fileName, bp.line, bp.length]));
        }
        return true;
    }
        
    var ary = e.inputData.match(/([^\s\:]+)\:?\s*(\d+)\s*$/);
    if (!ary)
    {
        console.displayUsageError(e.commandEntry);
        return false;
    }

    var matchingFiles = matchFileName (ary[1]);
    if (matchingFiles.length == 0)
    {
        display (getMsg(MSN_ERR_BP_NOSCRIPT, ary[1]), MT_ERROR);
        return false;
    }
    
    for (var i in matchingFiles)
    {
        var fileName = matchingFiles[i];
        if (getBreakpoint (fileName, ary[2]))
            display(MSN_BP_EXISTS, [fileName, ary[2]], MT_INFO);
        else
            setBreakpoint (fileName, ary[2]);
    }
    
    return true;
    
}

console.onInputClear =
function cli_iclear (e)
{
    var ary = e.inputData.match(/(\d+)|([^\s\:]+)\:?\s*(\d+)\s*$/);
    if (!ary)
    {
        console.displayUsageError(e.commandEntry);
        return false;
    }

    if (ary[1])
    {
        /* disable by breakpoint number */
        var idx = Number(ary[1]);
        return clearBreakpointByNumber (idx);
    }

    /* else disable breakpoint by filename pattern and line number */
    var matchingFiles = matchFileName (ary[2]);
    if (matchingFiles.length == 0)
    {
        display (getMsg(MSN_ERR_BP_NOSCRIPT, ary[2]), MT_ERROR);
        return false;
    }

    for (var i in matchingFiles)
        return clearBreakpoint (fileName, ary[3]);
    
    return true;
}

console.onInputCommands =
function cli_icommands (e)
{
    displayCommands (e.inputData);
    return true;
}

console.onInputCont =
function cli_icommands (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }
    cont();
}
    
console.onInputCompleteLine =
function con_icline (e)
{
    if (console._inputHistory[0] != e.line)
        console._inputHistory.unshift (e.line);
    
    if (console._inputHistory.length > console.prefs["input.history.max"])
        console._inputHistory.pop();

    console._lastHistoryReferenced = -1;
    console._incompleteLine = "";

    var ary = e.line.match (/(\S+) ?(.*)/);
    var command = ary[1];

    e.command = command;
    e.inputData =  ary[2] ? stringTrim(ary[2]) : "";
    e.line = e.line;
    console.onInputCommand (e);

}

console.onInputEval =
function con_ieval (e)
{
    if (e.inputData)
    {       
        display (e.inputData, MT_FEVAL_IN);
        var rv = evalInTargetScope (e.inputData);
        if (typeof rv != "undefined")
        {
            if (rv != null)
            {
                refreshResultsArray();
                var l = $.length;
                $[l] = rv;
                display (getMsg(MSN_FMT_TMP_ASSIGN, [l, formatValue (rv)]),
                         MT_FEVAL_OUT);
            }
            else
                display (formatValue (rv), MT_FEVAL_OUT);
        }
    }
    return true;
}

console.onInputEvalD =
function con_ievald (e)
{
    if (e.inputData)
    {       
        display (e.inputData, MT_EVAL_IN);
        var rv = evalInDebuggerScope (e.inputData)
        if (typeof rv != "undefined")
            display (String(rv), MT_EVAL_OUT);
    }
    return true;
}

console.onInputFBreak =
function cli_ifbreak(e)
{
    if (!e.inputData)
    {  /* if no input data, just list the breakpoints */
        if (console._futureBreaks.length == 0)
        {
            display (MSG_NO_FBREAKS_SET);
            return true;
        }

        display (getMsg(MSN_FBP_HEADER, console._futureBreaks.length));
        for (var i = 0; i < console._futureBreaks.length; ++i)
        {
            var bp = console._futureBreaks[i];
            display (getMsg(MSN_FBP_LINE, [i, bp.filePattern, bp.line]));
        }
        return true;
    }
        
    var ary = e.inputData.match(/([^\s\:]+)\:?\s*(\d+)\s*$/);
    if (!ary)
    {
        console.displayUsageError(e.commandEntry);
        return false;
    }

    var filePattern = ary[1];
    var line = ary[2];

    if (isFutureBreakpoint (filePattern, line))
        display(MSN_FBP_EXISTS, [filePattern, line], MT_INFO);
    else
    {
        setFutureBreakpoint (filePattern, line);
    }
    
    return true;
    
}

console.onInputFClear =
function cli_ifclear (e)
{
    var ary = e.inputData.match(/(\d+)|([^\s\:]+)\:?\s*(\d+)\s*$/);
    if (!ary)
    {
        console.displayUsageError(e.commandEntry);
        return false;
    }

    if (ary[1])
    {
        /* disable by breakpoint number */
        var idx = Number(ary[1]);
        if (!console._futureBreaks[idx])
        {
            display (getMsg(MSN_ERR_BP_NOINDEX, idx, MT_ERROR));
            return false;
        }
        clearFutureBreakpointByNumber(idx);
        return true;
    }

    clearFutureBreakpoint (ary[2], ary[3]);
    
    return true;
}

console.onInputFrame =
function con_iframe (e)
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }

    var idx = parseInt(e.inputData);    
    if (isNaN(idx))
        idx = getCurrentFrameIndex();

    setCurrentFrameByIndex(idx);
    
    displayFrame (console.frames[idx], idx, true);
    focusSource (console.frames[idx].script.fileName, console.frames[idx].line);
    
    return true;
}
            
console.onInputHelp =
function cli_ihelp (e)
{
    var ary = console._commands.list (e.inputData);
 
    if (ary.length == 0)
    {
        display (getMsg(MSN_ERR_NO_COMMAND, e.inputData), MT_ERROR);
        return false;
    }

    for (var i in ary)
    {        
        display (ary[i].name + " " + ary[i].usage, MT_USAGE);
        display (ary[i].help, MT_HELP);
    }

    return true;    
}

console.onInputProps =
function con_iprops (e, forceDebuggerScope)
{
    if (!e.inputData)
    {
        display (getMsg(MSN_ERR_REQUIRED_PARAM, MSG_VAL_EXPR));
        return false;
    }

    var v;
    
    if (forceDebuggerScope)
        v = evalInDebuggerScope (e.inputData);
    else
        v = evalInTargetScope (e.inputData);
    
    if (!(v instanceof jsdIValue) || v.jsType != jsdIValue.TYPE_OBJECT)
    {
        var str = (v instanceof jsdIValue) ? formatValue(v) : String(v)
        display (getMsg(MSN_ERR_INVALID_PARAM, [MSG_VAL_EXPR, str]),
                 MT_ERROR);
        return false;
    }
    
    display (getMsg(forceDebuggerScope ? MSN_PROPSD_HEADER : MSN_PROPS_HEADER,
                    e.inputData));
    displayProperties(v);
    return true;
}

console.onInputPropsD =
function con_ipropsd (e)
{
    return console.onInputProps (e, true);
}
            
console.onInputScope =
function con_iscope ()
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }
    
    if (getCurrentFrame().scope.propertyCount == 0)
        display (getMsg (MSN_NO_PROPERTIES, MSG_WORD_SCOPE + " 0"));
    else
        displayProperties (getCurrentFrame().scope);
    
    return true;
}

console.onInputStep =
function con_iwhere ()
{
    return console.doCommandStep();
}

console.onInputTMode =
function con_itmode (e)
{
    if (typeof e.inputData == "string")
    {
        if (e.inputData.search(/ignore/i) != -1)
        {
            setThrowMode(TMODE_IGNORE);
            return true;
        }
        else if (e.inputData.search(/trace/i) != -1)
        {
            setThrowMode(TMODE_TRACE);
            return true;
        }
        else if (e.inputData.search(/breaK/i) != -1)
        {
            setThrowMode(TMODE_BREAK);
            return true;
        }
    }

    /* display the current throw mode */
    setThrowMode(getThrowMode());
    return true;
}

console.onInputQuit =
function con_iquit ()
{
    window.close();
}

console.onInputWhere =
function con_iwhere ()
{
    if (!console.frames)
    {
        display (MSG_ERR_NO_STACK, MT_ERROR);
        return false;
    }
    
    displayCallStack();
    return true;
}

console.onSingleLineKeypress =
function con_slkeypress (e)
{
    switch (e.keyCode)
    {
        case 13:
            if (!e.target.value)
                return;
            
            dispatchCommand(e.target.value);
            e.target.value = "";
            break;

        case 38: /* up */
            if (console._lastHistoryReferenced <
                console._inputHistory.length - 1)
                e.target.value =
                    console._inputHistory[++console._lastHistoryReferenced];
            break;

        case 40: /* down */
            if (console._lastHistoryReferenced > 0)
                e.target.value =
                    console._inputHistory[--console._lastHistoryReferenced];
            else
            {
                console._lastHistoryReferenced = -1;
                e.target.value = console._incompleteLine;
            }
            
            break;
            
        case 33: /* pgup */
            w = window.frames[0];
            newOfs = w.pageYOffset - (w.innerHeight / 2);
            if (newOfs > 0)
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, 0);
            break;
            
        case 34: /* pgdn */
            w = window.frames[0];
            newOfs = w.pageYOffset + (w.innerHeight / 2);
            if (newOfs < (w.innerHeight + w.pageYOffset))
                w.scrollTo (w.pageXOffset, newOfs);
            else
                w.scrollTo (w.pageXOffset, (w.innerHeight + w.pageYOffset));
            break;

        case 9: /* tab */
            e.preventDefault();
            console.onTabCompleteRequest(e);
            break;       

        default:
            console._incompleteLine = e.target.value;
            break;
    }
}

console.onStackClick =
function con_stackclick (e)
{
    var target = e.originalTarget;
    
    if (target.localName == "outlinerbody")
    {
        var row = new Object();
        var colID = new Object();
        var childElt = new Object();
        
        var obo = console._stackOutliner.outlinerBoxObject;
        obo.getCellAt(e.clientX, e.clientY, row, colID, childElt);
        
        colID = colID.value;
        row = row.value;
        
        if (e.detail == 2 && console.frames)
        {
            setCurrentFrameByIndex(row);
            displayFrame (console.frames[row], row, true);
            focusSource (console.frames[row].script.fileName,
                         console.frames[row].line);
        }
    }
}

console.onScriptClick =
function con_stackclick (e)
{
    var target = e.originalTarget;
    
    if (target.localName == "outlinercol")
    {
        console._scriptsOutlinerView.toggleColumnMode();
        console._scriptsOutlinerView.setScripts(console._scripts);
    }
    else if (e.detail == 2 && target.localName == "outlinerbody")
    {
        var row = new Object();
        var colID = new Object();
        var childElt = new Object();
        
        var obo = console._scriptsOutliner.outlinerBoxObject;
        obo.getCellAt(e.clientX, e.clientY, row, colID, childElt);
        
        row = row.value;
        var fileName = console._scriptsOutlinerView.scripts[row].fileName;
        
        if (console._sources[fileName])
        {
            focusSource(fileName, 0);
        }
        else
        {
            function cb (data, url)
            {
                focusSource(url, 0);
            }
            loadSource (fileName, cb);
        }
    }

}

console.onSourceClick =
function con_sourceclick (e)
{
    var target = e.originalTarget;
    
    if (target.localName == "outlinerbody")
    {
        var row = new Object();
        var colID = new Object();
        var childElt = new Object();
        
        var obo = console._sourceOutliner.outlinerBoxObject;
        obo.getCellAt(e.clientX, e.clientY, row, colID, childElt);
        
        colID = colID.value;
        row = row.value;
        
        if (colID == "breakpoint-col")
        {
            var line = row + 1;
            var url = console._sourceOutlinerView.url;
            if (url)
            {
                if (getBreakpoint(url, line))
                {
                    clearBreakpoint (url, line);
                    if (isFutureBreakpoint (url, line))
                        clearFutureBreakpoint (url, line);
                }
                else if (isFutureBreakpoint (url, line))
                {
                    clearFutureBreakpoint (url, line);
                }
                else
                {
                    if (console._scripts[url])
                        setBreakpoint (url, line);
                    else
                        setFutureBreakpoint (url, line);
                }
                obo.invalidateRow(row);
            }
        }
    }

}

console.onTabCompleteRequest =
function con_tabcomplete (e)
{
    var selStart = e.target.selectionStart;
    var selEnd   = e.target.selectionEnd;            
    var v        = e.target.value;
    
    if (selStart != selEnd) 
    {
        /* text is highlighted, just move caret to end and exit */
        e.target.selectionStart = e.target.selectionEnd = v.length;
        return;
    }

    var firstSpace = v.indexOf(" ");
    if (firstSpace == -1)
        firstSpace = v.length;

    var pfx;
    var d;
    
    if ((selStart <= firstSpace))
    {
        /* The cursor is positioned before the first space, so we're completing
         * a command
         */
        var partialCommand = v.substring(0, firstSpace).toLowerCase();
        var cmds = console._commands.listNames(partialCommand);

        if (!cmds)
            /* partial didn't match a thing */
            display (getMsg(MSN_NO_CMDMATCH, partialCommand), MT_ERROR);
        else if (cmds.length == 1)
        {
            /* partial matched exactly one command */
            pfx = cmds[0];
            if (firstSpace == v.length)
                v =  pfx + " ";
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length + 1;
        }
        else if (cmds.length > 1)
        {
            /* partial matched more than one command */
            d = new Date();
            if ((d - console._lastTabUp) <= console.prefs["input.dtab.time"])
                display (getMsg (MSN_CMDMATCH,
                                 [partialCommand, "[" + cmds.join(MSG_COMMASP) +
                                  "]"]));
            else
                console._lastTabUp = d;
            
            pfx = getCommonPfx(cmds);
            if (firstSpace == v.length)
                v =  pfx;
            else
                v = pfx + v.substr (firstSpace);
            
            e.target.value = v;
            e.target.selectionStart = e.target.selectionEnd = pfx.length;
            
        }
                
    }

}

console.onUnload =
function con_unload (e)
{
    dd ("Application venkman, 'JavaScript Debugger' unloading.");
    
    detachDebugger();
    
    return true;
}

window.onresize =
function ()
{
    console.scrollDown();
}

