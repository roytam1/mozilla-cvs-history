/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is OEone Calendar Code, released October 31st, 2001.
 *
 * The Initial Developer of the Original Code is
 * OEone Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s): Garth Smedley <garths@oeone.com>
 *                 Mike Potter <mikep@oeone.com>
 *                 Chris Charabaruk <coldacid@meldstar.com>
 *                 Colin Phillips <colinp@oeone.com>
 *                 ArentJan Banck <ajbanck@planet.nl>
 *                 Curtis Jewell <csjewell@mail.freeshell.org>
 *                 Eric Belhaire <eric.belhaire@ief.u-psud.fr>
 *                 Mark Swaffer <swaff@fudo.org>
 *                 Michael Buettner <michael.buettner@sun.com>
 *                 Philipp Kewisch <mozilla@kewis.ch>
 *                 Berend Cornelius <berend.cornelius@sun.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

function addCalendarNames(aEvent) {
    var calendarMenuPopup = aEvent.target;
    var calendars = getCalendarManager().getCalendars({});
    while (calendarMenuPopup.hasChildNodes()) {
        calendarMenuPopup.removeChild(calendarMenuPopup.lastChild);
    }
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    var tasksSelected = (tasks.length > 0);
    if (tasksSelected) {
        var task = tasks[0];
        var isSame = isPropertyValueSame(tasks, "calendar");
        var selCalendarName = task.calendar.name;
        for (i in calendars) {
            var calendar = calendars[i];
            var calendarMenuItem = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuitem");
            calendarMenuItem.setAttribute("type", "checkbox");
            if (isSame && (selCalendarName.length > 0) && (calendar.name == selCalendarName)) {
                calendarMenuItem.setAttribute("checked", true);
            }
            calendarMenuItem.setAttribute("label", calendar.name);
            calendarMenuItem.setAttribute("oncommand", "contextChangeCalendar(event);");
            calendarMenuItem.calendar = calendar;
            calendarMenuPopup.appendChild(calendarMenuItem);
         }
    }
}

function addCategoryNames(aEvent) {
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    var tasksSelected = (tasks.length > 0);
    if (tasksSelected) {
        var index = appendCategoryItems(tasks[0], aEvent.target, document.getElementById("calendar_task_category_command"));
        aEvent.target.childNodes[index].setAttribute("checked","true");
    } else {
        appendCategoryItems(null, aEvent.target);
        applyAttributeToMenuChildren(aEvent.target, "disabled", (!tasksSelected));
    }
}

function changeTaskProgressMenu(aEvent) {
    changeMenuByPropertyName(aEvent, "percentComplete");
}

function changeTaskPriorityMenu(aEvent) {
    changeMenuByPropertyName(aEvent, "priority")
}

/** This highly specialized function checks a command which naming follows
 *  the notation 'calendar_' +  propertyname + ' + '-' + propertvalue + 'command',
 *  when its propertyvalue part matches the propertyvalue of the selected tasks
 *  as long as the selected tasks share common propertyValues.
 *  @param aEvent the event that contains a target from which the child elements
 *  are retrieved and unchecked.
 *  @param aPropertyName the name of the property that is available at a task
 */ 
function changeMenuByPropertyName(aEvent, aPropertyName) {
    uncheckChildNodes(aEvent);
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    var tasksSelected = ((tasks != null) && (tasks.length > 0));
    if (tasksSelected) {
        var task = tasks[0];
        if (isPropertyValueSame(tasks, aPropertyName)) {
            var command = document.getElementById("calendar_" + aPropertyName + "-" + task[aPropertyName] + "_command");
            if (command) {
                command.setAttribute("checked", "true");
            }
        }
    } else {
        applyAttributeToMenuChildren(aEvent.target, "disabled", (!tasksSelected));
    }
}

function changeContextMenuForTask(aEvent) {
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    var task = null;
    var tasksSelected = (tasks.length > 0);
    applyAttributeToMenuChildren(aEvent.target, "disabled", (!tasksSelected));
    document.getElementById("calendar_new_todo_command").removeAttribute("disabled");
    if (tasksSelected) {
        taskTree.contextTask = task = tasks[0];
        if (isPropertyValueSame(tasks, "isCompleted")) {;
            setBooleanAttribute(document.getElementById("calendar_iscompleted_command"), "checked", task.isCompleted);
        } else {
            document.getElementById("calendar_iscompleted_command").setAttribute("checked", false);
        }
    } else {
        taskTree.contextTask = null;
    }
}

function contextChangeTaskProgress2(aProgress) {
    contextChangeTaskProgress(aProgress);
    document.getElementById("calendar_percentComplete-100_command2").checked = false;
}

function contextChangeTaskProgress(aProgress) {
    startBatchTransaction();
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    for (var t = 0; t < tasks.length; t++) {
        var task = tasks[t];
        var newTask = task.clone().QueryInterface( Components.interfaces.calITodo );
        newTask.percentComplete = aProgress;
        switch (aProgress) {
            case 0:
                newTask.isCompleted = false;
                break;
            case 100:
                newTask.isCompleted = true;
                break;
            default:
                newTask.status = "IN-PROCESS";
                newTask.completedDate = null;
                break;
        }
        doTransaction('modify', newTask, newTask.calendar, task, null);
    }
    endBatchTransaction();
}

function contextChangeTaskCategory(aEvent) {
    startBatchTransaction();
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    var tasksSelected = (tasks.length > 0);
    if (tasksSelected) {
        var menuItem = aEvent.target;
        for (var t = 0; t < tasks.length; t++) {
            var newTask = tasks[t].clone().QueryInterface( Components.interfaces.calITodo );
            setCategory(newTask, menuItem);
            doTransaction('modify', newTask, newTask.calendar, tasks[t], null);
        }
    }
    endBatchTransaction();
}

function contextChangeCalendar(aEvent) {
   startBatchTransaction();
   var taskTree = getFocusedTaskTree();
   var tasks = taskTree.selectedTasks;
   for (var t = 0; t < tasks.length; t++) {
       var task = tasks[t];
       var newTask = task.clone().QueryInterface( Components.interfaces.calITodo );
       newTask.calendar = aEvent.target.calendar;
       doTransaction('modify', newTask, newTask.calendar, task, null);
    }
    endBatchTransaction();
}

function contextChangeTaskPriority(aPriority) {
    startBatchTransaction();
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    for (var t = 0; t < tasks.length; t++) {
        var task = tasks[t];
        var newTask = task.clone().QueryInterface( Components.interfaces.calITodo );
        newTask.priority = aPriority;
        doTransaction('modify', newTask, newTask.calendar, task, null);
     }
     endBatchTransaction();
  }

function modifyTaskFromContext() {
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    for (var t = 0; t < tasks.length; t++) {
        modifyEventWithDialog(tasks[t]);
    }
 }

/**
 *  Delete the current selected item with focus from the task tree
 */
function deleteToDoCommand(aDoNotConfirm) {
    var taskTree = getFocusedTaskTree();
    var selectedItems = taskTree.selectedTasks;
    calendarViewController.deleteOccurrences(selectedItems.length,
                                             selectedItems,
                                             false,
                                             aDoNotConfirm);
}

function getFocusedTaskTree() {
    // Which tree is focused depends on the mode.
    var taskTree;
    if (isSunbird() || !gCurrentMode || gCurrentMode == "mail") {
        taskTree = document.getElementById("unifinder-todo-tree");
    } else if (!isSunbird() && gCurrentMode == "task") {
        taskTree = document.getElementById("calendar-task-tree");
    }
    return taskTree;
}

function tasksToMail() {
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    calendarMailButtonDNDObserver.onDropItems(tasks);
}

function tasksToEvents() {
    var taskTree = getFocusedTaskTree();
    var tasks = taskTree.selectedTasks;
    calendarCalendarButtonDNDObserver.onDropItems(tasks);
}

function toggleCompleted(aEvent) {
    if (aEvent.target.getAttribute("checked") == "true") {
        contextChangeTaskProgress(0);
    } else {
        contextChangeTaskProgress(100);
    }
}
