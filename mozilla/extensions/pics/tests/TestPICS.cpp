#include "nsXPCOM.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIPICS.h"
#include "prmem.h"
#include "plstr.h"
#include "nspics.h"

static NS_DEFINE_IID(kPICSCID, NS_PICS_CID);

PRInt32 main(PRInt32 argc, char *argv[])
{

	char *fileBuf2;

	char fileBuf[10000] = 
			"(PICS-1.1 \"http://www.gcf.org/v2.5\" labels by \"John Doe\" for \"http://www.w3.org/DSig/DsigProj.html\" ratings (suds 0.5))";

	char fileBuf1[10000] = 
			"(PICS-1.1 \"http://www.gcf.org/v2.5\" l r (suds 0.5))";

  
	fileBuf2 = (char *)PR_Malloc(1000);
	PL_strcpy(fileBuf2, "(PICS-1.1 \"http://www.gcf.org/v2.5\" \n by \"John Doe\" labels for \"http://www.w3.org/DSig/DsigProj.html\" ratings (suds 0.5))");


	char fileBuf3[10000] = 
			"(PICS-1.1 l r (suds 0.5))";

	char fileBuf4[1000] = "(PICS-1.1 \"http://www.classify.org/safesurf/\" l gen true for \"http://www.w3.org\"     by \"philipd@w3.org\" r (ss~~000 1 ss~~100 1))";

	char fileBuf5[1000] = "(PICS-1.1 \"http://www.rsac.org/ratingsv01.html\" l gen true comment    \"RSACi North America Server\" by \"philipd@w3.org\" for \"http://www.w3.org\"     on \"1996.04.16T08:15-0500\" r (n 0 s 0 v 0 l 0))";

	char fileBuf6[1000] = "(PICS-1.1 \"http://www.gcf.org/v2.5\"  l r (suds 0.5 density 0 color/hue 1) r (subject 2 density 1 color/hue 1))";

	char fileBuf7[1000] = "(PICS-1.1 \"http://www.gcf.org/v2.5\" by \"John Doe\" labels on \"1994.11.05T08:15-0500\" until \"1995.12.31T23:59-0000\" for \"http://w3.org/PICS/Overview.html\" ratings (suds 0.5 density 0 color/hue 1) for \"http://w3.org/PICS/Underview.html\" by \"Jane Doe\" ratings (subject 2 density 1 color/hue 1))";
    
  char fileBuf8[1000] = "(PICS-1.1 \"\"http://www.rsac.org/ratingsv01.html\"\" l gen true r (n 0 s 0 v 0 l 0)";
  char fileBuf9[1000] = "(PICS-1.1 \"http://www.rsac.org/ratingsv01.html\" l gen true r (n 0 s 0 v 0 l 0)";
  char fileBuf10[1000] = "\"(PICS-1.1)";

  char fileBuf11[1000] =   "(PICS-1.1 \"http://www.rsac.org/ratingsv01.html\" l gen true comment \"RSACi North America Server\" by \"philipd@w3.org\" for \"http://www.w3.org\" on \"1996.04.16T08:15-0500\" r (n 0 s 0 v 0 l 0))";


  nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
  if (NS_FAILED(rv))
    return 1;

  nsCOMPtr<nsIPICS> pics = do_CreateInstance(kPICSCID);

  if (pics)
    pics->ProcessPICSLabel(fileBuf11);

	PR_Free(fileBuf2);
	return 0;
}

