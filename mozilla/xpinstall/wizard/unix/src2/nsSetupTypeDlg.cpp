/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, 
 * released March 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications 
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Samir Gehani <sgehani@netscape.com>
 */

#include "nsSetupTypeDlg.h"
#include "nsXInstaller.h"

static GtkWidget        *sBrowseBtn;
static gint             sBrowseBtnID;
static GtkWidget        *sFolder;
static GSList           *sGroup;
static GtkWidget        *sCreateDestDlg;

nsSetupTypeDlg::nsSetupTypeDlg() :
    mMsg0(NULL),
    mSetupTypeList(NULL)
{
}

nsSetupTypeDlg::~nsSetupTypeDlg()
{
    FreeSetupTypeList();

    if (mMsg0)
        free (mMsg0);
}

void
nsSetupTypeDlg::Back(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Back");
    if (aData != gCtx->sdlg) return;
    if (gCtx->bMoving)
    {
        gCtx->bMoving = FALSE;
        return;
    }

    // hide this notebook page
    gCtx->sdlg->Hide(nsXInstallerDlg::BACKWARD_MOVE);

    // disconnect this dlg's nav btn signal handlers
    gtk_signal_disconnect(GTK_OBJECT(gCtx->back), gCtx->backID);
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);
    gtk_signal_disconnect(GTK_OBJECT(sBrowseBtn), sBrowseBtnID);

    // show the next dlg
    gCtx->ldlg->Show(nsXInstallerDlg::BACKWARD_MOVE);
    gCtx->bMoving = TRUE;
}

void 
nsSetupTypeDlg::Next(GtkWidget *aWidget, gpointer aData)
{
    DUMP("Next");
    if (aData != gCtx->sdlg) return;
    if (gCtx->bMoving)
    {
        gCtx->bMoving = FALSE;
        return;
    }

    // verify selected destination directory exists
    if (OK != nsSetupTypeDlg::VerifyDestination())
        return;

    // hide this notebook page
    gCtx->sdlg->Hide(nsXInstallerDlg::FORWARD_MOVE);

    // disconnect this dlg's nav btn signal handlers
    gtk_signal_disconnect(GTK_OBJECT(gCtx->back), gCtx->backID);
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);
    gtk_signal_disconnect(GTK_OBJECT(sBrowseBtn), sBrowseBtnID);

    // show the last dlg
    if (gCtx->opt->mSetupType == (gCtx->sdlg->GetNumSetupTypes() - 1))
        gCtx->cdlg->Show(nsXInstallerDlg::FORWARD_MOVE);
    else
        gCtx->idlg->Show(nsXInstallerDlg::FORWARD_MOVE);
    gCtx->bMoving = TRUE;
}

int
nsSetupTypeDlg::Parse(nsINIParser *aParser)
{
    int err = OK;
    int bufsize = 0;
    char *showDlg = NULL;
    int i, j;
    char *currSec = (char *) malloc(strlen(SETUP_TYPE) + 3); // e.g. SetupType12
    if (!currSec) return E_MEM;
    char *currKey = (char *) malloc(1 + 3); // e.g. C0, C1, C12
    if (!currKey) return E_MEM;
    char *currVal = NULL;
    nsComponent *currComp = NULL;
    nsComponent *currCompDup = NULL;
    int currIndex;
    int currNumComps = 0;

    nsComponentList *compList = gCtx->cdlg->GetCompList();
    DUMP("Pre-verification of comp list")
    XI_VERIFY(compList);
    DUMP("Post-verification of comp list")

    nsSetupType *currST = NULL;
    char *currDescShort = NULL;
    char *currDescLong = NULL;

    XI_VERIFY(gCtx);

    /* optional keys */
    err = aParser->GetStringAlloc(DLG_SETUP_TYPE, MSG0, &mMsg0, &bufsize);
    if (err != OK && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;

    bufsize = 5;
    err = aParser->GetStringAlloc(DLG_SETUP_TYPE, SHOW_DLG, &showDlg, &bufsize);
    if (err != OK && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;
    if (bufsize != 0 && showDlg)
    {
        if (0 == strncmp(showDlg, "TRUE", 4))
            mShowDlg = nsXInstallerDlg::SHOW_DIALOG;
        else if (0 == strncmp(showDlg, "FALSE", 5))
            mShowDlg = nsXInstallerDlg::SKIP_DIALOG;
    }

    bufsize = 0;
    err = aParser->GetStringAlloc(DLG_SETUP_TYPE, TITLE, &mTitle, &bufsize);
    if (err != OK && err != nsINIParser::E_NO_KEY) goto BAIL; else err = OK;
    if (bufsize == 0)
            XI_IF_FREE(mTitle); 

    /* setup types */
    for (i=0; i<MAX_SETUP_TYPES; i++)
    {
        sprintf(currSec, SETUP_TYPEd, i);

        bufsize = 0;
        err = aParser->GetStringAlloc(currSec, DESC_SHORT, &currDescShort,
                                      &bufsize);
        if (err != OK && err != nsINIParser::E_NO_SEC) goto fin_iter;
        if (bufsize == 0 || err == nsINIParser::E_NO_SEC) // no more setup types
        {
            err = OK;
            break;
        }

        bufsize = 0;
        err = aParser->GetStringAlloc(currSec, DESC_LONG, &currDescLong,
                                      &bufsize);
        if (err != OK || bufsize == 0) goto fin_iter;

        currST = new nsSetupType();
        if (!currST) goto fin_iter;

        currST->SetDescShort(currDescShort);
        currST->SetDescLong(currDescLong);

        currNumComps = 0;
        for (j=0; j<MAX_COMPONENTS; j++)
        {
            sprintf(currKey, Cd, j);
            
            bufsize = 0;
            err = aParser->GetStringAlloc(currSec, currKey, &currVal, 
                                          &bufsize);
            if (err != OK && err != nsINIParser::E_NO_KEY) continue;
            if (bufsize == 0 || err == nsINIParser::E_NO_KEY) 
            {
                err = OK;
                break;
            }
        
            currComp = NULL;
            currIndex = atoi(currVal + strlen(COMPONENT));
            currComp = compList->GetCompByIndex(currIndex);
            if (currComp)
            {
                // preserve next ptr
                currCompDup = currComp->Duplicate(); 
                currST->SetComponent(currCompDup);
                currNumComps++;
            }
        }
        if (currNumComps > 0)
        {
            AddSetupType(currST);
            currST = NULL;
        }

fin_iter:
        XI_IF_DELETE(currST);
    }

    XI_IF_FREE(currSec);
    XI_IF_FREE(currKey);

    return err;

BAIL:
    return err;
}

int
nsSetupTypeDlg::Show(int aDirection)
{
    int err = OK;
    int numSetupTypes = 0;
    int i;
    GtkWidget *stTable = NULL;
    GtkWidget *radbtns[MAX_SETUP_TYPES];
    GtkWidget *desc[MAX_SETUP_TYPES];
    nsSetupType *currST = NULL;
    GtkWidget *destTable = NULL;
    GtkWidget *frame = NULL;
    GtkWidget *hbox = NULL;

    XI_VERIFY(gCtx);
    XI_VERIFY(gCtx->notebook);

    if (mWidgetsInit == FALSE)
    {
        // create a new table and add it as a page of the notebook
        mTable = gtk_table_new(4, 1, FALSE);
        gtk_notebook_append_page(GTK_NOTEBOOK(gCtx->notebook), mTable, NULL);
        mPageNum = gtk_notebook_get_current_page(GTK_NOTEBOOK(gCtx->notebook));
        gtk_widget_show(mTable);

        // insert a static text widget in the first row
        GtkWidget *msg0 = gtk_label_new(mMsg0);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), msg0, FALSE, FALSE, 0);
        gtk_widget_show(hbox);
        gtk_table_attach(GTK_TABLE(mTable), hbox, 0, 1, 1, 2,
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
			GTK_FILL, 20, 20);
        gtk_widget_show(msg0);

        // insert a [n x 2] heterogeneous table in the second row
        // where n = numSetupTypes
        numSetupTypes = GetNumSetupTypes();
        stTable = gtk_table_new(numSetupTypes, 4, FALSE);
        gtk_widget_show(stTable);
        gtk_table_attach(GTK_TABLE(mTable), stTable, 0, 1, 2, 3,
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
			20, 0);

        currST = GetSetupTypeList();
        if (!currST) return E_NO_SETUPTYPES;

        // first radio button
        gCtx->opt->mSetupType = 0;
        radbtns[0] = gtk_radio_button_new_with_label(NULL,
                        currST->GetDescShort());
        sGroup = gtk_radio_button_group(GTK_RADIO_BUTTON(radbtns[0]));
        gtk_table_attach(GTK_TABLE(stTable), radbtns[0], 0, 1, 0, 1,
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
            static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
			0, 0);
        gtk_signal_connect(GTK_OBJECT(radbtns[0]), "toggled",
                           GTK_SIGNAL_FUNC(RadBtnToggled), 0);
        gtk_widget_show(radbtns[0]);

        desc[0] = gtk_label_new(currST->GetDescLong());
        gtk_label_set_justify(GTK_LABEL(desc[0]), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap(GTK_LABEL(desc[0]), TRUE);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), desc[0], FALSE, FALSE, 0);
        gtk_widget_show(hbox);
        gtk_table_attach_defaults(GTK_TABLE(stTable), hbox, 1, 2, 0, 1);
        gtk_widget_show(desc[0]);

        // remaining radio buttons
        for (i = 1; i < numSetupTypes; i++)
        {
            currST = currST->GetNext();
            if (!currST) break;

            radbtns[i] = gtk_radio_button_new_with_label(sGroup,
                            currST->GetDescShort());
            sGroup = gtk_radio_button_group(GTK_RADIO_BUTTON(radbtns[i]));
            gtk_table_attach(GTK_TABLE(stTable), radbtns[i], 0, 1, i, i+1,
                static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
                static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND), 0, 0);
            gtk_signal_connect(GTK_OBJECT(radbtns[i]), "toggled",
                               GTK_SIGNAL_FUNC(RadBtnToggled),
			                   reinterpret_cast<void *>(i));
            gtk_widget_show(radbtns[i]);

            desc[i] = gtk_label_new(currST->GetDescLong());
            gtk_label_set_justify(GTK_LABEL(desc[i]), GTK_JUSTIFY_LEFT);
            gtk_label_set_line_wrap(GTK_LABEL(desc[i]), TRUE);
            hbox = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), desc[i], FALSE, FALSE, 0);
            gtk_widget_show(hbox);
            gtk_table_attach_defaults(GTK_TABLE(stTable), hbox, 1, 2, i, i+1);
            gtk_widget_show(desc[i]);
        }

        // insert a [1 x 2] heterogeneous table in the third row
        destTable = gtk_table_new(1, 2, FALSE);
        gtk_widget_show(destTable); 

        gtk_table_attach(GTK_TABLE(mTable), destTable, 0, 1, 3, 4,
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
			20, 20);
        frame = gtk_frame_new(DEST_DIR);
        gtk_table_attach_defaults(GTK_TABLE(destTable), frame, 0, 2, 0, 1);
        gtk_widget_show(frame);

        if (!gCtx->opt->mDestination)
        {
            gCtx->opt->mDestination = (char *) malloc(1024 * sizeof(char));
            getcwd(gCtx->opt->mDestination, 1024);
        }
        sFolder = gtk_label_new(gCtx->opt->mDestination);
        gtk_label_set_line_wrap(GTK_LABEL(sFolder), TRUE);
        gtk_widget_show(sFolder);
        gtk_table_attach_defaults(GTK_TABLE(destTable), sFolder, 0, 1, 0, 1);

        sBrowseBtn = gtk_button_new_with_label(BROWSE);
        gtk_widget_show(sBrowseBtn);
        gtk_table_attach(GTK_TABLE(destTable), sBrowseBtn, 1, 2, 0, 1,
            static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
			GTK_SHRINK, 10, 0);

        mWidgetsInit = TRUE;
    }
    else
    {
        gtk_notebook_set_page(GTK_NOTEBOOK(gCtx->notebook), mPageNum);
        gtk_widget_show(mTable);
    }

    // signal connect the buttons
    // NOTE: back button disfunctional in this dlg since user accepted license
    gCtx->backID = gtk_signal_connect(GTK_OBJECT(gCtx->back), "clicked",
                   GTK_SIGNAL_FUNC(nsSetupTypeDlg::Back), gCtx->sdlg);
    gCtx->nextID = gtk_signal_connect(GTK_OBJECT(gCtx->next), "clicked",
                   GTK_SIGNAL_FUNC(nsSetupTypeDlg::Next), gCtx->sdlg);
    sBrowseBtnID = gtk_signal_connect(GTK_OBJECT(sBrowseBtn), "clicked",
                   GTK_SIGNAL_FUNC(nsSetupTypeDlg::SelectFolder), NULL);  

    if (aDirection == nsXInstallerDlg::FORWARD_MOVE)
    {
        // change the button titles back to Back/Next
        gtk_container_remove(GTK_CONTAINER(gCtx->next), gCtx->acceptLabel);
        gtk_container_remove(GTK_CONTAINER(gCtx->back), gCtx->declineLabel);
        gCtx->nextLabel = gtk_label_new(NEXT);
        gCtx->backLabel = gtk_label_new(BACK);
        gtk_widget_show(gCtx->nextLabel);
        gtk_widget_show(gCtx->backLabel);
        gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
        gtk_container_add(GTK_CONTAINER(gCtx->back), gCtx->backLabel);
        gtk_widget_show(gCtx->next);
        gtk_widget_show(gCtx->back);
    }
        // from install dlg
    if (aDirection == nsXInstallerDlg::BACKWARD_MOVE && 
        // not custom setup type
        gCtx->opt->mSetupType != (gCtx->sdlg->GetNumSetupTypes() - 1))
    {
        DUMP("Back from Install to Setup Type");
        gtk_container_remove(GTK_CONTAINER(gCtx->next), gCtx->installLabel);
        gCtx->nextLabel = gtk_label_new(NEXT);
        gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
        gtk_widget_show(gCtx->nextLabel);
        gtk_widget_show(gCtx->next);
    }     

    gtk_widget_hide(gCtx->back);

    return err;
}

int
nsSetupTypeDlg::Hide(int aDirection)
{
    gtk_widget_hide(mTable);

    return OK;
}

int
nsSetupTypeDlg::SetMsg0(char *aMsg)
{
    if (!aMsg)
        return E_PARAM;

    mMsg0 = aMsg;

    return OK;
}

char *
nsSetupTypeDlg::GetMsg0()
{
    if (mMsg0)
        return mMsg0;

    return NULL;
}

int
nsSetupTypeDlg::AddSetupType(nsSetupType *aSetupType)
{
    if (!aSetupType)
        return E_PARAM;

    if (!mSetupTypeList)
    {
        mSetupTypeList = aSetupType;
        return OK;
    }

    nsSetupType *curr = mSetupTypeList;
    nsSetupType *next;
    while (curr)
    {
        next = NULL;
        next = curr->GetNext();
    
        if (!next)
        {
            return curr->SetNext(aSetupType);
        }

        curr = next;
    }

    return OK;
}

nsSetupType *
nsSetupTypeDlg::GetSetupTypeList()
{
    if (mSetupTypeList)
        return mSetupTypeList;

    return NULL;
}

int
nsSetupTypeDlg::GetNumSetupTypes()
{
    int num = 0;
    nsSetupType *curr = NULL;

    if (!mSetupTypeList)
        return 0;
    
    curr = mSetupTypeList;
    while(curr)
    {
        num++;
        curr = curr->GetNext();
    }

    return num;
}

nsSetupType *
nsSetupTypeDlg::GetSelectedSetupType()
{
    nsSetupType *curr = NULL;
    int numSetupTypes = GetNumSetupTypes();
    int setupTypeCount = 0;

    curr = GetSetupTypeList();
    while (curr && setupTypeCount < numSetupTypes)  // paranoia!
    {
        if (setupTypeCount == gCtx->opt->mSetupType)
            return curr;        

        setupTypeCount++;
        curr = curr->GetNext();
    }

    return NULL;
}

void
nsSetupTypeDlg::FreeSetupTypeList()
{
    nsSetupType *curr = mSetupTypeList;
    nsSetupType *prev;
    
    while (curr)
    {
        prev = curr;
        curr = curr->GetNext();

        XI_IF_DELETE(prev);
    }
}

void
nsSetupTypeDlg::SelectFolder(GtkWidget *aWidget, gpointer aData)
{
    DUMP("SelectFolder");

    GtkWidget *fileSel = NULL;
    char *selDir = gCtx->opt->mDestination;

    fileSel = gtk_file_selection_new(SELECT_DIR);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileSel), selDir);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSel)->ok_button),
                       "clicked", (GtkSignalFunc) SelectFolderOK, fileSel);
    gtk_signal_connect_object(GTK_OBJECT(
                                GTK_FILE_SELECTION(fileSel)->cancel_button),
                                "clicked", (GtkSignalFunc) gtk_widget_destroy,
                                GTK_OBJECT(fileSel));
    gtk_widget_show(fileSel); 

    
    
    // XXX very much incomplete...
}

void
nsSetupTypeDlg::SelectFolderOK(GtkWidget *aWidget, GtkFileSelection *aFileSel)
{
    DUMP("SelectFolderOK");

    struct stat destStat;
    char *selDir = gtk_file_selection_get_filename(
                    GTK_FILE_SELECTION(aFileSel));
    if (0 == stat(selDir, &destStat))
        if (!S_ISDIR(destStat.st_mode)) /* not a directory so don't tear down */
            return;

    strcpy(gCtx->opt->mDestination, selDir);

    // update folder path displayed
    gtk_label_set_text(GTK_LABEL(sFolder), gCtx->opt->mDestination);
    gtk_widget_show(sFolder);

    // tear down file sel dlg
    gtk_object_destroy(GTK_OBJECT(aFileSel)); 
}

void
nsSetupTypeDlg::RadBtnToggled(GtkWidget *aWidget, gpointer aData)
{
    DUMP("RadBtnToggled");
    
    gCtx->opt->mSetupType = (int) aData;
}

int
nsSetupTypeDlg::VerifyDestination()
{
    int err = E_NO_DEST;
    int stat_err = 0;
    struct stat dummy; 
    GtkWidget *yesButton, *noButton, *label;
    char message[1024];
    
    stat_err = stat(gCtx->opt->mDestination, &dummy);
    if (stat_err == 0)
        return OK;

    // destination doesn't exist so ask user if we should create it
    memset(message, 0, 1024);
    sprintf(message, DOESNT_EXIST, gCtx->opt->mDestination);

    sCreateDestDlg = gtk_dialog_new();
    label = gtk_label_new(message);
    yesButton = gtk_button_new_with_label(YES_LABEL);
    noButton = gtk_button_new_with_label(NO_LABEL);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sCreateDestDlg)->action_area),
                      yesButton);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sCreateDestDlg)->action_area),
                      noButton);
    gtk_signal_connect(GTK_OBJECT(yesButton), "clicked",
                       GTK_SIGNAL_FUNC(CreateDestYes), sCreateDestDlg);
    gtk_signal_connect(GTK_OBJECT(noButton), "clicked",
                       GTK_SIGNAL_FUNC(CreateDestNo), sCreateDestDlg);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(sCreateDestDlg)->vbox), label);
    
    gtk_widget_show_all(sCreateDestDlg);

    return err;
}

void
nsSetupTypeDlg::CreateDestYes(GtkWidget *aWidget, gpointer aData)
{
    DUMP("CreateDestYes");
    int err = 0; 
    err = mkdir(gCtx->opt->mDestination, 0755);
    gtk_widget_destroy(sCreateDestDlg);

    if (err != 0)
        ErrorHandler(E_MKDIR_FAIL);

    // hide this notebook page
    gCtx->sdlg->Hide(nsXInstallerDlg::FORWARD_MOVE);

    // disconnect this dlg's nav btn signal handlers
    gtk_signal_disconnect(GTK_OBJECT(gCtx->back), gCtx->backID);
    gtk_signal_disconnect(GTK_OBJECT(gCtx->next), gCtx->nextID);
    gtk_signal_disconnect(GTK_OBJECT(sBrowseBtn), sBrowseBtnID);

    // show the last dlg
    if (gCtx->opt->mSetupType == (gCtx->sdlg->GetNumSetupTypes() - 1))
        gCtx->cdlg->Show(nsXInstallerDlg::FORWARD_MOVE);
    else
        gCtx->idlg->Show(nsXInstallerDlg::FORWARD_MOVE);
}

void
nsSetupTypeDlg::CreateDestNo(GtkWidget *aWidget, gpointer aData)
{
    DUMP("CreateDestNo");

    gtk_widget_destroy(sCreateDestDlg);
}
