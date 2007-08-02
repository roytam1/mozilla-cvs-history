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
 * The Original Code is Lightning code.
 *
 * The Initial Developer of the Original Code is Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mike Shaver <shaver@mozilla.org>
 *   Stefan Sitter <ssitter@googlemail.com>
 *   Philipp Kewisch <mozilla@kewis.ch>
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

function eventToTodo(event)
{
    try {
        return event.originalTarget.selectedItem.todo;
    } catch (e) {
        return null;
    }
}

function editTodoItem(event)
{
    var todo = eventToTodo(event);
    if (todo)
        modifyEventWithDialog(todo);
}

function newTodoItem(event)
{
    createTodoWithDialog(ltnSelectedCalendar());
}

function deleteTodoItem(event)
{
    var todo = eventToTodo(event);
    if (todo)
        todo.calendar.deleteItem(todo, null);
}

function initializeTodoList()
{
    var todoList = document.getElementById("calendar-todo-list");
    todoList.calendar = getCompositeCalendar();
    todoList.addEventListener("todo-item-open", editTodoItem, false);
    todoList.addEventListener("todo-item-delete", deleteTodoItem, false);
    todoList.addEventListener("todo-empty-dblclick", newTodoItem, false);
    todoList.showCompleted = document.getElementById("completed-tasks-checkbox").checked;
    return;
}

function finishTodoList() {
    var todoList = document.getElementById("calendar-todo-list");
    todoList.removeEventListener("todo-item-open", editTodoItem, false);
    todoList.removeEventListener("todo-item-delete", deleteTodoItem, false);
    todoList.removeEventListener("todo-empty-dblclick", newTodoItem, false);
}

function toggleCompletedTasks()
{
    document.getElementById("calendar-todo-list").showCompleted =
        !document.getElementById("calendar-todo-list").showCompleted;

    agendaTreeView.refreshCalendarQuery();

    for each (view in getViewDeck().childNodes) {
        view.showCompleted = !view.showCompleted;
    }

    // Refresh the current view
    currentView().goToDay(currentView().selectedDay);

    return;
}

window.addEventListener("load", initializeTodoList, false);
window.addEventListener("unload", finishTodoList, false);
