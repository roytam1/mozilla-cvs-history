/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

#include "nsNNTPHost.h"

#include "nsMsgNewsCID.h"

static NS_DEFINE_CID(kNNTPNewsgroupCID, NS_NNTPNEWSGROUP_CID);
static NS_DEFINE_CID(kNNTPNewsgroupListCID, NS_NNTPNEWSGROUPLIST_CID);

#ifdef XP_UNIX
static const char LINEBREAK_START = '\012';
#else
static const char LINEBREAK_START = '\015';
#endif

#define PROTOCOL_DEBUG 

nsNNTPHost * nsNNTPHost::M_FileOwner = nsnull;

NS_IMPL_ISUPPORTS(nsNNTPHost, GetIID())


nsNNTPHost::nsNNTPHost()
{
	NS_INIT_REFCNT();
}

/* we're not supposed to implement this */
 
nsNNTPHost::~nsNNTPHost()
{
}

nsresult nsNNTPHost::Initialize(const char *name, PRInt32 port)
{
    m_hostname = new char [PL_strlen(name) + 1];
	PL_strcpy(m_hostname, name);

	PR_ASSERT(port);
	if (port == 0) port = NEWS_PORT;
	m_port = port;

	m_searchableGroupCharsets = PL_NewHashTable(20, PL_HashString, PL_CompareStrings, PL_CompareValues, nsnull, nsnull);

	m_nameAndPort = nsnull;
	m_fullUIName = nsnull;
	m_optionLines = nsnull;
	m_filename = nsnull;
	m_groups = nsnull;
	m_newsgrouplists = nsnull;
	m_dbfilename = nsnull;
	m_dirty = 0;
	m_writetimer = nsnull;
	m_postingAllowed = PR_TRUE;
	m_supportsExtensions = PR_FALSE;
	m_pushAuth = PR_FALSE;
	m_groupSucceeded = PR_FALSE;
	m_lastGroupUpdate = 0;
	m_groupTree = nsnull;
    m_inhaled = PR_FALSE;
    m_uniqueId = 0;

    return NS_OK;
}

nsresult
nsNNTPHost::CleanUp() { 
	if (m_dirty) WriteNewsrc();
	if (m_groupTreeDirty) SaveHostInfo();
	PR_FREEIF(m_optionLines);
	delete [] m_filename;
	delete [] m_hostname;
	PR_FREEIF(m_nameAndPort);
	PR_FREEIF(m_fullUIName);
	NS_IF_RELEASE(m_groups);
	NS_IF_RELEASE(m_newsgrouplists);
	delete [] m_dbfilename;
	delete m_groupTree;
	if (m_block)
		delete [] m_block;
#ifdef UNREADY_CODE
	if (m_groupFile)
		XP_FileClose (m_groupFile);
#endif
	PR_FREEIF(m_groupFilePermissions);
	if (M_FileOwner == this) M_FileOwner = nsnull;
	PR_FREEIF(m_hostinfofilename);
	PRInt32 i;
	for (i = 0; i < m_supportedExtensions.Count(); i++)
		PR_Free((char*) m_supportedExtensions[i]);
	for (i = 0; i < m_searchableGroups.Count(); i++)
		PR_Free((char*) m_searchableGroups[i]);
	for (i = 0; i < m_searchableHeaders.Count(); i++)
		PR_Free((char*) m_searchableHeaders[i]);
	for (i = 0; i < m_propertiesForGet.Count(); i++)
		PR_Free((char*) m_propertiesForGet[i]);
	for (i = 0; i < m_valuesForGet.Count(); i++)
		PR_Free((char*) m_valuesForGet[i]);
	if (m_searchableGroupCharsets)
	{
		// We do NOT free the individual key/value pairs,
		// because deleting m_searchableGroups above has
		// already caused this to happen.
		PL_HashTableDestroy(m_searchableGroupCharsets);
		m_searchableGroupCharsets = nsnull;
	}
    return NS_OK;
}



void
nsNNTPHost::OpenGroupFile(const PRIntn permissions)
{
#ifdef UNREADY_CODE
	PR_ASSERT(permissions);
	if (!permissions) return;
	if (m_groupFile) {
		if (m_groupFilePermissions &&
			  PL_strcmp(m_groupFilePermissions, permissions) == 0) {
			return;
		}
		XP_FileClose(m_groupFile);
		m_groupFile = nsnull;
	}
	if (M_FileOwner && M_FileOwner != this && M_FileOwner->m_groupFile) {
		XP_FileClose(M_FileOwner->m_groupFile);
		M_FileOwner->m_groupFile = nsnull;
	}
	M_FileOwner = this;
	PR_FREEIF(m_groupFilePermissions);
	m_groupFilePermissions = PL_strdup(permissions);
	m_groupFile = XP_FileOpen(m_hostinfofilename, xpXoverCache, permissions);
#endif
}



void nsNNTPHost::ClearNew()
{
	PRInt64 now = PR_Now();
	PRInt64 increment;
	
	LL_I2L(increment,1);
	LL_ADD(m_firstnewdate,now,increment);
}



void nsNNTPHost::dump()
{
	// ###tw  Write me...
}


PRInt32 nsNNTPHost::getPort()
{
	return m_port;
}

const char* nsNNTPHost::getNameAndPort()
{
	if (!m_nameAndPort) {
		if (m_port != (NEWS_PORT)) {
			m_nameAndPort = PR_smprintf("%s:%ld", m_hostname, long(m_port));
		} else {
			m_nameAndPort = PL_strdup(m_hostname);
		}
	}
	return m_nameAndPort;
}

const char* nsNNTPHost::getFullUIName()
{
	if (!m_fullUIName) {
		return getNameAndPort();
	}
	return m_fullUIName;
}

/* sspitzer:  from mozilla/network/protocol/pop3/mkpop3.c */

static PRInt32
msg_GrowBuffer (PRUint32 desired_size, PRUint32 element_size, PRUint32 quantum,
				char **buffer, PRUint32 *size)
{
  if (*size <= desired_size)
	{
	  char *new_buf;
	  PRUint32 increment = desired_size - *size;
	  if (increment < quantum) /* always grow by a minimum of N bytes */
		increment = quantum;

#ifdef TESTFORWIN16
	  if (((*size + increment) * (element_size / sizeof(char))) >= 64000)
		{
		  /* Make sure we don't choke on WIN16 */
		  PR_ASSERT(0);
		  return -1;
		}
#endif /* TESTFORWIN16 */

	  new_buf = (*buffer
				 ? (char *) PR_Realloc (*buffer, (*size + increment)
										* (element_size / sizeof(char)))
				 : (char *) PR_Malloc ((*size + increment)
									  * (element_size / sizeof(char))));
	  if (! new_buf)
      return -1; // NS_ERROR_OUT_OF_MEMORY;
	  *buffer = new_buf;
	  *size += increment;
	}
  return 0;
}

/* Take the given buffer, tweak the newlines at the end if necessary, and
   send it off to the given routine.  We are guaranteed that the given
   buffer has allocated space for at least one more character at the end. */
static PRInt32
msg_convert_and_send_buffer(char* buf, PRUint32 length, PRBool convert_newlines_p,
							nsresult (*per_line_fn) (char *line,
												  PRUint32 line_length,
												  void *closure),
							void *closure)
{
  /* Convert the line terminator to the native form.
   */
  char* newline;

  PR_ASSERT(buf && length > 0);
  if (!buf || length <= 0) return -1;
  newline = buf + length;
  PR_ASSERT(newline[-1] == CR || newline[-1] == LF);
  if (newline[-1] != CR && newline[-1] != LF) return -1;

  if (!convert_newlines_p)
	{
	}
#if (MSG_LINEBREAK_LEN == 1)
  else if ((newline - buf) >= 2 &&
		   newline[-2] == CR &&
		   newline[-1] == LF)
	{
	  /* CRLF -> CR or LF */
	  buf [length - 2] = MSG_LINEBREAK[0];
	  length--;
	}
  else if (newline > buf + 1 &&
		   newline[-1] != MSG_LINEBREAK[0])
	{
	  /* CR -> LF or LF -> CR */
	  buf [length - 1] = MSG_LINEBREAK[0];
	}
#else
  else if (((newline - buf) >= 2 && newline[-2] != CR) ||
		   ((newline - buf) >= 1 && newline[-1] != LF))
	{
	  /* LF -> CRLF or CR -> CRLF */
	  length++;
	  buf[length - 2] = MSG_LINEBREAK[0];
	  buf[length - 1] = MSG_LINEBREAK[1];
	}
#endif

  return (*per_line_fn)(buf, length, closure);
}

static PRInt32
msg_LineBuffer (const char *net_buffer, PRInt32 net_buffer_size,
				char **bufferP, PRUint32 *buffer_sizeP, PRUint32 *buffer_fpP,
				PRBool convert_newlines_p,
				nsresult (*per_line_fn) (char *line, PRUint32 line_length,
									  void *closure),
				void *closure)
{
#ifdef DEBUG_NEWS
  printf("msg_LineBuffer()\n");
#endif

  PRInt32 status = 0;
  if (*buffer_fpP > 0 && *bufferP && (*bufferP)[*buffer_fpP - 1] == CR &&
	  net_buffer_size > 0 && net_buffer[0] != LF) {
	/* The last buffer ended with a CR.  The new buffer does not start
	   with a LF.  This old buffer should be shipped out and discarded. */
	PR_ASSERT(*buffer_sizeP > *buffer_fpP);
	if (*buffer_sizeP <= *buffer_fpP) return -1;
	status = msg_convert_and_send_buffer(*bufferP, *buffer_fpP,
										 convert_newlines_p,
										 per_line_fn, closure);
	if (status < 0) return status;
	*buffer_fpP = 0;
  }
  while (net_buffer_size > 0)
	{
	  const char *net_buffer_end = net_buffer + net_buffer_size;
	  const char *newline = 0;
	  const char *s;


	  for (s = net_buffer; s < net_buffer_end; s++)
		{
		  /* Move forward in the buffer until the first newline.
			 Stop when we see CRLF, CR, or LF, or the end of the buffer.
			 *But*, if we see a lone CR at the *very end* of the buffer,
			 treat this as if we had reached the end of the buffer without
			 seeing a line terminator.  This is to catch the case of the
			 buffers splitting a CRLF pair, as in "FOO\r\nBAR\r" "\nBAZ\r\n".
		   */
		  if (*s == CR || *s == LF)
			{
			  newline = s;
			  if (newline[0] == CR)
				{
				  if (s == net_buffer_end - 1)
					{
					  /* CR at end - wait for the next character. */
					  newline = 0;
					  break;
					}
				  else if (newline[1] == LF)
					/* CRLF seen; swallow both. */
					newline++;
				}
			  newline++;
			  break;
			}
		}

	  /* Ensure room in the net_buffer and append some or all of the current
		 chunk of data to it. */
	  {
		const char *end = (newline ? newline : net_buffer_end);
		PRUint32 desired_size = (end - net_buffer) + (*buffer_fpP) + 1;

		if (desired_size >= (*buffer_sizeP))
		  {
			status = msg_GrowBuffer (desired_size, sizeof(char), 1024,
									 bufferP, buffer_sizeP);
			if (status < 0) return status;
		  }
		nsCRT::memcpy((*bufferP) + (*buffer_fpP), net_buffer, (end - net_buffer));
		(*buffer_fpP) += (end - net_buffer);
	  }

	  /* Now *bufferP contains either a complete line, or as complete
		 a line as we have read so far.

		 If we have a line, process it, and then remove it from `*bufferP'.
		 Then go around the loop again, until we drain the incoming data.
	   */
	  if (!newline)
		return 0;

	  status = msg_convert_and_send_buffer(*bufferP, *buffer_fpP,
										   convert_newlines_p,
										   per_line_fn, closure);
	  if (status < 0) return status;

	  net_buffer_size -= (newline - net_buffer);
	  net_buffer = newline;
	  (*buffer_fpP) = 0;
	}
  return 0;
}

nsresult 
nsNNTPHost::LoadNewsrcFileAndCreateNewsgroups(nsFileSpec &newsrcFile)
{
	char *ibuffer = 0;
	PRUint32 ibuffer_size = 0;
	PRUint32 ibuffer_fp = 0;
  PRInt32 size = 1024;
  char *buffer;
  buffer = new char[size];
  nsresult rv = NS_OK;
  PRInt32 numread = 0;
  PRInt32 status = 0;

  PR_FREEIF(m_optionLines);

  if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

  nsInputFileStream inputStream(newsrcFile); 
  while (1) {
    numread = inputStream.read(buffer, size);
    if (numread == 0) {
      break;
    }
    else {
#ifdef DEBUG_NEWS
      printf("%d: %s\n", numread, buffer);
#endif
      status = msg_LineBuffer(buffer, numread,
                     &ibuffer, &ibuffer_size, &ibuffer_fp,
                     FALSE,
#ifdef XP_OS2
                     (nsresult (_Optlink*) (char*,PRUint32,void*))
#endif /* XP_OS2 */
                     nsNNTPHost::ProcessLine_s, this);
      if (numread <= 0) {
        break;
      }
    }
  }

  if (status == 0 && ibuffer_fp > 0) {
    rv = ProcessLine_s(ibuffer, ibuffer_fp, this);
    ibuffer_fp = 0;
  }

  inputStream.close();
  delete [] buffer;
  
  return rv;
}

nsresult
nsNNTPHost::ProcessLine_s(char* line, PRUint32 line_size, void* closure)
{
#ifdef DEBUG_NEWS
  printf("nsNNTPHost::ProcessLine_s()\n");
#endif
  return ((nsNNTPHost*) closure)->ProcessLine(line, line_size);
}

nsresult
nsNNTPHost::ProcessLine(char* line, PRUint32 line_size)
{
	/* guard against blank line lossage */
	if (line[0] == '#' || line[0] == CR || line[0] == LF) return NS_OK;

	line[line_size] = 0;

	if ((line[0] == 'o' || line[0] == 'O') &&
		!PL_strncasecmp (line, "options", 7)) {
		return RememberLine(line);
	}

	char *s;
	char *end = line + line_size;
	static nsMsgKeySet *set;
	
	for (s = line; s < end; s++)
		if (*s == ':' || *s == '!')
			break;
	
	if (*s == 0) {
		/* What is this?? Well, don't just throw it away... */
		return RememberLine(line);
	}

	set = nsMsgKeySet::Create(s + 1 /* , this */);
	if (!set) return NS_ERROR_OUT_OF_MEMORY;

	PRBool subscribed = (*s == ':');
	*s = '\0';

	if (PL_strlen(line) == 0)
	{
		delete set;
		return NS_OK;
	}

	nsCOMPtr <nsINNTPNewsgroup> newsgroup;
    nsresult rv= NS_ERROR_NOT_INITIALIZED;
    
	if (subscribed && IsCategoryContainer(line))
	{
#ifdef HAVE_FOLDERINFO
		info = new nsINNTPCategoryContainer(line, set, subscribed,
												   this,
												   m_hostinfo->GetDepth() + 1);
#endif
		nsMsgGroupRecord* group = FindOrCreateGroup(line);
		// Go add all of our categories to the newsrc.
		AssureAllDescendentsLoaded(group);
		nsMsgGroupRecord* endRecord = group->GetSiblingOrAncestorSibling();
		nsMsgGroupRecord* child;
		for (child = group->GetNextAlphabetic() ;
			 child != endRecord ;
			 child = child->GetNextAlphabetic()) {
			PR_ASSERT(child);
			if (!child) break;
			char* fullname = child->GetFullName();
			if (!fullname) break;

            rv = FindGroup(fullname, getter_AddRefs(newsgroup));

			if (NS_FAILED(rv)) {
                // autosubscribe, if we haven't seen this one.
                char* groupLine = PR_smprintf("%s:", fullname);
                if (groupLine) {
                    rv = ProcessLine(groupLine, PL_strlen(groupLine));
                    PR_Free(groupLine);
                }
            }
			delete [] fullname;
		}
	}
	else {
		rv = NS_OK;

        if (NS_SUCCEEDED(rv)) {
            rv = nsComponentManager::CreateInstance(kNNTPNewsgroupCID, nsnull, nsINNTPNewsgroup::GetIID(), getter_AddRefs(newsgroup));
            if (NS_FAILED(rv)) {
                return rv;
            }
            rv = newsgroup->Initialize(line, set, subscribed);
			if (NS_SUCCEEDED(rv)) {
				nsCOMPtr<nsINNTPNewsgroupList> newsgroupList;
                
                rv = nsComponentManager::CreateInstance(kNNTPNewsgroupListCID, nsnull, nsINNTPNewsgroupList::GetIID(), getter_AddRefs(newsgroupList));
           
				if (NS_SUCCEEDED(rv)) {
                    rv = newsgroupList->Initialize(this, newsgroup, line, m_hostname);
					//add newsgroupList to host's list of newsgroups
					if (m_newsgrouplists) 
						m_newsgrouplists->AppendElement(newsgroupList);
				}
			}
		}
    }

	if (NS_FAILED(rv) || !newsgroup) return NS_ERROR_OUT_OF_MEMORY;

	// for now, you can't subscribe to category by itself.
    nsINNTPCategory *category;
    
	if (NS_SUCCEEDED(newsgroup->QueryInterface(nsINNTPCategory::GetIID(),
                                          (void **)&category))) {
        
        nsIMsgFolder *folder = getFolderFor(newsgroup);
        if (folder) {
            //            m_hostinfo->AddSubFolder(folder);
            m_hostinfo->AppendElement(folder);
            NS_RELEASE(folder);
        }
        
        NS_RELEASE(category);
	}

	if (m_groups != nsnull) {
		m_groups->AppendElement(newsgroup);
	}
	else {
		printf("m_groups is null.\n");
	}

	// prime the folder info from the folder cache while it's still around.
	// Except this might disable the update of new counts - check it out...
#ifdef HAVE_MASTER
	m_master->InitFolderFromCache(newsgroup);
#endif

	return NS_OK;
}

nsresult
nsNNTPHost::RememberLine(char* line)
{
	char* new_data;
	if (m_optionLines) {
		new_data =
			(char *) PR_Realloc(m_optionLines,
								PL_strlen(m_optionLines)
								+ PL_strlen(line) + 4);
	} else {
		new_data = (char *) PR_Malloc(PL_strlen(line) + 3);
	}
	if (!new_data) 
		return NS_ERROR_OUT_OF_MEMORY;

	PL_strcpy(new_data, line);
	PL_strcat(new_data, MSG_LINEBREAK);

	m_optionLines = new_data;

	return NS_OK;
}


#ifdef USE_NEWSRC_MAP_FILE

#define NEWSRC_MAP_FILE_COOKIE "netscape-newsrc-map-file"

nsresult
nsNNTPHost::MapHostToNewsrcFile(char *newshostname, nsFileSpec &fatFile, nsFileSpec &newsrcFile)
{
  char *lookingFor = nsnull;
	char buffer[512];
	char psuedo_name[512];
	char filename[512];
	char is_newsgroup[512];
  PRBool rv;

#ifdef DEBUG_NEWS
  printf("MapHostToNewsrcFile(%s,%s,%s,??)\n",newshostname,(const char *)fatFile, newshostname);
#endif
  lookingFor = PR_smprintf("newsrc-%s",newshostname);
  if (lookingFor == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsInputFileStream inputStream(fatFile);
 
  if (inputStream.eof()) {
    newsrcFile = "";
    inputStream.close();
    PR_FREEIF(lookingFor);
    return NS_ERROR_FAILURE;
  }

  /* we expect the first line to be NEWSRC_MAP_FILE_COOKIE */
	rv = inputStream.readline(buffer, sizeof(buffer));
#ifdef DEBUG_NEWS
  printf("buffer = %s\n", buffer);
#endif
  if ((!rv) || (PL_strncmp(buffer, NEWSRC_MAP_FILE_COOKIE, PL_strlen(NEWSRC_MAP_FILE_COOKIE)))) {
    newsrcFile = "";
    inputStream.close();
    PR_FREEIF(lookingFor);
    return NS_ERROR_FAILURE;
  }   

	while (!inputStream.eof()) {
    char * p;
    PRInt32 i;

    rv = inputStream.readline(buffer, sizeof(buffer));
    if (!rv) {
      newsrcFile = "";
      inputStream.close();
      PR_FREEIF(lookingFor);
      return NS_ERROR_FAILURE;
    }  

#ifdef DEBUG_NEWS
    printf("buffer = %s\n", buffer);    
#endif
    
    /*
      This used to be scanf() call which would incorrectly
      parse long filenames with spaces in them.  - JRE
    */
    
    filename[0] = '\0';
    is_newsgroup[0]='\0';
    
    for (i = 0, p = buffer; *p && *p != '\t' && i < 500; p++, i++)
      psuedo_name[i] = *p;
    psuedo_name[i] = '\0';
    if (*p) 
      {
        for (i = 0, p++; *p && *p != '\t' && i < 500; p++, i++)
          filename[i] = *p;
        filename[i]='\0';
        if (*p) 
          {
            for (i = 0, p++; *p && *p != '\r' && i < 500; p++, i++)
              is_newsgroup[i] = *p;
            is_newsgroup[i]='\0';
          }
      }

		if(!PL_strncmp(is_newsgroup, "TRUE", 4)) {
#ifdef DEBUG_NEWS
      printf("is_newsgroups_file = TRUE\n");
#endif
    }
    else {
#ifdef DEBUG_NEWS
      printf("is_newsgroups_file = FALSE\n");
#endif
    }
    
#ifdef DEBUG_NEWS
    printf("psuedo_name=%s,filename=%s\n", psuedo_name, filename);
#endif
    if (!PL_strncmp(psuedo_name,lookingFor,PL_strlen(lookingFor))) {
#ifdef DEBUG_NEWS
      printf("found a match for %s\n",lookingFor);
#endif
      newsrcFile = filename;
      inputStream.close();
      PR_FREEIF(lookingFor);
      return NS_OK;
    }
  }

  // failed to find a match in the map file
  newsrcFile = "";
  inputStream.close();
  PR_FREEIF(lookingFor);
  return NS_ERROR_FAILURE;
}
#endif /* USE_NEWSRC_MAP_FILE */

nsresult 
nsNNTPHost::GetNewsrcFile(char *newshostname, nsFileSpec &path, nsFileSpec &newsrcFile)
{
  nsresult rv = NS_OK;
  
  if (newshostname == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

#ifdef USE_NEWSRC_MAP_FILE
  // the fat file lives in the same directory as
  // the newsrc files
  nsFileSpec fatFile(path);
  fatFile.SetLeafName(NEWS_FAT_FILE_NAME);

  rv = MapHostToNewsrcFile(newshostname, fatFile, newsrcFile);
#else
  char *str = nsnull;

  str = PR_smprintf(".newsrc-%s", newshostname);
  if (str == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  newsrcFile = path;
  newsrcFile.SetLeafName(str);
  PR_FREEIF(str);
  str = nsnull;
  rv = NS_OK;
#endif /* USE_NEWSRC_MAP_FILE */

  return rv;
}


nsresult nsNNTPHost::LoadNewsrc(const char *uri /* , nsIMsgFolder* hostinfo*/)
{
#ifdef DEBUG_NEWS
    printf("nsNNTPHost::LoadNewsrc(%s)\n", uri);
#endif

	nsresult rv = NS_OK;

    /*	m_hostinfo = hostinfo; 
	PR_ASSERT(m_hostinfo);
	if (!m_hostinfo) return -1;
    */
	if (!m_newsgrouplists) {
		rv = NS_NewISupportsArray(&m_newsgrouplists);
		if (NS_FAILED(rv) || (m_newsgrouplists == nsnull)) {
			return NS_ERROR_OUT_OF_MEMORY;
		}
	}

	if (!m_groups) {

		rv = NS_NewISupportsArray(&m_groups);
		if (NS_FAILED(rv) || (m_groups == nsnull)) {
			return NS_ERROR_OUT_OF_MEMORY;
		}

	    nsFileSpec newsrcFile("");
		nsFileSpec path("");

		// turn uri into path
		rv = nsNewsURI2Path(kNewsRootURI, uri, path);
		if (NS_FAILED(rv)) {
			return rv;
		}

		rv = GetNewsrcFile(m_hostname, path, newsrcFile);
		if (NS_FAILED(rv)) {
			return rv;
		}

#ifdef DEBUG_NEWS
		printf("newsrc file = %s\n",(const char *)newsrcFile);
#endif	
		rv = LoadNewsrcFileAndCreateNewsgroups(newsrcFile);
		if (NS_FAILED(rv)) {
			return rv;
		}
	}

#if 0
	// build up the category tree for each category container so that roll-up 
	// of counts will work before category containers are opened.
	for (PRInt32 i = 0; i < m_groups->Count(); i++) {
        nsresult rv;
        
		nsINNTPNewsgroup *newsgroup = (nsINNTPNewsgroup *) (*m_groups)[i];
        
        nsINNTPCategoryContainer *catContainer;
        rv = newsgroup->QueryInterface(nsINNTPCategoryContainer::GetIID(),
                                       (void **)&catContainer);
		if (NS_SUCCEEDED(rv)) {
              
			char* name;
            nsresult rv = newsgroup->GetName(&name);
            
			nsMsgGroupRecord* group =
                m_groupTree->FindDescendant(name);
            
			PR_ASSERT(NS_SUCCEEDED(rv) && group);
            
			if (NS_SUCCEEDED(rv) && group) {
                nsIMsgFolder *folder = getFolderFor(newsgroup);
				folder->SetFlag(MSG_FOLDER_FLAG_ELIDED |
                                MSG_FOLDER_FLAG_DIRECTORY);
                NS_RELEASE(folder);
#ifdef HAVE_MASTER
				catContainer->BuildCategoryTree(catContainer, name, group,
					2, m_master);
#endif
			}
            NS_RELEASE(catContainer);
		}
	}
#endif /* 0 */

	return NS_OK;
}


nsresult
nsNNTPHost::WriteNewsrc()
{
	if (!m_groups) return NS_ERROR_NOT_INITIALIZED;
#ifdef UNREADY_CODE
	PR_ASSERT(m_dirty);
	// Just to be sure.  It's safest to go ahead and write it out anyway,
	// even if we do somehow get called without the dirty bit set.

	XP_File fid=0;
    
    fid = XP_FileOpen(GetNewsrcFileName(), xpTemporaryNewsRC,
                      XP_FILE_WRITE_BIN);
    
	if (!fid) return MK_UNABLE_TO_OPEN_NEWSRC;
	
	PRInt32 status = 0;

#ifdef XP_UNIX
	/* Clone the permissions of the "real" newsrc file into the temp file,
	   so that when we rename the finished temp file to the real file, the
	   preferences don't appear to change. */
	{
        XP_StatStruct st;

        if (XP_Stat (GetNewsrcFileName(), &st, xpNewsRC) == 0)
			/* Ignore errors; if it fails, bummer. */

		/* SCO doesn't define fchmod at all.  no big deal.
		   AIX3, however, defines it to take a char* as the first parameter.  The
		   man page says it takes an int, though.  ... */
#if defined( SCO_SV ) || defined ( AIXV3 )
	  {
	    		char *really_tmp_file = WH_FileName(GetNewsrcFileName(), xpTemporaryNewsRC);

			chmod  (really_tmp_file, (st.st_mode | S_IRUSR | S_IWUSR));

			PR_Free(really_tmp_file);
	  }
#else
			fchmod (fileno(fid), (st.st_mode | S_IRUSR | S_IWUSR));
#endif
	}
#endif /* XP_UNIX */

	if (m_optionLines) {
		status = XP_FileWrite(m_optionLines, PL_strlen(m_optionLines), fid);
		if (status < ((PRInt32)PL_strlen(m_optionLines))) {
			status = MK_MIME_ERROR_WRITING_FILE;
		}
	}

	PRInt32 n = m_groups->Count();
	for (PRInt32 i=0 ; i<n && status >= 0 ; i++) {
        nsresult rv;
		nsINNTPNewsgroup* newsgroup = (nsINNTPNewsgroup*) ((*m_groups)[i]);
		// GetNewsFolderInfo will get root category for cat container.
#ifdef HAVE_FOLDERINFO
		char* str = newsgroup->GetNewsFolderInfo()->GetSet()->Output();
#else
        char *str = nsnull;
#endif
		if (!str) {
			status = MK_OUT_OF_MEMORY;
			break;
		}
        char *name=nsnull;
        char *line;
        rv = newsgroup->GetName(&name);
        
        PRBool isSubscribed=PR_FALSE;
        rv = newsgroup->GetSubscribed(&isSubscribed);
        line = PR_smprintf("%s%s %s" MSG_LINEBREAK,
                                 name, 
								 isSubscribed ? ":" : "!", str);
		if (!line) {
			delete [] str;
			status = MK_OUT_OF_MEMORY;
			break;
		}
		PRInt32 length = PL_strlen(line);
		status = XP_FileWrite(line, length, fid);
		if (status < length) {
			status = MK_MIME_ERROR_WRITING_FILE;
		}
		delete [] str;
		PR_Free(line);
	}
	  
	XP_FileClose(fid);
  
	if (status >= 0) {
        if (XP_FileRename(GetNewsrcFileName(), xpTemporaryNewsRC,
                          GetNewsrcFileName(), xpNewsRC) < 0)
            status = MK_MIME_ERROR_WRITING_FILE;
	}

	if (status < 0) {
        
		XP_FileRemove(GetNewsrcFileName(), xpTemporaryNewsRC);
		return status;
	}
	m_dirty = PR_FALSE;
	if (m_writetimer) {
#ifdef UNREADY_CODE
		FE_ClearTimeout(m_writetimer);
#endif
		m_writetimer = nsnull;
	}

	if (m_groupTreeDirty) SaveHostInfo();

	return status;
#else
	return 0;
#endif
}


nsresult
nsNNTPHost::WriteIfDirty()
{
	if (m_dirty) return WriteNewsrc();
	return 0;
}


nsresult
nsNNTPHost::MarkDirty()
{
	m_dirty = PR_TRUE;
#ifdef UNREADY_CODE
	if (!m_writetimer)
		m_writetimer = FE_SetTimeout((TimeoutCallbackFunction)nsNNTPHost::WriteTimer, this,
								 5L * 60L * 1000L); // Hard-coded -- 5 minutes.
#endif
    return NS_OK;
}

void
nsNNTPHost::WriteTimer(void* closure)
{
	nsNNTPHost* host = (nsNNTPHost*) closure;
	host->m_writetimer = nsnull;
	if (host->WriteNewsrc() < 0) {
		// ###tw  Pop up error message???
		host->MarkDirty();		// Cause us to try again. Or is this bad? ###tw
	}
}


nsresult
nsNNTPHost::SetNewsRCFilename(char* name)
{
	delete [] m_filename;
	m_filename = new char [PL_strlen(name) + 1];
	if (!m_filename) return NS_ERROR_OUT_OF_MEMORY;
	PL_strcpy(m_filename, name);

	m_hostinfofilename = PR_smprintf("%s/hostinfo.dat", GetDBDirName());
	if (!m_hostinfofilename) return NS_ERROR_OUT_OF_MEMORY;
#ifdef UNREADY_CODE
	XP_StatStruct st;
	if (XP_Stat (m_hostinfofilename, &st, xpXoverCache) == 0) {
		m_fileSize = st.st_size;
	}
	                               
	if (m_groupFile) {
		XP_FileClose(m_groupFile);
		m_groupFile = nsnull;
	}

	OpenGroupFile();

	if (!m_groupFile) {
		// It does not exist.  Create an empty one.  But first, we go through
		// the whole directory and blow away anything that does not end in
		// ".snm".  This is to take care of people running B1 code, which
		// kept databases with such filenames.
		PRInt32 lastcount = -1;
		PRInt32 count = -1;
		do {
			XP_Dir dir = XP_OpenDir(GetDBDirName(), xpXoverCache);
			if (!dir) break;
			lastcount = count;
			count = 0;
			XP_DirEntryStruct *entry = nsnull;
			for (entry = XP_ReadDir(dir); entry; entry = XP_ReadDir(dir)) {
				count++;
				const char* name = entry->d_name;
				PRInt32 length = PL_strlen(name);
				if (name[0] == '.' || name[0] == '#' ||
					name[length - 1] == '~') continue;
				if (length < 4 ||
					PL_strcasecmp(name + length - 4, ".snm") != 0) {
					char* tmp = PR_smprintf("%s/%s", GetDBDirName(), name);
					if (tmp) {
						XP_FileRemove(tmp, xpXoverCache);
						PR_Free(tmp);
					}
				}
			}
			XP_CloseDir(dir);
		} while (lastcount != count); // Keep doing it over and over, until
									  // we get the same number of entries in
									  // the dir.  This is because I do not
									  // trust XP_ReadDir() to do the right
									  // thing if I delete files out from
									  // under it.

		// OK, now create the new empty hostinfo file.
		OpenGroupFile(XP_FILE_WRITE_BIN);
		PR_ASSERT(m_groupFile);
		if (!m_groupFile)
			return NS_ERROR_NOT_INITIALIZED;
		OpenGroupFile();
		PR_ASSERT(m_groupFile);
		if (!m_groupFile)
			return NS_ERROR_NOT_INITIALIZED;

		m_groupTreeDirty = 2;
	}

	m_blockSize = 10240;			// ###tw  This really ought to be the most
								// efficient file reading size for the current
								// operating system.

	m_block = nsnull;
	while (!m_block && (m_blockSize >= 512))
	{
		m_block = new char[m_blockSize + 1];
		if (!m_block)
			m_blockSize /= 2;
	}
	if (!m_block)
		return MK_OUT_OF_MEMORY;

	m_groupTree = nsMsgGroupRecord::Create(nsnull, nsnull, 0, 0, 0);
	if (!m_groupTree)
		return MK_OUT_OF_MEMORY;

	return ReadInitialPart();
#else
	return NS_OK;
#endif
}


PRInt32
nsNNTPHost::ReadInitialPart()
{
#ifdef UNREADY_CODE
	OpenGroupFile();
	if (!m_groupFile) return -1;

	PRInt32 length = XP_FileRead(m_block, m_blockSize, m_groupFile);
	if (length < 0) length = 0;
	m_block[length] = '\0';
	PRInt32 version = 0;

	char* ptr;
	char* endptr = nsnull;
	for (ptr = m_block ; *ptr ; ptr = endptr + 1) {
		while (*ptr == CR || *ptr == LF) ptr++;
		endptr = PL_strchr(ptr, LINEBREAK_START);
		if (!endptr) break;
		*endptr = '\0';
		if (ptr[0] == '#' || ptr[0] == '\0') {
			continue;			// Skip blank lines and comments.
		}
		if (PL_strcmp(ptr, "begingroups") == 0) {
			m_fileStart = endptr - m_block;
			break;
		}
		char* ptr2 = PL_strchr(ptr, '=');
		if (!ptr2) continue;
		*ptr2++ = '\0';
		if (PL_strcmp(ptr, "lastgroupdate") == 0) {
			m_lastGroupUpdate = strtol(ptr2, nsnull, 16);
		} else if (PL_strcmp(ptr, "firstnewdate") == 0) {
			m_firstnewdate = strtol(ptr2, nsnull, 16);
		} else if (PL_strcmp(ptr, "uniqueid") == 0) {
			m_uniqueId = strtol(ptr2, nsnull, 16);
		} else if (PL_strcmp(ptr, "pushauth") == 0) {
			m_pushAuth = strtol(ptr2, nsnull, 16);
		} else if (PL_strcmp(ptr, "version") == 0) {
			version = strtol(ptr2, nsnull, 16);
		}
	}
	m_block[0] = '\0';
	if (version != 1) {
		// The file got clobbered or damaged somehow.  Throw it away.
#ifdef DEBUG_bienvenu
		if (length > 0)
			PR_ASSERT(PR_FALSE);	// this really shouldn't happen, right?
#endif
		OpenGroupFile(XP_FILE_WRITE_BIN);
		PR_ASSERT(m_groupFile);
		if (!m_groupFile) return -1;
		OpenGroupFile();
		PR_ASSERT(m_groupFile);
		if (!m_groupFile) return -1;

		m_groupTreeDirty = 2;
		m_fileStart = 0;
		m_fileSize = 0;
	}
	m_groupTree->SetFileOffset(m_fileStart);
#endif
	return 0;
}

PRInt32
nsNNTPHost::CreateFileHeader()
{
	PRInt32 firstnewdate;

	LL_L2I(firstnewdate, m_firstnewdate);
	
	PR_snprintf(m_block, m_blockSize,
				"# Netscape newshost information file." MSG_LINEBREAK
				"# This is a generated file!  Do not edit." MSG_LINEBREAK
				"" MSG_LINEBREAK
				"version=1" MSG_LINEBREAK
				"newsrcname=%s" MSG_LINEBREAK
#ifdef OSF1
				"lastgroupdate=%08x" MSG_LINEBREAK
				"firstnewdate=%08x" MSG_LINEBREAK
				"uniqueid=%08x" MSG_LINEBREAK
#else
				"lastgroupdate=%08lx" MSG_LINEBREAK
				"firstnewdate=%08lx" MSG_LINEBREAK
				"uniqueid=%08lx" MSG_LINEBREAK
#endif
				"pushauth=%1x" MSG_LINEBREAK
				"" MSG_LINEBREAK
				"begingroups",
				m_filename, m_lastGroupUpdate, firstnewdate, m_uniqueId,
				m_pushAuth);
	return PL_strlen(m_block);

}

PRInt32 
nsNNTPHost::SaveHostInfo()
{                   
	PRInt32 status = 0;
#ifdef UNREADY_CODE
	nsMsgGroupRecord* grec;
	XP_File in = nsnull;
	XP_File out = nsnull;
	char* blockcomma = nsnull;
	char* ptrcomma = nsnull;
	char* filename = nsnull;
	PRInt32 length = CreateFileHeader();
	PR_ASSERT(length < m_blockSize - 50);
	if (m_inhaled || length != m_fileStart) {
		m_groupTreeDirty = 2;
	}
	if (m_groupTreeDirty < 0) {
		// Uh, somehow we set the sign bit, probably some routine returned an
		// error status.  This will probably never happen.  But if it does,
		// we should just make sure to write things out in the most paranoid
		// fashion.
		m_groupTreeDirty = 2;
	}
	if (m_groupTreeDirty < 2) {
		// Only bits and pieces are dirty.  Write them out without writing the
		// whole file.
		OpenGroupFile();
		if (!m_groupFile) {
            status = MK_UNABLE_TO_OPEN_NEWSRC;
            goto FAIL;
		}
		XP_FileSeek(m_groupFile, 0, SEEK_SET);
		XP_FileWrite(m_block, length, m_groupFile);
		char* ptr = nsnull;
		for (grec = m_groupTree->GetChildren();
			 grec;
			 grec = grec->GetNextAlphabetic()) {
			if (grec->IsDirty()) {
				if (grec->GetFileOffset() < m_fileStart) {
					m_groupTreeDirty = 2;
					break;
				}
				ptr = grec->GetSaveString();
				if (!ptr) {
					status = MK_OUT_OF_MEMORY;
					goto FAIL;
				}
				length = PL_strlen(ptr);
				if (length >= m_blockSize) {
					char* tmp = new char [length + 512];
					if (!tmp) {
						status = MK_OUT_OF_MEMORY;
						goto FAIL;
					}
					delete [] m_block;
					m_block = tmp;
					m_blockSize = length + 512;
				}
				// Read the old data, and make sure the old line was the
				// same length as the new one.  If not, set the dirty flag
				// to 2, so we end up writing the whole file, below.
				XP_FileSeek(m_groupFile, grec->GetFileOffset(), SEEK_SET);
				PRInt32 l = XP_FileRead(m_block, length, m_groupFile);
				if (l != length || m_block[length - 1] != ptr[length - 1]) {
					m_groupTreeDirty = 2;
					break;
				}
				m_block[l] = '\0';
				char* p1 = PL_strchr(ptr, LINEBREAK_START);
				char* p2 = PL_strchr(m_block, LINEBREAK_START);
				PR_ASSERT(p1);
				if (!p1 || !p2 || (p1 - ptr) != (p2 - m_block)) {
					m_groupTreeDirty = 2;
					break;
				}
				PR_ASSERT(grec->GetFileOffset() > 100);	// header is at least 100 bytes long
				XP_FileSeek(m_groupFile, grec->GetFileOffset(), SEEK_SET);
				XP_FileWrite(ptr, PL_strlen(ptr), m_groupFile);
				PR_Free(ptr);
				ptr = nsnull;
			}
		}
		PR_FREEIF(ptr);
	}
	if (m_groupTreeDirty >= 2) {
		// We need to rewrite the whole silly file.
		filename = PR_smprintf("%s/tempinfo", GetDBDirName());
		if (!filename) return MK_OUT_OF_MEMORY;
		PRInt32 position = 0;

		if (m_groupFile)
		{
			XP_FileClose(m_groupFile);
			m_groupFile = nsnull;
		}

		out = XP_FileOpen(filename, xpXoverCache, XP_FILE_WRITE_BIN);
		if (!out) return MK_MIME_ERROR_WRITING_FILE;

		length = CreateFileHeader();	// someone probably has hosed m_block - so regen
		status = XP_FileWrite(m_block, length, out);
		if (status < 0) goto FAIL;
		position += length;
		m_fileStart = position;
		m_groupTree->SetFileOffset(m_fileStart);
		status = XP_FileWrite(MSG_LINEBREAK, MSG_LINEBREAK_LEN, out);
		if (status < 0) goto FAIL;
		position += MSG_LINEBREAK_LEN;

		blockcomma = nsnull;
		if (!m_inhaled) {
			in = XP_FileOpen(m_hostinfofilename, xpXoverCache, XP_FILE_READ);
			if (!in) {
				status = MK_MIME_ERROR_WRITING_FILE;
				goto FAIL;
			}
			do {
				m_block[0] = '\0';
				XP_FileReadLine(m_block, m_blockSize, in);
			} while (m_block[0] &&
					 PL_strncmp(m_block, "begingroups", 11) != 0);
			m_block[0] = '\0';
			XP_FileReadLine(m_block, m_blockSize, in);
			blockcomma = PL_strchr(m_block, ',');
			if (blockcomma) *blockcomma = '\0';
		}
		for (grec = m_groupTree->GetChildren();
			 grec;
			 grec = grec->GetNextAlphabetic()) {
			char* ptr = grec->GetSaveString();
			if (!ptr) {
				status = MK_OUT_OF_MEMORY;
				goto FAIL;
			}
			ptrcomma = PL_strchr(ptr, ',');
			PR_ASSERT(ptrcomma);
			if (!ptrcomma) {
				status = -1;
				goto FAIL;
			}
			*ptrcomma = '\0';
			while (blockcomma &&
				   nsMsgGroupRecord::GroupNameCompare(m_block, ptr) < 0) {
				*blockcomma = ',';
				XP_StripLine(m_block);
				length = PL_strlen(m_block);
				status = XP_FileWrite(m_block, length, out);
				if (status < 0) goto FAIL;
				position += length;
				status = XP_FileWrite(MSG_LINEBREAK, MSG_LINEBREAK_LEN, out);
				if (status < 0) goto FAIL;
				position += MSG_LINEBREAK_LEN;
				m_block[0] = '\0';
				XP_FileReadLine(m_block, m_blockSize, in);
				blockcomma = PL_strchr(m_block, ',');
				if (blockcomma) *blockcomma = '\0';
			}
			if (blockcomma && PL_strcmp(m_block, ptr) == 0) {
				m_block[0] = '\0';
				XP_FileReadLine(m_block, m_blockSize, in);
				blockcomma = PL_strchr(m_block, ',');
				if (blockcomma) *blockcomma = '\0';
			}
			grec->SetFileOffset(position);
			*ptrcomma = ',';
			length = PL_strlen(ptr);
			// if the group doesn't exist on the server and has no children,
			// don't write it out.
			if (!grec->DoesNotExistOnServer()  || grec->GetNumKids() > 0)
			{
				status = XP_FileWrite(ptr, length, out);
				position += length;
			}
			else
				status = 0;
			PR_Free(ptr);
			if (status < 0) goto FAIL;
		}
		if (blockcomma) {
			*blockcomma = ',';
			do {
				XP_StripLine(m_block);
				length = PL_strlen(m_block);
				status = XP_FileWrite(m_block, length, out);
				if (status < 0) goto FAIL;
				position += length;
				status = XP_FileWrite(MSG_LINEBREAK, MSG_LINEBREAK_LEN, out);
				if (status < 0) goto FAIL;
				position += MSG_LINEBREAK_LEN;
				m_block[0] = '\0';
			} while (XP_FileReadLine(m_block, m_blockSize, in) && *m_block);
		}
		if (in) {
			XP_FileClose(in);
			in = nsnull;
		}
		XP_FileClose(out);
		out = nsnull;
		XP_FileRename(filename, xpXoverCache,
					  m_hostinfofilename, xpXoverCache);
		m_fileSize = position;
	}
	m_groupTreeDirty = 0;

FAIL:
	if (in) XP_FileClose(in);
	if (out) XP_FileClose(out);
	if (filename) PR_Free(filename);
	m_block[0] = '\0';
#endif
	return status;
}


struct InhaleState {
	nsMsgGroupRecord* tree;
	PRInt32 position;
	nsMsgGroupRecord* onlyIfChild;
	nsMsgGroupRecord* lastInhaled;
	char lastfullname[512];
};


PRInt32
nsNNTPHost::InhaleLine(char* line, PRUint32 length, void* closure)
{
	PRInt32 status = 0;
	InhaleState* state = (InhaleState*) closure;
	PRInt32 position = state->position;
	state->position += length;
	char* lastdot;
	char* endptr = line + length;
	char* comma;
	for (comma = line ; comma < endptr ; comma++) {
		if (*comma == ',') break;
	}
	if (comma >= endptr) return 0;
	*comma = '\0';
	lastdot = PL_strrchr(line, '.');
	nsMsgGroupRecord* parent;
	nsMsgGroupRecord* child;
	if (lastdot) {
		*lastdot = '\0';
		// Try to find the parent of this line.  Very often, it will be
		// the last line we read, or some ancestor of that line.
		parent = nsnull;
		if (state->lastInhaled && PL_strncmp(line, state->lastfullname,
											 lastdot - line) == 0) {
			char c = state->lastfullname[lastdot - line];
			if (c == '\0') parent = state->lastInhaled;
			else if (c == '.') {
				parent = state->lastInhaled;
				char* ptr = state->lastfullname + (lastdot - line);
				PR_ASSERT(parent);
				while (parent && ptr) {
					parent = parent->GetParent();
					PR_ASSERT(parent);
					ptr = PL_strchr(ptr + 1, '.');
				}
			}
		}


		if (!parent) parent = state->tree->FindDescendant(line);
		*lastdot = '.';
		PR_ASSERT(parent);
		if (!parent) {
			status = -1;
			goto DONE;
		}
	} else {
		parent = state->tree;
		lastdot = line - 1;
	}
	child = parent->FindDescendant(lastdot + 1);
	*comma = ',';
	if (child) {
		// It's already in memory.
		child->SetFileOffset(position);
	} else {
		child = nsMsgGroupRecord::Create(parent, line, endptr - line, position);
		if (!child) {
			status = MK_OUT_OF_MEMORY;
			goto DONE;
		}
	}
	if (state->onlyIfChild) {
		nsMsgGroupRecord* tmp;
		for (tmp = child; tmp ; tmp = tmp->GetParent()) {
			if (tmp == state->onlyIfChild) break;
		}
		if (tmp == nsnull) status = -2;	// Indicates we're done.
	}
	PR_ASSERT(comma - line < ((PRInt32)sizeof(state->lastfullname)));
	if ((comma - line)/sizeof(char) < sizeof(state->lastfullname)) {
		PL_strncpyz(state->lastfullname, line, comma - line + 1);
		state->lastfullname[comma - line] = '\0';
		state->lastInhaled = child;
	}
DONE:
	if (comma) *comma = ',';
	return status;
}


PRInt32
nsNNTPHost::Inhale(PRBool force)
{
#ifdef UNREADY_CODE
	if (m_groupTreeDirty) SaveHostInfo();
	if (force) {
		while (m_groupTree->GetChildren()) {
			delete m_groupTree->GetChildren();
		}
		m_inhaled = PR_FALSE;
	}
	PR_ASSERT(!m_inhaled);
	if (m_inhaled) return -1;
	PRInt32 status = 0;
	OpenGroupFile();
	if (!m_groupFile) return -1;
	XP_FileSeek(m_groupFile, 0, SEEK_SET);
	status = ReadInitialPart();
	if (status < 0) return status;
	XP_FileSeek(m_groupFile, m_fileStart, SEEK_SET);
	InhaleState state;
	state.tree = m_groupTree;
	state.position = m_fileStart;
	state.onlyIfChild = nsnull;
	state.lastInhaled = nsnull;
	char* buffer = nsnull;
	PRUint32 buffer_size = 0;
	PRUint32 buffer_fp = 0;
	do {
		status = XP_FileRead(m_block, m_blockSize, m_groupFile);
		if (status <= 0) break;
		status = msg_LineBuffer(m_block, status,
								&buffer, &buffer_size, &buffer_fp,
								PR_FALSE, 
#ifdef XP_OS2
								(nsresult (_Optlink*) (char*,PRUint32,void*))
#endif
								nsNNTPHost::InhaleLine, &state);
	} while (status >= 0);
	if (status >= 0 && buffer_fp > 0) {
		status = InhaleLine(buffer, buffer_fp, &state);
	}
	PR_FREEIF(buffer);
	m_block[0] = '\0';
	if (status >= 0) m_inhaled = PR_TRUE;
	return status;
#else
	return 0;
#endif
}


PRInt32
nsNNTPHost::Exhale()
{
	PR_ASSERT(m_inhaled);
	if (!m_inhaled) return -1;
	PRInt32 status = SaveHostInfo();
	while (m_groupTree->GetChildren()) {
		delete m_groupTree->GetChildren();
	}
	m_inhaled = PR_FALSE;
	return status;
}


PRInt32
nsNNTPHost::EmptyInhale()
{
	PR_ASSERT(!m_inhaled);
	if (m_inhaled) return -1;
	while (m_groupTree->GetChildren()) {
		delete m_groupTree->GetChildren();
	}
	m_inhaled = PR_TRUE;
	return 0;
}


nsresult
nsNNTPHost::FindGroup(const char* name, nsINNTPNewsgroup* *_retval)
{
    nsresult result = NS_ERROR_NOT_INITIALIZED;

	if (name == nsnull) {
		return NS_ERROR_NULL_POINTER;
	}
#ifdef DEBUG_NEWS
		printf("FindGroup(%s)\n",name);
#endif
			
	if (m_groups == nsnull) return result;
	PRUint32 cnt;
    nsresult rv = m_groups->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    PRInt32 n = cnt;
	for (PRInt32 i=0 ; i<n ; i++) {
        char *groupname = nsnull;
        rv = NS_OK;
        
		nsINNTPNewsgroup* info = (nsINNTPNewsgroup*) m_groups->ElementAt(i);
        rv = info->GetName(&groupname);
       
#ifdef DEBUG_NEWS
		printf("%d = %s\n",i,groupname?groupname:"null");
#endif

		if (NS_SUCCEEDED(rv) && (name != nsnull) && PL_strcmp(name, groupname) == 0) {
			*_retval = info;
            result = NS_OK;
		}
	}
    return result;
}

nsresult
nsNNTPHost::AddGroup(const char *name,
                     nsINNTPNewsgroup **_retval)
{
    return AddGroup(name, nsnull, _retval);
}


nsresult
nsNNTPHost::AddGroup(const char *name,
                     nsMsgGroupRecord *inGroupRecord,
                     nsINNTPNewsgroup **_retval)
{
#ifdef DEBUG_NEWS
	printf("nsNNTPHost::AddGroup(%s)\n",name);
#endif
	nsINNTPNewsgroup *newsInfo = nsnull;
	nsINNTPCategoryContainer *categoryContainer = nsnull;
	char* containerName = nsnull;
	PRBool needpaneupdate = PR_FALSE;
    PRBool isSubscribed=FALSE;
    
	nsMsgGroupRecord* group = (inGroupRecord) ? inGroupRecord : FindOrCreateGroup(name);
	if (!group) goto DONE;	// Out of memory.

	if (!group->IsCategoryContainer() && group->IsCategory()) {
		nsMsgGroupRecord *container = group->GetCategoryContainer();
		PR_ASSERT(container);
		if (!container) goto DONE;
		containerName = container->GetFullName();
		if (!containerName) goto DONE; // Out of memory.
        
        nsresult rv = FindGroup(containerName, &newsInfo);
        
		if (NS_SUCCEEDED(rv)) {
            rv = newsInfo->QueryInterface(nsINNTPCategoryContainer::GetIID(),
                                          (void **)&categoryContainer);
            
            // if we're not subscribed to container, do that instead.
            if (NS_SUCCEEDED(rv))
                rv = newsInfo->GetSubscribed(&isSubscribed);
            
            if (NS_FAILED(rv) || !isSubscribed)
                {
                    name = containerName;
                    group = FindOrCreateGroup(name);
                }
            NS_RELEASE(newsInfo);
            // categoryContainer released at the end of this function
            // (yuck)
        }
	}

	// no need to diddle folder pane for new categories.
	needpaneupdate = m_hostinfo && !group->IsCategory();

    nsresult rv;
    rv = FindGroup(name, &newsInfo);
	if (NS_SUCCEEDED(rv)) {	// seems to be already added
        PRBool subscribed;
        rv = newsInfo->GetSubscribed(&subscribed);
		if (NS_SUCCEEDED(rv) && !subscribed) {
			newsInfo->SetSubscribed(PR_TRUE);
            nsIMsgFolder *newsFolder = getFolderFor(newsInfo);
            //            m_hostinfo->AddSubfolderIfUnique(newsFolder);
            m_hostinfo->AppendElement(newsFolder);
            /*
            nsISupportsArray* infolist;
            nsresult rv = m_hostinfo->GetSubFolders(&infolist);
			// don't add it if it's already in the list!
			if (infolist->FindIndex(0, newsInfo) == -1)
				infolist->Add(newsInfo);
            */
		} else {
			goto DONE;
		}
	} else {
		char* groupLine = PR_smprintf("%s:", name);
		if (!groupLine) {
			goto DONE;			// Out of memory.
		}
	
		// this will add and auto-subscribe - OK, a cheap hack.
		if (ProcessLine(groupLine, PL_strlen(groupLine)) == 0) {

			if (m_groups != nsnull) {
				// groups are added at end so look there first...
                PRUint32 cnt;
                rv = m_groups->Count(&cnt);
                if (NS_FAILED(rv)) return rv;
				newsInfo = (nsINNTPNewsgroup *)
                    m_groups->ElementAt(cnt - 1);
			}
			
			if (!newsInfo) goto DONE;

            char *name;
            rv = newsInfo->GetName(&name);
			if (NS_SUCCEEDED(rv) &&
                PL_strcmp(name, name)) {
				rv = FindGroup(name, &newsInfo);
				PR_ASSERT(NS_SUCCEEDED(rv));
			}
		}
		PR_Free(groupLine);
	}
	PR_ASSERT(newsInfo);
	if (!newsInfo) goto DONE;

	if (group->IsCategoryContainer()) {
		// Go add all of our categories to the newsrc.
		AssureAllDescendentsLoaded(group);
		nsMsgGroupRecord* end = group->GetSiblingOrAncestorSibling();
		nsMsgGroupRecord* child;
		for (child = group->GetNextAlphabetic() ;
			 child != end ;
			 child = child->GetNextAlphabetic()) {
			PR_ASSERT(child);
			if (!child) break;
			char* fullname = child->GetFullName();
			if (!fullname) break;
            
			nsINNTPNewsgroup* info;
            
            rv = FindGroup(fullname, &info);
			if (NS_SUCCEEDED(rv)) {
                PRBool subscribed;
                rv = info->GetSubscribed(&subscribed);
				if (NS_SUCCEEDED(rv) && !subscribed) {
					info->SetSubscribed(PR_TRUE);
				}
			} else {
				char* groupLine = PR_smprintf("%s:", fullname);
				if (groupLine) {
					ProcessLine(groupLine, PL_strlen(groupLine));
					PR_Free(groupLine);
				}
			}
			delete [] fullname;
		}

        // XXX ACK. This is hairy code. Fix this.
        // solution: SwitchNewsToCategoryContainer should
        // return an nsresult
        nsINNTPCategoryContainer *catContainer;
        rv =
            newsInfo->QueryInterface(nsINNTPCategoryContainer::GetIID(),
                                     (void **)&catContainer);
		if (NS_FAILED(rv))
		{
			catContainer = SwitchNewsToCategoryContainer(newsInfo);
            if (catContainer) rv = NS_OK;
		}
		PR_ASSERT(NS_SUCCEEDED(rv));
		if (NS_SUCCEEDED(rv)) {
            // XXX should this call be on catContainer or newsInfo?
            nsIMsgFolder *folder = getFolderFor(newsInfo);
			folder->SetFlag(MSG_FOLDER_FLAG_ELIDED);
            NS_IF_RELEASE(folder);
#ifdef HAVE_MASTER
			catContainer->BuildCategoryTree(catContainer, name, group,
											);
#endif
            NS_RELEASE(catContainer);
		}
	}
	else if (group->IsCategory() && categoryContainer)
	{
#ifdef HAVE_MASTER
		categoryContainer->AddToCategoryTree(categoryContainer, name, group, m_master);
#endif
	}

#ifdef HAVE_MASTER
	if (needpaneupdate && newsInfo)
		m_master->BroadcastFolderAdded (newsInfo);
#endif
    
	MarkDirty();

DONE:

    NS_IF_RELEASE(categoryContainer);

	if (containerName) delete [] containerName;
    if (_retval) *_retval = newsInfo;
    return NS_OK;
}

nsINNTPCategoryContainer *
nsNNTPHost::SwitchNewsToCategoryContainer(nsINNTPNewsgroup *newsInfo)
{
    nsresult rv;
	PRInt32 groupIndex = m_groups->IndexOf(newsInfo);
	if (groupIndex != -1)
	{
        // create a category container to hold this newsgroup
		nsINNTPCategoryContainer *newCatCont = nsnull;
        // formerly newsInfo->CloneIntoCategoryContainer();
#if 0                           // not implemented yet
        rv = nsComponentManager::CreateInstance(kNNTPCategoryContainerCID, nsnull, nsINNTPCategoryContainer::GetIID(), getter_AddRefs(newCatCont));
        if (NS_FAILED(rv)) return nsnull;
        
        rv = newCatCont->Initialize(newsInfo);
        if (NS_FAILED(rv)) return nsnull;
#endif
        
		// slip the category container where the newsInfo was.
		m_groups->ReplaceElementAt(newCatCont, groupIndex);

        nsIMsgFolder *newsFolder = getFolderFor(newsInfo);

        rv = newsInfo->QueryInterface(nsIMsgFolder::GetIID(),
                                      (void **)&newsFolder);
        if (newsFolder) {
            nsIMsgFolder *catContFolder = getFolderFor(newCatCont);
            if (catContFolder) {
                m_hostinfo->ReplaceElement(newsFolder, catContFolder);
                NS_RELEASE(catContFolder);
            }
            NS_RELEASE(newsFolder);
        }
        
        /*
        nsISupportsArray* infolist;
        nsresult rv = m_hostinfo->GetSubFolders(&infolist);
		// replace in folder pane server list as well.
		groupIndex = infoList->FindIndex(0, newsInfo);
		if (groupIndex != -1)
			infoList->SetAt(groupIndex, newCatCont);
        */
        return newCatCont;
	}
    // we used to just return newsInfo if groupIndex == -1
    // now we return nsnull and caller has to check for that.
    return nsnull;
}

nsINNTPNewsgroup *
nsNNTPHost::SwitchCategoryContainerToNews(nsINNTPCategoryContainer*
                                          catContainerInfo)
{
	nsINNTPNewsgroup *retInfo = nsnull;

	PRInt32 groupIndex = m_groups->IndexOf(catContainerInfo);
	if (groupIndex != -1)
	{
		nsINNTPNewsgroup *rootCategory;
        catContainerInfo->GetRootCategory(&rootCategory);
		// slip the root category container where the category container was.
		m_groups->ReplaceElementAt(rootCategory, groupIndex);

        nsIMsgFolder *catContFolder = getFolderFor(catContainerInfo);
        if (catContFolder) {
            nsIMsgFolder *rootFolder = getFolderFor(rootCategory);
            if (rootFolder) {
                m_hostinfo->ReplaceElement(catContFolder, rootFolder);
                NS_RELEASE(rootFolder);
            }
            NS_RELEASE(catContFolder);
        }
        /*
        nsISupportsArray* infoList;
        nsresult rv = m_hostinfo->GetSubFolders(&infoList);
		// replace in folder pane server list as well.
		groupIndex = infoList->FindIndex(0, catContainerInfo);
		if (groupIndex != -1)
			infoList->SetAt(groupIndex, rootCategory);
        */
		retInfo = rootCategory;
		// this effectively leaks the category container, but I don't think that's a problem
	}
	return retInfo;
}
nsresult
nsNNTPHost::RemoveGroup (nsINNTPNewsgroup *newsInfo)
{
    PRBool subscribed;
    if (!newsInfo) return NS_ERROR_NULL_POINTER;
    nsresult rv = ((nsINNTPNewsgroup *)newsInfo)->GetSubscribed(&subscribed);
	if (NS_SUCCEEDED(rv) && subscribed) 
	{
		((nsINNTPNewsgroup *)newsInfo)->SetSubscribed(PR_FALSE);
#ifdef HAVE_MASTER
		m_master->BroadcastFolderDeleted (newsInfo);
#endif
        nsIMsgFolder* newsFolder = getFolderFor((nsINNTPNewsgroup*)newsInfo);
        if (newsFolder) {
            m_hostinfo->RemoveElement(newsFolder);
            NS_RELEASE(newsFolder);
        }
        /*
        nsISupportsArray* infolist;
        nsresult rv = m_hostinfo->GetSubFolders(&infolist);
		infolist->Remove(newsInfo);
        */
	}
    return NS_OK;
}

nsresult
nsNNTPHost::RemoveGroupByName(const char* name)
{
	nsINNTPNewsgroup *newsInfo = nsnull;

    nsresult rv = FindGroup(name, &newsInfo);
    if (NS_SUCCEEDED(rv))
        RemoveGroup (newsInfo);

    return NS_OK;
}


#ifdef DEBUG_terry
// #define XP_WIN16
#endif

const char*
nsNNTPHost::GetDBDirName()
{
	if (!m_filename) return nsnull;
	if (!m_dbfilename) {
#if defined(XP_WIN16) || defined(XP_OS2)
		const PRInt32 MAX_HOST_NAME_LEN = 8;
#elif defined(XP_MAC)
		const PRInt32 MAX_HOST_NAME_LEN = 25;
#else
		const PRInt32 MAX_HOST_NAME_LEN = 55;
#endif
		// Given a hostname, use either that hostname, if it fits on our
		// filesystem, or a hashified version of it, if the hostname is too
		// long to fit.
		char hashedname[MAX_HOST_NAME_LEN + 1];
		PRBool needshash = (((PRInt32)PL_strlen(m_filename)) > MAX_HOST_NAME_LEN);
#if defined(XP_WIN16)  || defined(XP_OS2)
		if (!needshash) {
			needshash = PL_strchr(m_filename, '.') != nsnull ||
				PL_strchr(m_filename, ':') != nsnull;
		}
#endif

		PL_strncpyz(hashedname, m_filename, MAX_HOST_NAME_LEN + 1);
		if (needshash) {
			PR_snprintf(hashedname + MAX_HOST_NAME_LEN - 8, 9, "%08lx",
						(unsigned long) PL_HashString(m_filename));
		}
		m_dbfilename = new char [PL_strlen(hashedname) + 15];
#if defined(XP_WIN16) || defined(XP_OS2)
		PL_strcpy(m_dbfilename, hashedname);
		PL_strcat(m_dbfilename, ".dir");
#else
		PL_strcpy(m_dbfilename, "host-");
		PL_strcat(m_dbfilename, hashedname);
		char* ptr = PL_strchr(m_dbfilename, ':');
		if (ptr) *ptr = '.'; // Windows doesn't like colons in filenames.
#endif

#ifdef UNREADY_CODE
		XP_MakeDirectory(m_dbfilename, xpXoverCache);
#endif
	}
	return m_dbfilename;
}


nsresult
nsNNTPHost::GetNumGroupsNeedingCounts(PRInt32 *value)
{
	if (!m_groups) return NS_ERROR_NOT_INITIALIZED;
	PRUint32 cnt;
    nsresult rv = m_groups->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    PRInt32 num = cnt;
	PRInt32 result = 0;
	for (PRInt32 i=0 ; i<num ; i++) {
		nsINNTPNewsgroup* info = (nsINNTPNewsgroup*) m_groups->ElementAt(i);
        PRBool wantNewTotals, subscribed;
        rv = info->GetWantNewTotals(&wantNewTotals);
        if (NS_SUCCEEDED(rv))
            rv = info->GetSubscribed(&subscribed);
		if (NS_SUCCEEDED(rv) &&
            wantNewTotals &&
            subscribed) {
			result++;
		}
	}
	*value = result;
    // return rv; ??
    return NS_OK;
}

nsresult
nsNNTPHost::GetFirstGroupNeedingCounts(char **result)
{
	if (!m_groups) return NS_ERROR_NULL_POINTER;
	PRUint32 cnt;
    nsresult rv = m_groups->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    PRInt32 num = cnt;
	for (PRInt32 i=0 ; i<num ; i++) {
		nsINNTPNewsgroup* info = (nsINNTPNewsgroup*)m_groups->ElementAt(i);
        
        PRBool wantNewTotals, subscribed;
        rv = info->GetWantNewTotals(&wantNewTotals);
        if (NS_SUCCEEDED(rv))
            rv = info->GetSubscribed(&subscribed);
        
		if (NS_SUCCEEDED(rv) &&
            wantNewTotals &&
            subscribed) {
			info->SetWantNewTotals(PR_FALSE);
            char *name;
            rv = info->GetName(&name);
			if (NS_SUCCEEDED(rv)) {
                *result = PL_strdup(name);
                return NS_OK;
            }
		}
	}
    *result = nsnull;
    // return rv; ??
	return NS_OK;
}


nsresult
nsNNTPHost::GetFirstGroupNeedingExtraInfo(char **result)
{
	nsMsgGroupRecord* grec = nsnull;

    *result=nsnull;
	for (grec = m_groupTree->GetChildren();	 grec; grec = grec->GetNextAlphabetic()) {
		if (grec && grec->NeedsExtraInfo()) {
			char *fullName = grec->GetFullName();
			*result = PL_strdup(fullName);
			delete [] fullName;
            return NS_OK;
		}
	}
	return NS_OK;
}



void
nsNNTPHost::SetWantNewTotals(PRBool value)
{
	if (!m_groups) return;
	PRUint32 cnt;
    nsresult rv = m_groups->Count(&cnt);
    if (NS_FAILED(rv)) return;  // XXX error?
    PRInt32 n = cnt;
	for (PRInt32 i=0 ; i<n ; i++) {
		nsINNTPNewsgroup* info = (nsINNTPNewsgroup*)m_groups->ElementAt(i);
		info->SetWantNewTotals(value);
	}
}



PRBool nsNNTPHost::NeedsExtension (const char * /*extension*/)
{
	//###phil need to flesh this out
	PRBool needed = m_supportsExtensions;
	return needed;
}

nsresult
nsNNTPHost::AddExtension (const char *ext)
{
    PRBool alreadyHasExtension=FALSE;
    QueryExtension(ext, &alreadyHasExtension);
	if (!alreadyHasExtension)
	{
		char *ourExt = PL_strdup (ext);
		if (ourExt)
			m_supportedExtensions.AppendElement(ourExt);
	}
    return NS_OK;
}

nsresult
nsNNTPHost::QueryExtension (const char *ext, PRBool *_retval)
{
    *_retval = PR_FALSE;
	for (PRInt32 i = 0; i < m_supportedExtensions.Count(); i++)
		if (!PL_strcmp(ext, (char*) m_supportedExtensions[i])) {
			*_retval=PR_TRUE;
            return NS_OK;
        }
	return NS_OK;
}

nsresult
nsNNTPHost::AddSearchableGroup (const char *group)
{
    PRBool searchableGroup;
    nsresult rv = QuerySearchableGroup(group, &searchableGroup);
	if (NS_SUCCEEDED(rv) && !searchableGroup)
	{
		char *ourGroup = PL_strdup (group);
		if (ourGroup)
		{
			// strip off character set spec 
			char *space = PL_strchr(ourGroup, ' ');
			if (space)
				*space = '\0';

			m_searchableGroups.AppendElement(ourGroup);

			space++; // walk over to the start of the charsets
			// Add the group -> charset association.
			PL_HashTableAdd(m_searchableGroupCharsets, ourGroup, space);
		}
	}
    return NS_OK;
}

nsresult
nsNNTPHost::QuerySearchableGroup (const char *group, PRBool *_retval)
{
    *_retval = PR_FALSE;
	for (PRInt32 i = 0; i < m_searchableGroups.Count(); i++)
	{
		const char *searchableGroup = (const char*) m_searchableGroups[i];
		char *starInSearchableGroup = nsnull;

		if (!PL_strcmp(searchableGroup, "*")) {
			*_retval = PR_TRUE; // everything is searchable
            return NS_OK;
        }
		else if (nsnull != (starInSearchableGroup = PL_strchr(searchableGroup, '*')))
		{
			if (!PL_strncasecmp(group, searchableGroup, PL_strlen(searchableGroup)-2)) {
				*_retval = PR_TRUE; // this group is in a searchable hierarchy
                return NS_OK;
            }
		}
		else if (!PL_strcasecmp(group, searchableGroup)) {
            *_retval = PR_TRUE; // this group is individually searchable
            return NS_OK;
        }
	}
	return NS_OK;
}

// ### mwelch This should have been merged into one routine with QuerySearchableGroup,
//            but with two interfaces.
nsresult
nsNNTPHost::QuerySearchableGroupCharsets(const char *group, char **result)
{
	// Very similar to the above, but this time we look up charsets.
	PRBool gotGroup = PR_FALSE;
	const char *searchableGroup = nsnull;
    *result = nsnull;

	for (PRInt32 i = 0; (i < m_searchableGroups.Count()) && (!gotGroup); i++)
	{
		searchableGroup = (const char*) m_searchableGroups[i];
		char *starInSearchableGroup = nsnull;

		if (!PL_strcmp(searchableGroup, "*"))
			gotGroup = PR_TRUE; // everything is searchable
		else if (nsnull != (starInSearchableGroup = PL_strchr(searchableGroup, '*')))
		{
			if (!PL_strncasecmp(group, searchableGroup, PL_strlen(searchableGroup)-2))
				gotGroup = PR_TRUE; // this group is in a searchable hierarchy
		}
		else if (!PL_strcasecmp(group, searchableGroup))
			gotGroup = PR_TRUE; // this group is individually searchable
	}

    if (gotGroup)
	{
		// Look up the searchable group for its supported charsets
		*result = (char *) PL_HashTableLookup(m_searchableGroupCharsets, searchableGroup);
	}

	return NS_OK;
}

nsresult
nsNNTPHost::AddSearchableHeader (const char *header)
{
    PRBool searchable;
    nsresult rv = QuerySearchableHeader(header, &searchable);
	if (NS_SUCCEEDED(rv) && searchable)
	{
		char *ourHeader = PL_strdup(header);
		if (ourHeader)
			m_searchableHeaders.AppendElement(ourHeader);
	}
    return NS_OK;
}

nsresult
nsNNTPHost::QuerySearchableHeader(const char *header, PRBool *_retval)
{
    *_retval=PR_FALSE;
	for (PRInt32 i = 0; i < m_searchableHeaders.Count(); i++)
		if (!PL_strncasecmp(header, (char*) m_searchableHeaders[i], PL_strlen(header))) {
			*_retval = PR_TRUE;
            return NS_OK;
        }
	return NS_OK;
}

nsresult
nsNNTPHost::AddPropertyForGet (const char *property, const char *value)
{
	char *tmp = nsnull;
	
	tmp = PL_strdup(property);
	if (tmp)
		m_propertiesForGet.AppendElement(tmp);

	tmp = PL_strdup(value);
	if (tmp)
		m_valuesForGet.AppendElement(tmp);

    // this is odd. do we need this return value?
    return NS_OK;
}

nsresult
nsNNTPHost::QueryPropertyForGet (const char *property, char **_retval)
{
    *_retval=nsnull;
	for (PRInt32 i = 0; i < m_propertiesForGet.Count(); i++)
		if (!PL_strcasecmp(property, (const char *) m_propertiesForGet[i])) {
            *_retval = (char *)m_valuesForGet[i];
			return NS_OK;
        }
    
	return NS_OK;
}


nsresult
nsNNTPHost::SetPushAuth(PRBool value)
{
	if (m_pushAuth != value) 
	{
		m_pushAuth = value;
		m_groupTreeDirty |= 1;
	}
    return NS_OK;
}



const char*
nsNNTPHost::GetURLBase()
{
	if (!m_urlbase) {
		m_urlbase = PR_smprintf("%s://%s", "news",
								getNameAndPort());
	}
	return m_urlbase;
}




PRInt32 nsNNTPHost::RemoveHost()
{
#ifdef UNREADY_CODE
	if (m_groupFile) {
		XP_FileClose(m_groupFile);
		m_groupFile = nsnull;
	}
	m_dirty = 0;
	if (m_writetimer) {
#ifdef UNREADY_CODE
		FE_ClearTimeout(m_writetimer);
#endif
		m_writetimer = nsnull;
	}

	// Tell netlib to close any connections we might have to this news host.
	NET_OnNewsHostDeleted (m_hostname);

	// Delete disk storage for summary files etc.
	// Use notifications through the master to prevent the folder pane 
	// from loading anything while we're deleting all the newsgroups.
#ifdef HAVE_MASTER
	m_master->BroadcastBeginDeletingHost (m_hostinfo);
	nsIMsgFolder *tree = m_master->GetFolderTree();
    rv = tree->PropagateDelete(&m_hostinfo, PR_TRUE /*deleteStorage*/);

	// Here's a little hack to work around the fact that the FE may be holding
	// a newsgroup open, and part of the delete might have failed. The WinFE does 
	// this when you delete a news server from the prefs
	if (NS_SUCCEEDED(rv) && m_hostinfo)
	{
		m_master->BroadcastFolderDeleted (m_hostinfo);
		tree->RemoveSubFolder (m_hostinfo);
	}
	m_master->BroadcastEndDeletingHost (m_hostinfo);

	m_master->GetHostTable()->RemoveEntry(this);
#endif
    // XXX And what are we supposed to be doing here? Do we keep
    // a reference to ourselves?
    // maybe NS_RELEASE(this); ?
#if 0
	delete this;
#endif
#endif

	return 0;
}



char*
nsNNTPHost::GetPrettyName(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (group) {
		const char* orig = group->GetPrettyName();
		if (orig) {
			char* result = new char [PL_strlen(orig) + 1];
			if (result) {
				PL_strcpy(result, orig);
				return result;
			}
		}
	}
	return nsnull;
}


nsresult
nsNNTPHost::SetPrettyName(const char* name, const char* prettyname)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return NS_ERROR_OUT_OF_MEMORY;
	nsresult rv = group->SetPrettyName(prettyname);
	if (NS_FAILED(rv))
	{
		nsINNTPNewsgroup *newsFolder;
        rv = FindGroup(name, &newsFolder);
		// make news folder forget prettyname so it will query again
		if (NS_SUCCEEDED(rv) && newsFolder)	
			newsFolder->SetPrettyName(nsnull);
	}

    // this used to say
    // m_groupTreeDirty |= status;
    // where status came from the previous SetPrettyName
	if (NS_SUCCEEDED(rv)) m_groupTreeDirty |= 1;
	return rv;
}


PRTime
nsNNTPHost::GetAddTime(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return LL_ZERO;
	return group->GetAddTime();
}


PRInt32
nsNNTPHost::GetUniqueID(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return 0;
	return group->GetUniqueID();
}


PRBool
nsNNTPHost::IsCategory(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	return group->IsCategory();
}


PRBool
nsNNTPHost::IsCategoryContainer(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	return group->IsCategoryContainer();
}

PRInt32
nsNNTPHost::SetIsCategoryContainer(const char* name, PRBool value, nsMsgGroupRecord *inGroupRecord)
{
    nsresult rv;
	nsMsgGroupRecord* group = (inGroupRecord) ? inGroupRecord : FindOrCreateGroup(name);
	if (!group) return MK_OUT_OF_MEMORY;
	PRInt32 status = group->SetIsCategoryContainer(value);
	m_groupTreeDirty |= status;
	if (status > 0)
	{
		nsINNTPNewsgroup *newsgroup;
        rv = FindGroup(name, &newsgroup);
        
        nsIMsgFolder *newsFolder = getFolderFor(newsgroup);

		// make news folder have correct category container flag
		if (NS_SUCCEEDED(rv) && newsgroup)	
		{
			if (value)
			{
                // change newsgroup into a category container
				newsFolder->SetFlag(MSG_FOLDER_FLAG_CAT_CONTAINER);
				SwitchNewsToCategoryContainer(newsgroup);
			}
			else
			{
                // change category container into a newsgroup
                nsINNTPCategoryContainer *catCont;
                rv = newsgroup->QueryInterface(nsINNTPCategoryContainer::GetIID(),
                                               (void **)&catCont);

				if (NS_SUCCEEDED(rv))
				{
                    // release the old newsgroup/newsfolder and get
                    // the converted one instead 
                    NS_IF_RELEASE(newsgroup);
					newsgroup = SwitchCategoryContainerToNews(catCont);
                    NS_RELEASE(catCont);

                    // make sure newsFolder is in sync with newsgroup
                    NS_IF_RELEASE(newsFolder);
                    newsFolder = getFolderFor(newsgroup);
                    
				}
                
				if (newsFolder)
				{
					newsFolder->ClearFlag(MSG_FOLDER_FLAG_CAT_CONTAINER);
					newsFolder->ClearFlag(MSG_FOLDER_FLAG_CATEGORY);
				}
			}
            NS_RELEASE(newsFolder);
            NS_RELEASE(newsgroup);
		}
	}
	return status;
}

nsresult
nsNNTPHost::SetGroupNeedsExtraInfo(const char *name, PRBool value)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return NS_ERROR_OUT_OF_MEMORY;
	nsresult rv = group->SetNeedsExtraInfo(value);
	if (NS_SUCCEEDED(rv)) m_groupTreeDirty |= 1;
	return rv;
}


char*
nsNNTPHost::GetCategoryContainer(const char* name, nsMsgGroupRecord *inGroupRecord)
{
	nsMsgGroupRecord* group = (inGroupRecord) ? inGroupRecord : FindOrCreateGroup(name);
	if (group) {
		group = group->GetCategoryContainer();
		if (group) return group->GetFullName();
	}
	return nsnull;
}

nsINNTPNewsgroup *
nsNNTPHost::GetCategoryContainerFolderInfo(const char *name)
{
    nsINNTPNewsgroup *newsgroup=nsnull;
    nsresult rv;
	// because GetCategoryContainer returns nsnull for a category container...
	nsMsgGroupRecord *group = FindOrCreateGroup(name);
	if (group->IsCategoryContainer()) {
        rv = FindGroup(name, &newsgroup);
        if (NS_SUCCEEDED(rv)) return newsgroup;
        else return nsnull;
    }

	char *categoryContainerName = GetCategoryContainer(name);
	if (categoryContainerName)
	{
		rv = FindGroup(categoryContainerName, &newsgroup);
		delete [] categoryContainerName;
	}
	return newsgroup;
}


nsresult
nsNNTPHost::GetIsVirtualGroup(const char* name, PRBool *_retval)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) {
        *_retval = PR_FALSE;
        return NS_OK;
    }
	else
        return group->IsVirtual(_retval);
}


nsresult
nsNNTPHost::SetIsVirtualGroup(const char* name, PRBool value)
{
    return SetIsVirtualGroup(name, value, nsnull);
}
nsresult
nsNNTPHost::SetIsVirtualGroup(const char* name, PRBool value,
                              nsMsgGroupRecord* inGroupRecord)
{
	nsMsgGroupRecord* group = (inGroupRecord) ? inGroupRecord : FindOrCreateGroup(name);

	if (!group) return NS_ERROR_OUT_OF_MEMORY;

	nsresult rv = group->SetIsVirtual(value);
	if (NS_SUCCEEDED(rv)) m_groupTreeDirty |= 1;
	return rv;
}



PRBool
nsNNTPHost::IsGroup(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	return group->IsGroup();
}


PRInt32
nsNNTPHost::SetIsGroup(char* name, PRBool value)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return MK_OUT_OF_MEMORY;
	PRInt32 status = group->SetIsGroup(value);
	m_groupTreeDirty |= status;
	return status;
}


PRBool
nsNNTPHost::IsHTMLOk(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	if (group->IsHTMLOKGroup()) return PR_TRUE;
	for ( ; group ; group = group->GetParent()) {
		if (group->IsHTMLOKTree()) return PR_TRUE;
	}
	return PR_FALSE;
}


PRBool
nsNNTPHost::IsHTMLOKGroup(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	return group->IsHTMLOKGroup();
}


PRInt32
nsNNTPHost::SetIsHTMLOKGroup(char* name, PRBool value)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return MK_OUT_OF_MEMORY;
	PRInt32 status = group->SetIsHTMLOKGroup(value);
	m_groupTreeDirty |= status;
	return status;
}


PRBool
nsNNTPHost::IsHTMLOKTree(char* name)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return PR_FALSE;
	return group->IsHTMLOKTree();
}


PRInt32 
nsNNTPHost::SetIsHTMLOKTree(char* name, PRBool value)
{
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (!group) return MK_OUT_OF_MEMORY;
	PRInt32 status = group->SetIsHTMLOKTree(value);
	m_groupTreeDirty |= status;
	return status;
}

nsMsgGroupRecord*
nsNNTPHost::FindGroupInBlock(nsMsgGroupRecord* parent,
							   char* name,
							   PRInt32* comp)
{
#ifdef UNREADY_CODE
	char* ptr;
	char* ptr2;
	char* tmp;
RESTART:
	ptr = m_block;
	*comp = 0;
	for (;;) {
		ptr = PL_strchr(ptr, LINEBREAK_START);
		ptr2 = ptr ? PL_strchr(ptr, ',') : 0;
		if (!ptr2) {
			if (*comp != 0) return nsnull;
			// Yikes.  The whole string, and we can't even find a legitimate
			// beginning of a real line.  Grow the buffer until we can.

			if (m_fileSize - m_fileStart <= m_blockSize) {
				// Oh, well, maybe the file is just empty.  Fine.
				*comp = 0;
				return nsnull;
			}

			if (PRInt32(PL_strlen(m_block)) < m_blockSize) goto RELOAD;
			goto GROWBUFFER;
		}
		while (*ptr == CR || *ptr == LF) ptr++;
		*ptr2 = '\0';
		PRInt32 c = nsMsgGroupRecord::nameCompare(name, ptr);
		*ptr2 = ',';
		if (c < 0) {
			if (*comp > 0) {
				// This group isn't in the file, since earlier compares said
				// "look later" and this one says "look earlier".
				*comp = 0;
				return nsnull;
			}
			*comp = c;
			return nsnull;		// This group is somewhere before this chunk.
		}
		*comp = c;
		if (c == 0) {
			// Hey look, we found it.
			PRInt32 offset = m_blockStart + (ptr - m_block);
			ptr2 = PL_strchr(ptr2, LINEBREAK_START);
			if (!ptr2) {
				// What a pain.  We found the right place, but the rest of
				// the line is off the end of the buffer.
				if (offset > m_blockStart + MSG_LINEBREAK_LEN) {
					m_blockStart = offset - MSG_LINEBREAK_LEN;
					goto RELOAD;
				}
				// We couldn't find it, even though we're at the beginning
				// of the buffer.  This means that our buffer is too small.
				// Bleah.

				goto GROWBUFFER;
			}
			return nsMsgGroupRecord::Create(parent, ptr, -1, offset);
		}
	}
	/* NOTREACHED */
	return nsnull;

GROWBUFFER:
			
	tmp = new char [m_blockSize + 512 + 1];
	if (!tmp) return nsnull;
	delete [] m_block;
	m_block = tmp;
	m_blockSize += 512;
	
RELOAD:
	if (m_blockStart + m_blockSize > m_fileSize) {
		m_blockStart = m_fileSize - m_blockSize;
		if (m_blockStart < m_fileStart) m_blockStart = m_fileStart;
	}

	OpenGroupFile();
	if (!m_groupFile) return nsnull;
	XP_FileSeek(m_groupFile, m_blockStart, SEEK_SET);
	PRInt32 length = XP_FileRead(m_block, m_blockSize, m_groupFile);
	if (length < 0) length = 0;
	m_block[length] = '\0';
	goto RESTART;
#else
	return nsnull;
#endif
}


nsMsgGroupRecord*
nsNNTPHost::LoadSingleEntry(nsMsgGroupRecord* parent, char* name,
							  PRInt32 min, PRInt32 max)
{
#ifdef UNREADY_CODE
	OpenGroupFile();
	if (!m_groupFile) return nsnull;

#ifdef DEBUG_NEWS
	if (parent != m_groupTree) {
		char* pname = parent->GetFullName();
		if (pname) {
			PR_ASSERT(PL_strncmp(pname, name, PL_strlen(pname)) == 0);
			delete [] pname;
			pname = nsnull;
		}
	}
#endif /* DEBUG_NEWS */

	if (min < m_fileStart) min = m_fileStart;
	if (max < m_fileStart || max > m_fileSize) max = m_fileSize;

	nsMsgGroupRecord* result = nsnull;
	PRInt32 comp = 1;

	// First, check if we happen to already have the line for this group in
	// memory.
	if (m_block[0]) {
		result = FindGroupInBlock(parent, name, &comp);
	}

	while (!result && comp != 0 && min < max) {
		PRInt32 mid = (min + max) / 2;
		m_blockStart = mid - MSG_LINEBREAK_LEN;
		XP_FileSeek(m_groupFile, m_blockStart, SEEK_SET);
		PRInt32 length = XP_FileRead(m_block, m_blockSize, m_groupFile);
		if (length < 0) length = 0;
		m_block[length] = '\0';

		result = FindGroupInBlock(parent, name, &comp);

		if (comp > 0) {
			min = mid + 1;
		} else {
			max = mid;
		}
	}
	return result;
#else
	return 0;
#endif /* UNREADY_CODE */
}


nsMsgGroupRecord*
nsNNTPHost::FindOrCreateGroup(const char* name,
								int* statusOfMakingGroup)
{
	char buf[256];
	
	nsMsgGroupRecord* parent = m_groupTree;
	const char* start = name;
	PR_ASSERT(start && *start);
	if (!start || !*start) return nsnull;

	PR_ASSERT(*start != '.');	// names can't start with ".".
	if (*start == '.') return nsnull;
	
	while (*start)
	{
		if (*start == '.') start++;
		const char* end = PL_strchr(start, '.');
		if (!end) end = start + PL_strlen(start);
		PRInt32 length = end - start;
		PR_ASSERT(length > 0);	// names can't contain ".." or end in
								// a ".".
		if (length <= 0) return nsnull;
		PR_ASSERT((PRUint32)length < sizeof(buf));
		if ((PRUint32)length >= sizeof(buf)) return nsnull;
		PL_strncpyz(buf, start, length + 1);
		buf[length] = '\0';
		
		nsMsgGroupRecord* prev = parent;
		nsMsgGroupRecord* ptr = nsnull;
		PRInt32 comp = 0;  // Initializing to zero.
		if (parent)
		{
			for (ptr = parent->GetChildren() ; ptr ; ptr = ptr->GetSibling()) 
			{
				comp = nsMsgGroupRecord::GroupNameCompare(ptr->GetPartName(), buf);
				if (comp >= 0) 
					break;
				prev = ptr;
			}
		}

		if (ptr == nsnull || comp != 0) 
		{
			// We don't have this one in memory.  See if we can load it in.
			if (!m_inhaled && parent) 
			{
				if (ptr == nsnull) 
				{
					ptr = parent->GetSiblingOrAncestorSibling();
				}
				length = end - name;
				char* tmp = new char[length + 1];
				if (!tmp) return nsnull;
				PL_strncpyz(tmp, name, length + 1);
				tmp[length] = '\0';
				ptr = LoadSingleEntry(parent, tmp,
									  prev->GetFileOffset(),
									  ptr ? ptr->GetFileOffset() : m_fileSize);
				delete [] tmp;
				tmp = nsnull;
			} 
			else 
			{
				ptr = nsnull;
			}

			if (!ptr) 
			{
				m_groupTreeDirty = 2;
				ptr = nsMsgGroupRecord::Create(parent, buf, PR_Now(),
											  m_uniqueId++, 0);
				if (!ptr) return nsnull;
			}
		}
		parent = ptr;
		start = end;
	}
	PRInt32 status = parent->SetIsGroup(PR_TRUE);
	m_groupTreeDirty |= status;
	if (statusOfMakingGroup) *statusOfMakingGroup = status;
	return parent;
}


PRInt32
nsNNTPHost::NoticeNewGroup(const char* name, nsMsgGroupRecord **outGroupRecord)
{
	PRInt32 status = 0;
	nsMsgGroupRecord* group = FindOrCreateGroup(name, &status);
	if (!group) return MK_OUT_OF_MEMORY;
	if (outGroupRecord)
		*outGroupRecord = group;
	return status;
}


PRInt32 
nsNNTPHost::AssureAllDescendentsLoaded(nsMsgGroupRecord* group)
{
#ifdef UNREADY_CODE
	PRInt32 status = 0;
	PR_ASSERT(group);
	if (!group) return -1;
	if (group->IsDescendentsLoaded()) return 0;
	m_blockStart = group->GetFileOffset();
	PR_ASSERT(group->GetFileOffset() > 0);
	if (group->GetFileOffset() == 0) return -1;
	InhaleState state;
	state.tree = m_groupTree;
	state.position = m_blockStart;
	state.onlyIfChild = group;
	state.lastInhaled = nsnull;
	OpenGroupFile();
	PR_ASSERT(m_groupFile);
	if (!m_groupFile) return -1;
	XP_FileSeek(m_groupFile, group->GetFileOffset(), SEEK_SET);
	char* buffer = nsnull;
	PRUint32 buffer_size = 0;
	PRUint32 buffer_fp = 0;
	do {
		status = XP_FileRead(m_block, m_blockSize, m_groupFile);
		if (status <= 0) break;
		status = msg_LineBuffer(m_block, status,
								&buffer, &buffer_size, &buffer_fp,
								PR_FALSE, 
#ifdef XP_OS2
								(nsresult (_Optlink*) (char*,PRUint32,void*))
#endif
								nsNNTPHost::InhaleLine, &state);
	} while (status >= 0);
	if (status >= 0 && buffer_fp > 0) {
		status = InhaleLine(buffer, buffer_fp, &state);
	}
	if (status == -2) status = 0; // Special status meaning we ran out of
								  // lines that are children of the given
								  // group.
	
	PR_FREEIF(buffer);
	m_block[0] = '\0';

	if (status >= 0) group->SetIsDescendentsLoaded(PR_TRUE);
	return status;
#else
	return 0;
#endif
}

nsresult
nsNNTPHost::GroupNotFound(const char *name, PRBool opening)
{
	// if no group command has succeeded, don't blow away categories.
	// The server might be wedged...
	if (!opening && !m_groupSucceeded)
		return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
	nsMsgGroupRecord* group = FindOrCreateGroup(name);
	if (group && (group->IsCategory() || opening))
	{
		nsINNTPNewsgroup *newsInfo = nsnull;
        nsIMsgFolder *newsFolder = nsnull;

        // get the group and corresponding folder
        rv = FindGroup(name, &newsInfo);
        if (NS_SUCCEEDED(rv))
            newsFolder = getFolderFor(newsInfo);
        
		group->SetDoesNotExistOnServer(PR_TRUE);
		m_groupTreeDirty |= 2;	// deleting a group has to force a rewrite anyway
		if (group->IsCategory())
		{
			nsINNTPNewsgroup *catCont = GetCategoryContainerFolderInfo(name);
			if (catCont)
			{
                nsIMsgFolder *catFolder = getFolderFor(catCont);
                if (catFolder) {

                    nsCOMPtr<nsIFolder> parentCategory;
					rv = newsFolder->GetParent(getter_AddRefs(parentCategory));
                    if (NS_SUCCEEDED(rv)) {
                        parentCategory->RemoveElement(newsFolder);
                    }
                    NS_RELEASE(catFolder);
                }
			}
		}
#ifdef HAVE_MASTER
		else if (newsInfo) {
			m_master->BroadcastFolderDeleted (newsInfo);
		}
#endif
		if (newsInfo)
		{
            m_hostinfo->RemoveElement(newsFolder);
            /*
            nsISupportsArray* infolist;
            nsresult rv = m_hostinfo->GetSubFolders(&infolist);
			infolist->Remove(newsInfo);
            */
			m_groups->RemoveElement(newsInfo);
			m_dirty = PR_TRUE;
            NS_RELEASE(newsInfo);
            NS_IF_RELEASE(newsFolder);
		}
	}
    return NS_OK;
}


PRInt32 
nsNNTPHost::ReorderGroup(nsINNTPNewsgroup *groupToMove, nsINNTPNewsgroup *groupToMoveBefore, PRInt32 *newIdx)
{
	// Do the list maintenance to reorder a newsgroup. The news host has a list, and
	// so does the FolderInfo which represents the host in the hierarchy tree

	PRInt32 err = MK_MSG_CANT_MOVE_FOLDER;
    nsIEnumerator *infoList=nsnull;
	nsresult rv = m_hostinfo->GetSubFolders(&infoList);

	if (NS_SUCCEEDED(rv) && groupToMove && groupToMoveBefore && infoList)
	{
        nsIMsgFolder *folderToMoveBefore = getFolderFor(groupToMoveBefore);
		if (m_groups->RemoveElement(groupToMove)) 
		{
			// Not necessary to remove from infoList here because the folderPane does that (urk)

			// Unsubscribed groups are in the lists, but not in the view
			nsIMsgFolder *group = nsnull;
			PRInt32 idxInView, idxInData;
            PRInt32 idxInHostInfo = 0;
            infoList->First();
			PRBool	foundIdxInHostInfo = PR_FALSE;

			PRUint32 cnt;
            rv = m_groups->Count(&cnt);
            if (NS_FAILED(rv)) return rv;
            for (idxInData = 0, idxInView = -1; idxInData < (PRInt32)cnt; idxInData++)
			{
				group = (nsIMsgFolder*)m_groups->ElementAt(idxInData);

                nsIMsgFolder *groupInHostInfo;
                (void)infoList->CurrentItem((nsISupports**)&groupInHostInfo);
#if 0
                nsISupports *hostInfoSupports;
                (void)infoList->CurrentItem(&hostInfoSupports);
                
				nsIMsgFolder *groupInHostInfo=nsnull;
                rv = hostInfoSupports->QueryInterface(::nsISupports::GetIID(),
                                                      (void **)&groupInHostInfo);
#endif
#ifdef HAVE_PANE
				if (group->CanBeInFolderPane())
					idxInView++;
#endif
                /* XXX - pointer comparisons are bad in COM! */
				if (group == folderToMoveBefore)
					break;
				if (groupInHostInfo == folderToMoveBefore)
					foundIdxInHostInfo = PR_TRUE;
				else if (!foundIdxInHostInfo) {
					idxInHostInfo++;
                    infoList->Next();
                }
                NS_RELEASE(groupInHostInfo);
#if 0
                NS_RELEASE(hostInfoSupports);
#endif
			} 

			if (idxInView != -1) 
			{
				m_groups->InsertElementAt(groupToMove, idxInData); // the index has to be the same, right?
                nsISupports* groupSupports;
                groupToMove->QueryInterface(::nsISupports::GetIID(),
                                            (void **)&groupSupports);
//XXX				infoList->InsertElementAt(groupSupports, idxInHostInfo);
                NS_RELEASE(groupSupports);

				MarkDirty (); // Make sure the newsrc gets written out in the new order
				*newIdx = idxInView; // Tell the folder pane where the new item belongs

				err = 0;
                NS_RELEASE(groupSupports);
			}
		}
	}
	return err;
}


PRInt32
nsNNTPHost::DeleteFiles ()
{
#ifdef UNREADY_CODE
	// hostinfo.dat
	PR_ASSERT(m_hostinfofilename);
	if (m_hostinfofilename)
		XP_FileRemove (m_hostinfofilename, xpXoverCache);

	// newsrc file
	const char *newsrc = GetNewsrcFileName();
	PR_ASSERT(newsrc);
	if (newsrc)
		XP_FileRemove (newsrc, xpNewsRC);

	// Delete directory
	const char *dbdirname = GetDBDirName();
	PR_ASSERT(dbdirname);
	if (dbdirname)
		return XP_RemoveDirectory (dbdirname, xpXoverCache);
#endif
	return 0;
}

nsresult
nsNNTPHost::GetNewsgroupList(const char* name, nsINNTPNewsgroupList **_retval)
{
	nsresult result = NS_ERROR_NOT_INITIALIZED;

	if (name == nsnull) {
		return NS_ERROR_NULL_POINTER;
	}
#ifdef DEBUG_NEWS
	printf("GetNewsgroupList(%s)\n",name);	
#endif
			
	if (m_newsgrouplists == nsnull) return result;
	PRUint32 cnt;
    nsresult rv = m_newsgrouplists->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    PRInt32 n = cnt;
	for (PRInt32 i=0 ; i<n ; i++) {
        char *newsgroupname = nsnull;
		nsINNTPNewsgroupList* list = (nsINNTPNewsgroupList*)m_newsgrouplists->ElementAt(i);
        rv = list->GetGroupName(&newsgroupname);
       
#ifdef DEBUG_NEWS
		printf("%d = %s\n",i,newsgroupname?newsgroupname:"null");
#endif

		if (NS_SUCCEEDED(rv) && (newsgroupname != nsnull) && PL_strcmp(name,newsgroupname) == 0) {
			*_retval = list;
            result = NS_OK;
		}
	}
    return result;
}

/* given a message id like asl93490@ahost.com, convert
 * it to a newsgroup and a message number within that group
 * (stolen from msgglue.cpp - MSG_NewsGroupAndNumberOfID)
 */
nsresult
nsNNTPHost::GetNewsgroupAndNumberOfID(const char *message_id,
                                      nsINNTPNewsgroup **group,
                                      PRUint32 *messageNumber)
{
#ifdef HAVE_DBVIEW
    MessageDBView *view = pane->GetMsgView();
    if (!view || !view->GetDB())
        return NS_ERROR_NOT_INITIALIZED;
    nsMsgKey = view->GetDB()->GetMessageKeyForID(message_id);
    *messageNumber = (nsMsgKey == nsMsgKey_None) ? 0 : nsMsgKey;
#endif

    /* Why are we always choosing the current pane's folder? */
#ifdef HAVE_PANE
     MSG_FolderInfo *folderInfo = pane->GetFolder();
    if (folderInfo != nsnull && folderInfo->IsNews())
    {
        MSG_FolderInfoNews *newsFolderInfo =
            (MSG_FolderInfoNews *) folderInfo;
        *name = newsFolderInfo->GetNewsgroupName();
    }
#endif
    return NS_OK;
}

/* this function originally lived in a pane
 */
nsresult
nsNNTPHost::AddNewNewsgroup(const char *name,
                            PRInt32 first,
                            PRInt32 last,
                            const char *flags,
                            PRBool xactiveFlags) {

    nsMsgGroupRecord     *groupRecord = nsnull;
    nsresult rv;
    
#ifdef DEBUG_NEWS
	printf("nsNNTPHost::AddNewNewsgroup(%s,...)\n",name);
#endif

    PRInt32 status = NoticeNewGroup(name, &groupRecord);
    if (status < 0) return status;
    
    /* this used to be for the pane */
    /*    if (status > 0) m_numNewGroups++; */

    PRBool     bIsCategoryContainer = PR_FALSE;
    PRBool     bIsVirtual = PR_FALSE;

    while (flags && *flags)
    {
        char flag = toupper(*flags);
        flags++;
        switch (flag)
        {
            case 'C':
                bIsCategoryContainer = TRUE;
                break;
            case 'P':           // profile
            case 'V':
                bIsVirtual = TRUE;
                break;
            default:
                break;
        }
    }
    if (xactiveFlags)
    {
        SetIsCategoryContainer(name, bIsCategoryContainer, groupRecord);
        SetIsVirtualGroup(name, bIsVirtual, groupRecord);
    }

    if (status > 0) {
        // If this really is a new newsgroup, then if it's a category of a
        // subscribed newsgroup, then automatically subscribe to it.
        char* containerName = GetCategoryContainer(name, groupRecord);
        if (containerName) {
            nsINNTPNewsgroup* categoryInfo;
            rv = FindGroup(containerName, &categoryInfo);
            
            if (NS_SUCCEEDED(rv)) {
                PRBool isSubscribed;
                categoryInfo->GetSubscribed(&isSubscribed);
                if (isSubscribed) {
                    // this autosubscribes categories of subscribed newsgroups.
                    nsINNTPNewsgroup *newsgroup;
                    rv = AddGroup(name, groupRecord, &newsgroup);
                    if (NS_SUCCEEDED(rv)) NS_RELEASE(newsgroup);
                    
                }
                NS_RELEASE(categoryInfo);
            }
            delete [] containerName;
        }
    }

    if (status <=0) return NS_ERROR_UNEXPECTED;
    
    return NS_OK;
}

nsresult
nsNNTPHost::DisplaySubscribedGroup(const char *group,
                                   PRInt32 first_message,
                                   PRInt32 last_message,
                                   PRInt32 total_messages,
                                   PRBool visit_now)
{
    nsresult rv;
    nsINNTPNewsgroup *newsgroup=nsnull;

    
    rv = FindGroup(group, &newsgroup);
    
    SetGroupSucceeded(TRUE);
    if (!newsgroup && visit_now) // let's try autosubscribe...
    {
        rv = AddGroup(group, nsnull, &newsgroup);
    }

    if (!newsgroup)
        return NS_OK;
    else {
        PRBool subscribed;
        newsgroup->GetSubscribed(&subscribed);
        if (!subscribed)
            newsgroup->SetSubscribed(TRUE);
    }

    if (!newsgroup) return NS_OK;
    newsgroup->UpdateSummaryFromNNTPInfo(first_message, last_message,
                                    total_messages);
    return NS_OK;
}


#define MSG_IMPL_GETFOLDER(_type)\
nsIMsgFolder * \
nsNNTPHost::getFolderFor(_type * _class) {\
   nsIMsgFolder* folder;\
   nsresult rv = \
      _class->QueryInterface(nsIMsgFolder::GetIID(), (void **)&folder);\
   if (NS_SUCCEEDED(rv)) return folder;\
   else return nsnull;\
}\

MSG_IMPL_GETFOLDER(nsINNTPNewsgroup)
MSG_IMPL_GETFOLDER(nsINNTPCategoryContainer)





