/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsScrollbar.h"
#include "nsToolkit.h"
#include "nsGUIEvent.h"
#include "nsUnitConversion.h"
#include "nsWindow.h"

PRLogModuleInfo * QtScrollLM = PR_NewLogModule("QtScroll");

//=============================================================================
//
// nsQScrollBar class
//
//=============================================================================
nsQScrollBar::nsQScrollBar(nsWidget * widget,
                           int minValue, 
                           int maxValue, 
                           int LineStep, 
                           int PageStep, 
                           int value, 
                           Orientation orientation,
                           QWidget * parent, 
                           const char * name)
	: QScrollBar(minValue, 
                 maxValue, 
                 LineStep, 
                 PageStep, 
                 value,
                 orientation, 
                 parent, 
                 name),
      nsQBaseWidget(widget)
{
#if 1
    connect((QScrollBar *)this,
            //SIGNAL(sliderMoved(int)),
            SIGNAL(valueChanged(int)),
            SLOT(SetValue(int)));
#endif
#if 0
    connect((QScrollBar *) this,
            SIGNAL(nextLine()),
            SLOT(NextLine()));
    connect((QScrollBar *) this,
            SIGNAL(prevLine()),
            SLOT(PreviousLine()));
    connect((QScrollBar *) this,
            SIGNAL(nextPage()),
            SLOT(NextPage()));
    connect((QScrollBar *) this,
            SIGNAL(prevPage()),
            SLOT(PreviousPage()));
#endif
}

nsQScrollBar::~nsQScrollBar()
{
}

void nsQScrollBar::SetValue(int value)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::SetValue: setting %d\n",
            value));
    ScrollBarMoved(NS_SCROLLBAR_POS, value);
}

void nsQScrollBar::PreviousLine()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::PreviousLine()\n"));
    ScrollBarMoved(NS_SCROLLBAR_LINE_PREV);
}

void nsQScrollBar::NextLine()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::NextLine()\n"));
    ScrollBarMoved(NS_SCROLLBAR_LINE_NEXT);
}

void nsQScrollBar::PreviousPage()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::PreviousPage()\n"));
    ScrollBarMoved(NS_SCROLLBAR_PAGE_PREV);
}

void nsQScrollBar::NextPage()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::NextPage()\n"));
    ScrollBarMoved(NS_SCROLLBAR_PAGE_NEXT);
}

void nsQScrollBar::ScrollBarMoved(int message, int value)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsQScrollBar::ScrollBarMoved()\n"));
    if (mWidget)
    {
        nsScrollbarEvent nsEvent;

        nsEvent.message         = message;
        nsEvent.widget          = mWidget;
        NS_IF_ADDREF(nsEvent.widget);
        nsEvent.eventStructType = NS_SCROLLBAR_EVENT;
        nsEvent.position        = value;
        
        ((nsScrollbar *)mWidget)->OnScroll(nsEvent, value);
    }
}

NS_IMPL_ADDREF (nsScrollbar);
NS_IMPL_RELEASE (nsScrollbar);

//-------------------------------------------------------------------------
//
// nsScrollbar constructor
//
//-------------------------------------------------------------------------
nsScrollbar::nsScrollbar(PRBool aIsVertical) : nsWidget (), nsIScrollbar ()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::nsScrollbar()\n"));
    mOrientation = aIsVertical ? QScrollBar::Vertical : QScrollBar::Horizontal;
    mLineStep = 1;
    mPageStep = 10;
    mMaxValue = 100;
    mValue    = 0;
}

//-------------------------------------------------------------------------
//
// nsScrollbar destructor
//
//-------------------------------------------------------------------------
nsScrollbar::~nsScrollbar()
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::~nsScrollbar()\n"));
}

//-------------------------------------------------------------------------
//
// Create the native scrollbar widget
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::CreateNative(QWidget * parentWindow)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::CreateNative: max=%d, linestep=%d, pagestep=%d\n",
            mMaxValue, 
            mLineStep, 
            mPageStep));
    mWidget = new nsQScrollBar(this, 
                               0,
                               mMaxValue,
                               mLineStep,
                               mPageStep,
                               mValue,
                               mOrientation, 
                               parentWindow, 
                               QScrollBar::tr("nsScrollBar"));

    if (mWidget)
    {
        ((QScrollBar *)mWidget)->setTracking(true);
    }

    return NS_OK;
    //return nsWidget::CreateNative(parentWindow);
}

//-------------------------------------------------------------------------
//
// Query interface implementation
//
//-------------------------------------------------------------------------
nsresult nsScrollbar::QueryInterface(const nsIID & aIID, void **aInstancePtr)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::QueryInterface()\n"));
    nsresult result = nsWidget::QueryInterface(aIID, aInstancePtr);

    static NS_DEFINE_IID(kInsScrollbarIID, NS_ISCROLLBAR_IID);
    if (result == NS_NOINTERFACE && aIID.Equals(kInsScrollbarIID)) 
    {
        *aInstancePtr = (void*) ((nsIScrollbar*)this);
        NS_ADDREF_THIS();
        result = NS_OK;
    }

    return result;
}

//-------------------------------------------------------------------------
//
// Define the range settings
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetMaxRange(PRUint32 aEndRange)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::SetMaxRange()\n"));
    mMaxValue = aEndRange;

    ((QScrollBar *)mWidget)->setRange(0, mMaxValue - mPageStep);

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Return the range settings
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetMaxRange(PRUint32 & aMaxRange)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::GetMaxRange: %d\n",
            aMaxRange));
    aMaxRange = mMaxValue;

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb position
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetPosition(PRUint32 aPos)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::SetPosition()\n"));
    mValue = aPos;

    ((QScrollBar *)mWidget)->setValue(aPos);

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the current thumb position.
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetPosition(PRUint32 & aPos)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::GetPosition %u\n", 
            mValue));
    aPos = mValue;

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the thumb size
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetThumbSize(PRUint32 aSize)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::SetThumbSize()\n"));
    if (aSize > 0)
    {
        mPageStep = aSize;
        
        ((QScrollBar *)mWidget)->setSteps(mLineStep, mPageStep);
    }

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the thumb size
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetThumbSize(PRUint32 & aThumbSize)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::GetThumbSize: %u\n",
            mPageStep));
    aThumbSize = mPageStep;

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetLineIncrement(PRUint32 aLineIncrement)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::SetLineIncrement\n"));
    if (aLineIncrement > 0)
    {
        mLineStep = aLineIncrement;

        ((QScrollBar *)mWidget)->setSteps(mLineStep, mPageStep);
    }

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get the line increment for this scrollbar
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::GetLineIncrement(PRUint32 & aLineInc)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::GetLineIncrement: %u\n",
            mLineStep));
    aLineInc = mLineStep;

    return NS_OK;
}


//-------------------------------------------------------------------------
//
// Set all scrolling parameters
//
//-------------------------------------------------------------------------
NS_METHOD nsScrollbar::SetParameters(PRUint32 aMaxRange, 
                                     PRUint32 aThumbSize,
                                     PRUint32 aPosition, 
                                     PRUint32 aLineIncrement)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::SetParameters: max=%d,thumb=%d,pos=%d,inc=%d\n", 
            aMaxRange, 
            aThumbSize, 
            aPosition, 
            aLineIncrement));
    mPageStep = (int) ((aThumbSize > 0) ? aThumbSize : 1);
    mValue    = (int) ((aPosition > 0) ? aPosition : 0);
    mLineStep = (int) ((aLineIncrement > 0) ? aLineIncrement : 1);
    mMaxValue = (int) ((aMaxRange > 0) ? aMaxRange : 10);
    
    ((QScrollBar *)mWidget)->setValue(mValue);
    ((QScrollBar *)mWidget)->setSteps(mLineStep, mPageStep);
    ((QScrollBar *)mWidget)->setRange(0, mMaxValue - mPageStep);

    return NS_OK;
}

//-------------------------------------------------------------------------
//
// Deal with scrollbar messages (actually implemented only in nsScrollbar)
//
//-------------------------------------------------------------------------
PRBool nsScrollbar::OnScroll(nsScrollbarEvent & aEvent, PRUint32 cPos)
{
    PR_LOG(QtScrollLM, 
           PR_LOG_DEBUG, 
           ("nsScrollBar::OnScroll()\n"));
    PRBool result = PR_TRUE;

    switch (aEvent.message)
    {

        // scroll one line right or down
    case NS_SCROLLBAR_LINE_NEXT:
    {
        ((QScrollBar *)mWidget)->addLine();
        
        mValue = ((QScrollBar *)mWidget)->value();

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
        {
            aEvent.position = (PRUint32) mValue;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            mValue = aEvent.position;
        }
        break;
    }


    // scroll one line left or up
    case NS_SCROLLBAR_LINE_PREV:
    {
        ((QScrollBar *)mWidget)->subtractLine();
        mValue = ((QScrollBar *)mWidget)->value();

        // if an event callback is registered, give it the chance
        // to change the decrement
        if (mEventCallback)
        {
            aEvent.position = (PRUint32) mValue;
            aEvent.widget = (nsWidget *) this;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            mValue = aEvent.position;
        }
        break;
    }

    // Scrolls one page right or down
    case NS_SCROLLBAR_PAGE_NEXT:
    {
        ((QScrollBar *)mWidget)->addPage();
        mValue = ((QScrollBar *)mWidget)->value();

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
        {
            aEvent.position = (PRUint32) mValue;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            mValue = aEvent.position;
        }
        break;
    }

    // Scrolls one page left or up.

    case NS_SCROLLBAR_PAGE_PREV:
    {
        ((QScrollBar *)mWidget)->subtractPage();
        mValue = ((QScrollBar *)mWidget)->value();

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
        {
            aEvent.position = (PRUint32) mValue;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            mValue = aEvent.position;
        }
        break;
    }


    // Scrolls to the absolute position. The current position is specified by
    // the cPos parameter.
    case NS_SCROLLBAR_POS:
    {
        mValue = cPos;

        // if an event callback is registered, give it the chance
        // to change the increment
        if (mEventCallback)
        {
            aEvent.position = (PRUint32) mValue;
            result = ConvertStatus ((*mEventCallback) (&aEvent));
            mValue = aEvent.position;
        }
        break;
    }
    }

    //((QScrollBar *)mWidget)->setValue(mValue);

    return result;
}


