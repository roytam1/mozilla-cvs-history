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


#include "nsXInstaller.h"
#include "logo.xpm"

nsXIContext *gCtx = NULL;

nsXInstaller::nsXInstaller()
{
}

nsXInstaller::~nsXInstaller()
{
}

int
nsXInstaller::ParseConfig()
{
    int err = OK;
    nsINIParser *parser = NULL; 

    XI_ERR_BAIL(InitContext());

    parser = new nsINIParser( CONFIG_INI );
    if (!parser)
        return E_MEM;    

    err = parser->GetError();
    if (err != nsINIParser::OK)
        return err;

    XI_ERR_BAIL(ParseGeneral(parser));
    XI_ERR_BAIL(gCtx->ldlg->Parse(parser));
    XI_ERR_BAIL(gCtx->wdlg->Parse(parser));
    XI_ERR_BAIL(gCtx->cdlg->Parse(parser)); // components before setup type
    XI_ERR_BAIL(gCtx->sdlg->Parse(parser));
    XI_ERR_BAIL(gCtx->idlg->Parse(parser));

    return OK;

BAIL:
    return err;
}

int 
nsXInstaller::InitContext()
{
    int err = OK;

    gCtx = new nsXIContext();
    if (!gCtx)
        return E_MEM;

    gCtx->me = this;

    gCtx->ldlg = new nsLicenseDlg();
    gCtx->wdlg = new nsWelcomeDlg();
    gCtx->sdlg = new nsSetupTypeDlg();
    gCtx->cdlg = new nsComponentsDlg();
    gCtx->idlg = new nsInstallDlg();
    if (!gCtx->ldlg || !gCtx->wdlg || !gCtx->sdlg || 
        !gCtx->cdlg || !gCtx->idlg )
    {
        err = E_MEM;
        goto BAIL;
    }

    return OK;
    
BAIL:
    XI_IF_DELETE(gCtx->ldlg);
    XI_IF_DELETE(gCtx->wdlg);
    XI_IF_DELETE(gCtx->sdlg);
    XI_IF_DELETE(gCtx->cdlg);
    XI_IF_DELETE(gCtx->idlg);
    XI_IF_DELETE(gCtx);

    return err;
}

int 
nsXInstaller::RunWizard(int argc, char **argv)
{
    int err = OK;
    GtkWidget *logovbox;

    XI_VERIFY(gCtx);

    // create the dialog window
    gtk_init(&argc, &argv);
    gdk_rgb_init();

    gCtx->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    XI_VERIFY(gCtx->window);
    gtk_signal_connect(GTK_OBJECT(gCtx->window), "delete_event",
                       GTK_SIGNAL_FUNC(Kill), NULL);

    gtk_widget_set_usize(gCtx->window, XI_WIN_WIDTH, XI_WIN_HEIGHT);
    gtk_container_set_border_width(GTK_CONTAINER(gCtx->window), 5);
    gtk_window_set_title(GTK_WINDOW(gCtx->window), "Mozilla Installer"); // XXX
    gtk_widget_show(gCtx->window);

    // create and display the logo and cancel button
    logovbox = DrawLogo();
    if (logovbox)
        DrawCancelButton(logovbox);

    // create and display the nav buttons
    XI_ERR_BAIL(DrawNavButtons());

    // create the notebook whose pages are dlgs
    gCtx->notebook = gtk_notebook_new();
    XI_VERIFY(gCtx->notebook);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gCtx->notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(gCtx->notebook), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(gCtx->notebook), FALSE);
    gtk_widget_show(gCtx->notebook);
    gtk_container_add(GTK_CONTAINER(gCtx->canvas), gCtx->notebook);

    // show welcome dlg
    gCtx->wdlg->Show(nsXInstallerDlg::FORWARD_MOVE); 

    // pop over to main event loop
    gtk_main();

    return OK;

BAIL:
    return err;
}

gint
nsXInstaller::Kill(GtkWidget *widget, GtkWidget *event, gpointer data)
{
    gtk_main_quit();
    XI_IF_DELETE(gCtx);
    return FALSE;
}

GtkWidget *
nsXInstaller::DrawLogo()
{
    GtkWidget *logo = NULL;
    GdkPixmap *pixmap = NULL;
    GdkBitmap *mask = NULL;
    GtkStyle *style = NULL;
    GtkWidget *mainhbox = NULL;
    GtkWidget *logovbox = NULL;
    GtkWidget *canvasvbox = NULL;

    style = gtk_widget_get_style(gCtx->window);
    pixmap = gdk_pixmap_create_from_xpm_d(gCtx->window->window, &mask,
                                          &style->bg[GTK_STATE_NORMAL],
                                          (gchar **)logo_xpm);

    logo = gtk_pixmap_new(pixmap, mask);
    gtk_widget_show(logo);

    mainhbox = gtk_hbox_new(FALSE, 10);
    logovbox = gtk_vbox_new(FALSE, 30);
    canvasvbox = gtk_vbox_new(FALSE, 10);

    gtk_box_pack_start(GTK_BOX(logovbox), logo, FALSE, FALSE, 0);
    gtk_widget_show(logovbox);
    gtk_widget_show(canvasvbox);

    gtk_box_pack_start(GTK_BOX(mainhbox), logovbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mainhbox), canvasvbox, TRUE, TRUE, 0);
    gtk_widget_show(mainhbox);

    gtk_container_add(GTK_CONTAINER(gCtx->window), mainhbox);

    gCtx->mainbox = canvasvbox; /* canvasvbox = canvas + nav btns' box */

    return logovbox;
}

int
nsXInstaller::DrawCancelButton(GtkWidget *aLogoVBox)
{
    int err = OK;
    GtkWidget *hbox;

    gCtx->cancel = gtk_button_new_with_label(CANCEL);
    hbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), gCtx->cancel, TRUE, TRUE, 15);
    gtk_box_pack_end(GTK_BOX(aLogoVBox), hbox, FALSE, TRUE, 10);
    gtk_signal_connect(GTK_OBJECT(gCtx->cancel), "clicked",
                       GTK_SIGNAL_FUNC(Kill), NULL);
    gtk_widget_show(hbox);
    gtk_widget_show(gCtx->cancel);

    return err;
}

int
nsXInstaller::DrawNavButtons()
{
    int err = OK;
    
    GtkWidget *navbtnhbox;
    GtkWidget *canvasvbox;
    GtkWidget *navbtntable;

    XI_VERIFY(gCtx->mainbox);

    gCtx->next = gtk_button_new();  
    gCtx->back = gtk_button_new(); 
    gCtx->nextLabel = gtk_label_new(NEXT);
    gCtx->backLabel = gtk_label_new(BACK);
    XI_VERIFY(gCtx->next);
    XI_VERIFY(gCtx->back);
    gtk_widget_show(gCtx->next);
    gtk_widget_show(gCtx->back);
    gtk_container_add(GTK_CONTAINER(gCtx->next), gCtx->nextLabel);
    gtk_container_add(GTK_CONTAINER(gCtx->back), gCtx->backLabel);
    gtk_widget_show(gCtx->nextLabel);
    gtk_widget_show(gCtx->backLabel);
    
    navbtnhbox = gtk_hbox_new(TRUE, 10);
    canvasvbox = gtk_vbox_new(TRUE, 10);
    gtk_box_pack_start(GTK_BOX(gCtx->mainbox), canvasvbox, TRUE, TRUE, 0); 
    gtk_box_pack_start(GTK_BOX(gCtx->mainbox), navbtnhbox, FALSE, FALSE, 0); 

    // put a table in the nav btn box
    navbtntable = gtk_table_new(1, 6, TRUE);
    gtk_box_pack_start(GTK_BOX(navbtnhbox), navbtntable, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(navbtntable), gCtx->back, 4, 5, 0, 1, 
        static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
		static_cast<GtkAttachOptions>( GTK_FILL | GTK_EXPAND),
		5, 5);
    gtk_table_attach(GTK_TABLE(navbtntable), gCtx->next, 5, 6, 0, 1,
        static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
		static_cast<GtkAttachOptions>(GTK_FILL | GTK_EXPAND),
		5, 5);

    gtk_widget_show(navbtntable);    
    gtk_widget_show(navbtnhbox); 
    gtk_widget_show(canvasvbox);

    gtk_widget_show(gCtx->mainbox);

    XI_VERIFY(canvasvbox);
    gCtx->canvas = canvasvbox; 

    return err;
}

int
nsXInstaller::ParseGeneral(nsINIParser *aParser)
{
    int     err = OK;
    char    *dest = NULL;
    int     size = 0;
 
    /* optional: destination directory can be specified in config.ini */
    err = aParser->GetStringAlloc(GENERAL, DEFAULT_LOCATION, &dest, &size);
    if (err == OK)
        gCtx->opt->mDestination = dest;

    return err;
}

int
main(int argc, char **argv)
{
    nsXInstaller *installer = new nsXInstaller();
    int err = OK;

    if (installer)
    {
        if ( (err = installer->ParseConfig()) == OK)
            err = installer->RunWizard(argc, argv);
    }
    else
        err = E_MEM;

    XI_IF_DELETE(installer);
    DUMP("post nsXInstaller instance deletion");

	exit(err);
}

