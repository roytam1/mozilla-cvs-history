

#include "nscore.h"

#include "nsCaretProperties.h"



//-----------------------------------------------------------------------------
nsCaretProperties::nsCaretProperties()
:	mCaretWidth(eDefaultCaretWidth)
,	mBlinkRate(eDefaulBlinkRate)
{
	// in your platform-specific class, get data from the OS in your constructor
}


//-----------------------------------------------------------------------------
nsCaretProperties* NewCaretProperties()
{
	return new nsCaretProperties();
}
