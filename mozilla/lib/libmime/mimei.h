/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/* mimei.h --- class definitions for the MIME parser, version 2.
   Created: Jamie Zawinski <jwz@netscape.com>, 15-May-96.
 */

#ifndef _MIMEI_H_
#define _MIMEI_H_

/*
  This module, libmime, implements a general-purpose MIME parser.
  One of the methods provided by this parser is the ability to emit
  an HTML representation of it.

  All Mozilla-specific code is (and should remain) isolated in the
  file mimemoz.c.  Generally, if the code involves images, netlib
  streams, or MWContexts, it should be in mimemoz.c instead of in
  the main body of the MIME parser.

  The parser is object-oriented and fully buzzword-compliant.
  There is a class for each MIME type, and each class is responsible
  for parsing itself, and/or handing the input data off to one of its
  child objects.

  The class hierarchy is:

     MimeObject (abstract)
      |
      |--- MimeContainer (abstract)
      |     |
      |     |--- MimeMultipart (abstract)
      |     |     |
      |     |     |--- MimeMultipartMixed
      |     |     |
      |     |     |--- MimeMultipartDigest
      |     |     |
      |     |     |--- MimeMultipartParallel
      |     |     |
      |     |     |--- MimeMultipartAlternative
      |     |     |
      |     |     |--- MimeMultipartRelated
      |     |     |
      |     |     |--- MimeMultipartAppleDouble
      |     |     |
      |     |     |--- MimeSunAttachment
      |     |     |
      |     |     |--- MimeMultipartSigned (abstract)
      |     |          |
      |     |          |--- 
      |     |
      |     |--- MimeEncrypted (abstract)
      |     |     |
      |     |     |--- 
      |     |
      |     |--- MimeMessage
      |     |
      |     |--- MimeUntypedText
      |
      |--- MimeLeaf (abstract)
      |     |
      |     |--- MimeInlineText (abstract)
      |     |     |
      |     |     |--- MimeInlineTextPlain
      |     |     |
      |     |     |--- MimeInlineTextHTML
      |     |     |
      |     |     |--- MimeInlineTextRichtext
      |     |     |     |
      |     |     |     |--- MimeInlineTextEnriched
      |     |	  |
      |     |	  |--- MimeInlineTextVCard
      |     |
      |     |--- MimeInlineImage
      |     |
      |     |--- MimeExternalObject
      |
      |--- MimeExternalBody

  =========================================================================
  The definition of these classes is somewhat idiosyncratic, since I defined
  my own small object system, instead of giving the C++ virus another foothold.
  (I would have liked to have written this in Java, but our runtime isn't
  quite ready for prime time yet.)

  There is one header file and one source file for each class (for example,
  the MimeInlineText class is defined in "mimetext.h" and "mimetext.c".)
  Each header file follows the following boiler-plate form:

  TYPEDEFS: these come first to avoid circular dependencies.

      typedef struct FoobarClass FoobarClass;
      typedef struct Foobar      Foobar;

  CLASS DECLARATION:
  Theis structure defines the callback routines and other per-class data
  of the class defined in this module.

      struct FoobarClass {
        ParentClass superclass;
        ...any callbacks or class-variables...
      };

  CLASS DEFINITION:
  This variable holds an instance of the one-and-only class record; the
  various instances of this class point to this object.  (One interrogates
  the type of an instance by comparing the value of its class pointer with
  the address of this variable.)

      extern FoobarClass foobarClass;

  INSTANCE DECLARATION:
  Theis structure defines the per-instance data of an object, and a pointer
  to the corresponding class record.

      struct Foobar {
        Parent parent;
        ...any instance variables...
      };

  Then, in the corresponding .c file, the following structure is used:

  CLASS DEFINITION:
  First we pull in the appropriate include file (which includes all necessary
  include files for the parent classes) and then we define the class object
  using the MimeDefClass macro:

      #include "foobar.h"
      #define MIME_SUPERCLASS parentlClass
      MimeDefClass(Foobar, FoobarClass, foobarClass, &MIME_SUPERCLASS);

  The definition of MIME_SUPERCLASS is just to move most of the knowlege of the
  exact class hierarchy up to the file's header, instead of it being scattered
  through the various methods; see below.

  METHOD DECLARATIONS:
  We will be putting function pointers into the class object, so we declare
  them here.  They can generally all be static, since nobody outside of this
  file needs to reference them by name; all references to these routines should
  be through the class object.

      extern int FoobarMethod(Foobar *);
      ...etc...

  CLASS INITIALIZATION FUNCTION:
  The MimeDefClass macro expects us to define a function which will finish up
  any initialization of the class object that needs to happen before the first
  time it is instantiated.  Its name must be of the form "<class>Initialize",
  and it should initialize the various method slots in the class as
  appropriate.  Any methods or class variables which this class does not wish
  to override will be automatically inherited from the parent class (by virtue
  of its class-initialization function having been run first.)  Each class
  object will only be initialized once.

      static int
      FoobarClassInitialize(FoobarClass *class)
      {
        class->method = FoobarMethod.
        ...etc...
      }

  METHOD DEFINITIONS:
  Next come the definitions of the methods we referred to in the class-init
  function.  The way to access earlier methods (methods defined on the
  superclass) is to simply extract them from the superclass's object.
  But note that you CANNOT get at methods by indirecting through
  object->class->superclass: that will only work to one level, and will
  go into a loop if some subclass tries to continue on this method.

  The easiest way to do this is to make use of the MIME_SUPERCLASS macro that
  was defined at the top of the file, as shown below.  The alternative to that
  involves typing the literal name of the direct superclass of the class
  defined in this file, which will be a maintenance headache if the class
  hierarchy changes.  If you use the MIME_SUPERCLASS idiom, then a textual
  change is required in only one place if this class's superclass changes.

      static void
      Foobar_finalize (MimeObject *object)
      {
        ((MimeObjectClass*)&MIME_SUPERCLASS)->finalize(object);  //  RIGHT
        parentClass.whatnot.object.finalize(object);             //  (works...)
        object->class->superclass->finalize(object);             //  WRONG!!
      }
 */

#include "libmime.h"
#include "mimehdrs.h"

typedef struct MimeObject      MimeObject;
typedef struct MimeObjectClass MimeObjectClass;

#ifdef XP_WIN16
 /* Those winners who brought us the Win16 compiler seemed to be under
    the impression that C is a case-insensitive language.  How very.
  */
# define mimeObject				  mimeObject16
# define mimeContainer			  mimeContainer16
# define mimeMultipart			  mimeMultipart16
# define mimeMultipartMixed		  mimeMultipartMixed16
# define mimeMultipartDigest	  mimeMultipartDigest16
# define mimeMultipartParallel	  mimeMultipartParallel16
# define mimeMultipartAlternative mimeMultipartAlternative16
# define mimeMultipartRelated	  mimeMultipartRelated16
# define mimeMultipartAppleDouble mimeMultipartAppleDouble16
# define mimeSunAttachment		  mimeSunAttachment16
# define mimeMultipartSigned	  mimeMultipartSigned16
# define mimeMultipartSignedPKCS7 mimeMultipartSignedPKCS716
# define mimeEncrypted			  mimeEncrypted16
# define mimeEncryptedPKCS7		  mimeEncryptedPKCS716
# define mimeMessage			  mimeMessage16
# define mimeUntypedText		  mimeUntypedText16
# define mimeLeaf				  mimeLeaf16
# define mimeInlineText			  mimeInlineText16
# define mimeInlineTextPlain	  mimeInlineTextPlain16
# define mimeInlineTextHTML		  mimeInlineTextHTML16
# define mimeInlineTextRichtext	  mimeInlineTextRichtext16
# define mimeInlineTextEnriched	  mimeInlineTextEnriched16
# define mimeInlineTextVCard	  mimeInlineTextVCard16
# define mimeInlineImage		  mimeInlineImage16
# define mimeExternalObject		  mimeExternalObject16
# define mimeExternalBody		  mimeExternalBody16
#endif /* XP_WIN16 */


/* (I don't pretend to understand this.) */
#define cpp_stringify_noop_helper(x)#x
#define cpp_stringify(x) cpp_stringify_noop_helper(x)


/* Macro used for setting up class definitions.
 */
#define MimeDefClass(ITYPE,CTYPE,CVAR,CSUPER) \
 static int CTYPE##Initialize(CTYPE *); \
 CTYPE CVAR = { cpp_stringify(ITYPE), sizeof(ITYPE), \
				(MimeObjectClass *) CSUPER, \
				(int (*) (MimeObjectClass *)) CTYPE##Initialize, 0, }


/* Creates a new (subclass of) MimeObject of the given class, with the
   given headers (which are copied.)
 */
extern MimeObject *mime_new (MimeObjectClass *class, MimeHeaders *hdrs,
							 const char *override_content_type);


/* Destroys a MimeObject (or subclass) and all data associated with it.
 */
extern void mime_free (MimeObject *object);

/* Given a content-type string, finds and returns an appropriate subclass
   of MimeObject.  A class object is returned.  If `exact_match_p' is true,
   then only fully-known types will be returned; that is, if it is true,
   then "text/x-unknown" will return MimeInlineTextPlainType, but if it is
   false, it will return NULL.
 */
extern MimeObjectClass *mime_find_class (const char *content_type,
										 MimeHeaders *hdrs,
										 MimeDisplayOptions *opts,
										 XP_Bool exact_match_p);

/* Given a content-type string, creates and returns an appropriate subclass
   of MimeObject.  The headers (from which the content-type was presumably
   extracted) are copied.
 */
extern MimeObject *mime_create (const char *content_type, MimeHeaders *hdrs,
								MimeDisplayOptions *opts);


/* Querying the type hierarchy */
extern XP_Bool mime_subclass_p(MimeObjectClass *child,
							   MimeObjectClass *parent);
extern XP_Bool mime_typep(MimeObject *obj, MimeObjectClass *class);

/* Returns a string describing the location of the part (like "2.5.3").
   This is not a full URL, just a part-number.
 */
extern char *mime_part_address(MimeObject *obj);

/* Puts a part-number into a URL.  If append_p is true, then the part number
   is appended to any existing part-number already in that URL; otherwise,
   it replaces it.
 */
extern char *mime_set_url_part(const char *url, char *part, XP_Bool append_p);


/* Given a part ID, looks through the MimeObject tree for a sub-part whose ID
   number matches, and returns the MimeObject (else NULL.)
   (part is not a URL -- it's of the form "1.3.5".)
 */
extern MimeObject *mime_address_to_part(const char *part, MimeObject *obj);


/* Given a part ID, looks through the MimeObject tree for a sub-part whose ID
   number matches; if one is found, returns the Content-Name of that part.
   Else returns NULL.  (part is not a URL -- it's of the form "1.3.5".)
 */
extern char *mime_find_suggested_name_of_part(const char *part,
											  MimeObject *obj);

/* Given a part ID, looks through the MimeObject tree for a sub-part whose ID
   number matches; if one is found, returns the Content-Name of that part.
   Else returns NULL.  (part is not a URL -- it's of the form "1.3.5".)
 */
extern char *mime_find_content_type_of_part(const char *part, MimeObject *obj);

/* Given a part ID, looks through the MimeObject tree for a sub-part whose ID
   number matches; if one is found, and if it represents a PKCS7-encrypted
   object, returns information about the security status of that object.

   `part' is not a URL -- it's of the form "1.3.5" and is interpreted relative
   to the `obj' argument.
 */
extern void mime_find_security_info_of_part(const char *part, MimeObject *obj,
									  void **pkcs7_encrypt_content_info_return,
									     void **pkcs7_sign_content_info_return,
											char **sender_email_addr_return,
											int32 *decode_error_return,
											int32 *verify_error_return);


/* Parse the various "?" options off the URL and into the options struct.
 */
extern int mime_parse_url_options(const char *url, MimeDisplayOptions *);


/* Asks whether the given object is one of the cryptographically signed
   or encrypted objects that we know about.  (MimeMessageClass uses this
   to decide if the headers need to be presented differently.)
 */
extern XP_Bool mime_crypto_object_p(MimeHeaders *, XP_Bool clearsigned_counts);

/* Tells whether the given MimeObject is a message which has been encrypted
   or signed.  (Helper for MIME_GetMessageCryptoState()). 
 */
extern void mime_get_crypto_state (MimeObject *obj,
								   XP_Bool *signed_p, XP_Bool *encrypted_p,
								   XP_Bool *signed_ok, XP_Bool *encrypted_ok);


/* Whether the given object has written out the HTML version of its headers
   in such a way that it will have a "crypto stamp" next to the headers.  If
   this is true, then the child must write out its HTML slightly differently
   to take this into account...
 */
extern XP_Bool mime_crypto_stamped_p(MimeObject *obj);

/* How the crypto code tells the MimeMessage object what the crypto stamp
   on it says. */
extern void mime_set_crypto_stamp(MimeObject *obj,
								  XP_Bool signed_p, XP_Bool encrypted_p);


struct MimeParseStateObject {

  MimeObject *root;				/* The outermost parser object. */

  XP_Bool separator_queued_p;	/* Whether a separator should be written out
								   before the next text is written (this lets
								   us write separators lazily, so that one
								   doesn't appear at the end, and so that more
								   than one don't appear in a row.) */

  XP_Bool separator_suppressed_p; /* Whether the currently-queued separator
								   should not be printed; this is a kludge to
								   prevent seps from being printed just after
								   a header block... */

  XP_Bool first_part_written_p;	/* State used for the `Show Attachments As
								   Links' kludge. */

  XP_Bool post_header_html_run_p; /* Whether we've run the
									 options->generate_post_header_html_fn */

  XP_Bool first_data_written_p;	/* State used for Mozilla lazy-stream-
								   creation evilness. */

  XP_Bool decrypted_p;			/* If options->decrypt_p is true, then this
								   will be set to indicate whether any
								   decryption did in fact occur.
								 */
};



/* Some output-generation utility functions...
 */
extern int MimeObject_output_init(MimeObject *obj, const char *content_type);

/* The `user_visible_p' argument says whether the output that has just been
   written will cause characters or images to show up on the screen, that
   is, it should be FALSE if the stuff being written is merely structural
   HTML or whitespace ("<P>", "</TABLE>", etc.)  This information is used
   when making the decision of whether a separating <HR> is needed.
 */
extern int MimeObject_write(MimeObject *, char *data, int32 length,
							XP_Bool user_visible_p);
extern int MimeOptions_write(MimeDisplayOptions *,
							 char *data, int32 length,
							 XP_Bool user_visible_p);

/* Writes out the right kind of HR (or rather, queues it for writing.) */
extern int MimeObject_write_separator(MimeObject *);


/* Random junk
 */

extern int MK_OUT_OF_MEMORY;

#ifdef FREEIF
# undef FREEIF
#endif
#define FREEIF(obj) do { if (obj) { XP_FREE (obj); obj = 0; }} while (0)


#ifndef MOZILLA_30
/* Turn this on if you want to play with the idea of displaying icons in the
   headers to represent attachments, and put icons next to each attachment so
   you can easily save them without having to bring up the "as links" view.
   Right now, this is all really half-baked, half-implemented,
   half-thought-out, and so on.  But the current "attachment panel" needs to be
   destroyed, and this is the only hope. */
#define JS_ATTACHMENT_MUMBO_JUMBO
#endif /* MOZILLA_30 */


/* #### These ought to be in libxp or nspr, not libmsg...
 */
extern int msg_GrowBuffer (int32 desired_size, int32 element_size,
						   int32 quantum, char **buffer, int32 *size);

extern int msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
						   char **bufferP, int32 *buffer_sizeP,
						   int32 *buffer_fpP,
						   XP_Bool convert_newlines_p,
						   int (*per_line_fn) (char *line,
											   int32 line_length,
											   void *closure),
						   void *closure);

#endif /* _MIMEI_H_ */
