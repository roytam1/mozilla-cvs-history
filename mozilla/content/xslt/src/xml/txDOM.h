/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Please see release.txt distributed with this file for more information.
 *
 */

// Tom Kneeland (3/29/99)
//
//  Implementation of the Document Object Model Level 1 Core
//
// Modification History:
// Who  When        What
// TK   03/29/99    Created
// LF   08/06/1999  Changed static const short NodeType to enum
//                  Added "friend NamedNodeMap"; to NodeListDefinition
//

#ifndef MITRE_DOM
#define MITRE_DOM

#ifdef __BORLANDC__
#include <stdlib.h>
#endif

#include "List.h"
#include "TxString.h"
#include "baseutils.h"

#ifndef NULL
typedef 0 NULL;
#endif

typedef UNICODE_CHAR DOM_CHAR;

#define kTxNsNodeIndexOffset 0x00000000;
#define kTxAttrIndexOffset 0x40000000;
#define kTxChildIndexOffset 0x80000000;

class NodeList;
class NamedNodeMap;
class Document;
class Element;
class Attr;
class Text;
class Comment;
class CDATASection;
class ProcessingInstruction;
class EntityReference;
class DocumentType;

class txAtom;

/*
 * NULL string for use by Element::getAttribute() for when the attribute
 * specified by "name" does not exist, and therefore shoud be "NULL".
 * Used in txNamespaceManager as well.
 */
const String NULL_STRING;

#define kNameSpaceID_Unknown -1
#define kNameSpaceID_None     0
// not really a namespace, but it needs to play the game
#define kNameSpaceID_XMLNS    1 
#define kNameSpaceID_XML      2
// kNameSpaceID_XSLT is 6 for module, see nsINameSpaceManager.h
#define kNameSpaceID_XSLT     3

//
//Definition and Implementation the DOMImplementation class
//
class DOMImplementation
{
  public:
    DOMImplementation();
    ~DOMImplementation();

    MBool hasFeature(String feature, const String& version) const;

  private:
    String implFeature;
    String implVersion;
};

//
// Abstract Class defining the interface for a Node.  See NodeDefinition below
// for the actual implementation of the WC3 node.
//
class Node : public TxObject
{
  public:
    //Node type constants
    //-- LF - changed to enum
    enum NodeType {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE,
        TEXT_NODE,
        CDATA_SECTION_NODE,
        ENTITY_REFERENCE_NODE,
        ENTITY_NODE,
        PROCESSING_INSTRUCTION_NODE,
        COMMENT_NODE,
        DOCUMENT_NODE,
        DOCUMENT_TYPE_NODE,
        DOCUMENT_FRAGMENT_NODE,
        NOTATION_NODE
    };

    virtual ~Node() {}

    //Read functions
    virtual const String& getNodeName() const = 0;
    virtual const String& getNodeValue() = 0;
    virtual unsigned short getNodeType() const = 0;
    virtual Node* getParentNode() const = 0;
    virtual NodeList* getChildNodes() = 0;
    virtual Node* getFirstChild() const = 0;
    virtual Node* getLastChild() const = 0;
    virtual Node* getPreviousSibling() const = 0;
    virtual Node* getNextSibling() const = 0;
    virtual NamedNodeMap* getAttributes() = 0;
    virtual Document* getOwnerDocument() const = 0;

    //Write functions
    virtual void setNodeValue(const String& nodeValue) = 0;

    //Node manipulation functions
    virtual Node* insertBefore(Node* newChild, Node* refChild) = 0;
    virtual Node* replaceChild(Node* newChild, Node* oldChild) = 0;
    virtual Node* removeChild(Node* oldChild) = 0;
    virtual Node* appendChild(Node* newChild) = 0;
    virtual Node* cloneNode(MBool deep, Node* dest) = 0;

    virtual MBool hasChildNodes() const = 0;
    
    //From DOM3 26-Jan-2001 WD
    virtual String getBaseURI() = 0;

    //Introduced in DOM2
    virtual const String& getNamespaceURI() = 0;

    //txXPathNode functions
    virtual MBool getLocalName(txAtom** aLocalName) = 0;
    virtual PRInt32 getNamespaceID() = 0;
    virtual PRInt32 lookupNamespaceID(txAtom* aPrefix) = 0;
    virtual Node* getXPathParent() = 0;
    virtual PRInt32 compareDocumentPosition(Node* aOther) = 0;
};

//
// Abstract class containing the Interface for a NodeList.  See NodeDefinition
// below for the actual implementation of a WC3 NodeList as it applies to the
// getChildNodes Node function.  Also see NodeListDefinition for the
// implementation of a NodeList as it applies to such functions as
// getElementByTagName.
//
class NodeList
{
  public:
    virtual Node* item(PRUint32 index) = 0;
    virtual PRUint32 getLength() = 0;
  protected:
    PRUint32 length;
};

//
//Definition of the implementation of a NodeList.  This class maintains a
//linked list of pointers to Nodes.  "Friends" of the class can add and remove
//pointers to Nodes as needed.
//      *** NOTE: Is there any need for someone to "remove" a node from the
//                list?
//
class NodeListDefinition : public NodeList
{
  friend class NamedNodeMap; //-- LF
  public:
    NodeListDefinition();
    virtual ~NodeListDefinition();

    void append(Node& newNode);
    void append(Node* newNode);

    //Inherited from NodeList
    Node* item(PRUint32 index);
    PRUint32 getLength();

  protected:
    struct ListItem {
      ListItem* next;
      ListItem* prev;
      Node* node;
    };

    ListItem* firstItem;
    ListItem* lastItem;
};

//
//Definition of a NamedNodeMap.  For the time being it builds off the
//NodeListDefinition class.  This will probably change when NamedNodeMap needs
//to move to a more efficient search algorithm for attributes.
//
class NamedNodeMap : public NodeListDefinition
{
  public:
    NamedNodeMap();
    virtual ~NamedNodeMap();

    Node* getNamedItem(const String& name);
    virtual Node* setNamedItem(Node* arg);
    virtual Node* removeNamedItem(const String& name);

  private:
    NodeListDefinition::ListItem* findListItemByName(const String& name);
};

//
// Subclass of NamedNodeMap that contains a list of attributes.
// Whenever an attribute is added to or removed from the map, the attributes
// ownerElement is updated.
//
class AttrMap : public NamedNodeMap
{
    // Elenent needs to be friend to be able to set the AttrMaps ownerElement
    friend class Element;

  public:
    AttrMap();
    virtual ~AttrMap();

    Node* setNamedItem(Node* arg);
    Node* removeNamedItem(const String& name);
    void clear();

  private:
    Element* ownerElement;
};

//
// Definition and Implementation of Node and NodeList functionality.  This is
// the central class, from which all other DOM classes (objects) are derrived.
// Users of this DOM should work strictly with the Node interface and NodeList
// interface (see above for those definitions)
//
class NodeDefinition : public Node, public NodeList
{
  public:
    NodeDefinition(NodeType type, const String& name,
                   const String& value, Document* owner);
    virtual ~NodeDefinition();      //Destructor, delete all children of node

    //Read functions
    const String& getNodeName() const;
    virtual const String& getNodeValue();
    unsigned short getNodeType() const;
    Node* getParentNode() const;
    NodeList* getChildNodes();
    Node* getFirstChild() const;
    Node* getLastChild() const;
    Node* getPreviousSibling() const;
    Node* getNextSibling() const;
    virtual NamedNodeMap* getAttributes();
    Document* getOwnerDocument() const;

    //Write functions
    virtual void setNodeValue(const String& nodeValue);

    //Child node manipulation functions
    virtual Node* insertBefore(Node* newChild, Node* refChild);
    virtual Node* replaceChild(Node* newChild, Node* oldChild);
    virtual Node* removeChild(Node* oldChild);
    virtual Node* appendChild(Node* newChild);
    Node* cloneNode(MBool deep, Node* dest);

    MBool hasChildNodes() const;
    
    //From DOM3 26-Jan-2001 WD
    virtual String getBaseURI();

    //Introduced in DOM2
    const String& getNamespaceURI();

    //txXPathNode functions
    virtual MBool getLocalName(txAtom** aLocalName);
    virtual PRInt32 getNamespaceID();
    virtual PRInt32 lookupNamespaceID(txAtom*);
    virtual Node* getXPathParent();
    virtual PRInt32 compareDocumentPosition(Node* aOther);

    //Inherited from NodeList
    Node* item(PRUint32 index);
    PRUint32 getLength();

  protected:
    //Name, value, and attributes for this node.  Available to derrived
    //classes, since those derrived classes have a better idea how to use them,
    //than the generic node does.
    String nodeName;
    String nodeValue;

    void DeleteChildren();

    Node* implInsertBefore(NodeDefinition* newChild, NodeDefinition* refChild);
  private:
    //Type of node this is
    NodeType nodeType;

    //Data members for linking this Node to its parent and siblings
    NodeDefinition* parentNode;
    NodeDefinition* previousSibling;
    NodeDefinition* nextSibling;

    //Pointer to the node's document
    Document* ownerDocument;

    //Data members for maintaining a list of child nodes
    NodeDefinition* firstChild;
    NodeDefinition* lastChild;

    // Struct to hold document order information
    struct OrderInfo {
        ~OrderInfo();
        PRUint32* mOrder;
        PRInt32 mSize;
        Node* mRoot;
    };

    // OrderInfo object for comparing document order
    OrderInfo* mOrderInfo;

    // Helperfunction for compareDocumentOrder
    OrderInfo* getOrderInfo();
};

//
//Definition and Implementation of a Document Fragment.  All functionality is
//inherrited directly from NodeDefinition.  We just need to make sure the Type
//of the node set to Node::DOCUMENT_FRAGMENT_NODE.
//
class DocumentFragment : public NodeDefinition
{
  public:
    DocumentFragment(const String& name, const String& value, Document* owner);

    //Override insertBefore to limit Elements to having only certain nodes as
    //children
    Node* insertBefore(Node* newChild, Node* refChild);
};

//
//Definition and Implementation of a Document.
//
class Document : public NodeDefinition
{
  public:
    Document(DocumentType* theDoctype = NULL);

    Element* getDocumentElement();
    DocumentType* getDoctype();
    const DOMImplementation& getImplementation();

    //Factory functions for various node types
    DocumentFragment* createDocumentFragment();
    Element* createElement(const String& tagName);
    Attr* createAttribute(const String& name);
    Text* createTextNode(const String& theData);
    Comment* createComment(const String& theData);
    CDATASection* createCDATASection(const String& theData);
    ProcessingInstruction* createProcessingInstruction(const String& target,
                                                       const String& data);
    EntityReference* createEntityReference(const String& name);

    //Override functions to enforce the One Element rule for documents, as well
    //as limit documents to certain types of nodes.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);

    // Introduced in DOM Level 2
    Element* createElementNS(const String& aNamespaceURI,
                             const String& aTagName);

    Attr* createAttributeNS(const String& aNamespaceURI,
                            const String& aName);

    Element* getElementById(const String aID);

    //Override to return documentBaseURI
    String getBaseURI();

    PRInt32 namespaceURIToID(const String& aNamespaceURI);
    void namespaceIDToURI(PRInt32 aNamespaceID, String& aNamespaceURI);

  private:
    Element* documentElement;
    DocumentType* doctype;
    DOMImplementation implementation;

    // This class is friend to be able to set the documentBaseURI
    friend class XMLParser;
    String documentBaseURI;
};

//
//Definition and Implementation of an Element
//
class Element : public NodeDefinition
{
  public:
    Element(const String& tagName, Document* owner);
    Element(const String& aNamespaceURI, const String& aTagName,
            Document* aOwner);
    virtual ~Element();

    //Override insertBefore to limit Elements to having only certain nodes as
    //children
    Node* insertBefore(Node* newChild, Node* refChild);

    const String& getTagName();
    NamedNodeMap* getAttributes();
    const String& getAttribute(const String& name);
    void setAttribute(const String& name, const String& value);
    void setAttributeNS(const String& aNamespaceURI,
                        const String& aName,
                        const String& aValue);
    void removeAttribute(const String& name);
    Attr* getAttributeNode(const String& name);
    Attr* setAttributeNode(Attr* newAttr);
    Attr* removeAttributeNode(Attr* oldAttr);
    NodeList* getElementsByTagName(const String& name);
    void normalize();

    //txXPathNode functions override
    MBool getLocalName(txAtom** aLocalName);
    PRInt32 getNamespaceID();
    MBool getAttr(txAtom* aLocalName, PRInt32 aNSID, String& aValue);
    MBool hasAttr(txAtom* aLocalName, PRInt32 aNSID);

  private:
    AttrMap mAttributes;
    txAtom* mLocalName;
    PRInt32 mNamespaceID;
};

//
//Definition and Implementation of a Attr
//    NOTE:  For the time bing use just the default functionality found in the
//           NodeDefinition class
//
class Attr : public NodeDefinition
{
    // These need to be friend to be able to update the ownerElement
    friend class AttrMap;
    friend class Element;
  public:
    Attr(const String& name, Document* owner);
    Attr(const String& aNamespaceURI, const String& aName,
         Document* aOwner);
    virtual ~Attr();

    const String& getName() const;
    MBool getSpecified() const;
    const String& getValue();
    void setValue(const String& newValue);

    //Override the set and get member functions for a node's value to create a
    //new TEXT node when set, and to interpret its children when read.
    void setNodeValue(const String& nodeValue);
    const String& getNodeValue();

    //Override insertBefore to limit Attr to having only certain nodes as
    //children
    Node* insertBefore(Node* newChild, Node* refChild);

    //txXPathNode functions override
    MBool getLocalName(txAtom** aLocalName);
    PRInt32 getNamespaceID();
    Node* getXPathParent();

  private:
    Element* ownerElement;

    MBool specified;
    txAtom* mLocalName;
    PRInt32 mNamespaceID;
};

//
//Definition and Implementation of CharacterData.  This class mearly provides
//the interface and some default implementation.  It is not intended to be
//instantiated by users of the DOM
//
class CharacterData : public NodeDefinition
{
  public:
    const String& getData() const;
    void setData(const String& source);
    PRUint32 getLength() const;

    String& substringData(PRUint32 offset, PRUint32 count, String& dest);
    void appendData(const String& arg);
    void insertData(PRUint32 offset, const String& arg);
    void deleteData(PRUint32 offset, PRUint32 count);
    void replaceData(PRUint32 offset, PRUint32 count, const String& arg);

  protected:
    CharacterData(NodeType type, const String& name,
                   const String& value, Document* owner);
};

//
//Definition and Implementation of a Text node.  The bulk of the functionality
//comes from CharacterData and NodeDefinition.
//
class Text : public CharacterData
{
  public:
    Text(const String& theData, Document* owner);

    Text* splitText(PRUint32 offset);

    //Override "child manipulation" function since Text Nodes can not have
    //any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);

  protected:
    Text(NodeType type, const String& name, const String& value,
         Document* owner);
};

//
//Definition and Implementation of a Comment node.  All of the functionality is
//inherrited from CharacterData and NodeDefinition.
//
class Comment : public CharacterData
{
  public:
    Comment(const String& theData, Document* owner);

    //Override "child manipulation" function since Comment Nodes can not have
    //any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);
};

//
//Definition and Implementation of a CDATASection node.  All of the
//functionality is inherrited from Text, CharacterData, and NodeDefinition
//
class CDATASection : public Text
{
  public:
    CDATASection(const String& theData, Document* owner);

    //Override "child manipulation" function since CDATASection Nodes can not
    //have any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);
};

//
//Definition and Implemention of a ProcessingInstruction node.  Most
//functionality is inherrited from NodeDefinition.
//  The Target of a processing instruction is stored in the nodeName datamember
//  inherrited from NodeDefinition.
//  The Data of a processing instruction is stored in the nodeValue datamember
//  inherrited from NodeDefinition
//
class ProcessingInstruction : public NodeDefinition
{
  public:
    ProcessingInstruction(const String& theTarget, const String& theData,
                          Document* owner);
    ~ProcessingInstruction();

    const String& getTarget() const;
    const String& getData() const;

    void setData(const String& theData);

    //Override "child manipulation" function since ProcessingInstruction Nodes
    //can not have any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);

    //txXPathNode functions override
    MBool getLocalName(txAtom** aLocalName);

  private:
    txAtom* mLocalName;
};

//
//Definition and Implementation of a Notation.  Most functionality
//is inherrited from NodeDefinition.
//
class Notation : public NodeDefinition
{
  public:
    Notation(const String& name, const String& pubID,
             const String& sysID);

    const String& getPublicId() const;
    const String& getSystemId() const;

    //Override "child manipulation" function since Notation Nodes
    //can not have any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);

  private:
    String publicId;
    String systemId;
};

//
//Definition and Implementation of an Entity
//
class Entity : public NodeDefinition
{
  public:
    Entity(const String& name, const String& pubID,
           const String& sysID, const String& notName);

    const String& getPublicId() const;
    const String& getSystemId() const;
    const String& getNotationName() const;

    //Override insertBefore to limit Entity to having only certain nodes as
    //children
    Node* insertBefore(Node* newChild, Node* refChild);

  private:
    String publicId;
    String systemId;
    String notationName;
};

//
//Definition and Implementation of an EntityReference
//
class EntityReference : public NodeDefinition
{
  public:
    EntityReference(const String& name, Document* owner);

    //Override insertBefore to limit EntityReference to having only certain
    //nodes as children
    Node* insertBefore(Node* newChild, Node* refChild);
};

//
//Definition and Implementation of the DocumentType
//
class DocumentType : public NodeDefinition
{
  public:
    DocumentType(const String& name, NamedNodeMap* theEntities,
                 NamedNodeMap* theNotations);
    ~DocumentType();

    NamedNodeMap* getEntities();
    NamedNodeMap* getNotations();

    //Override "child manipulation" function since Notation Nodes
    //can not have any children.
    Node* insertBefore(Node* newChild, Node* refChild);
    Node* replaceChild(Node* newChild, Node* oldChild);
    Node* removeChild(Node* oldChild);
    Node* appendChild(Node* newChild);

  private:
    NamedNodeMap* entities;
    NamedNodeMap* notations;
};

class txNamespaceManager
{
public:
    static PRInt32 getNamespaceID(const String& aURI)
    {
        if (!mNamespaces && !init())
            return kNameSpaceID_Unknown;
        txListIterator nameIter(mNamespaces);
        PRInt32 id=0;
        String* uri;
        while (nameIter.hasNext()) {
            uri = (String*)nameIter.next();
            id++;
            if (uri->isEqual(aURI))
                return id;
        }
        uri = new String(aURI);
        NS_ASSERTION(uri, "Out of memory, namespaces are getting lost");
        if (!uri)
            return kNameSpaceID_Unknown;
        mNamespaces->add(uri);
        id++;
        return id;
    }

    static const String& getNamespaceURI(const PRInt32 aID)
    {
        // empty namespace, and errors
        if (aID <= 0)
            return NULL_STRING;
        if (!mNamespaces && !init())
            return NULL_STRING;
        txListIterator nameIter(mNamespaces);
        String* aURI = (String*)nameIter.advance(aID);
        if (aURI)
            return *aURI;
        return NULL_STRING;
    }

    static MBool init()
    {
        NS_ASSERTION(!mNamespaces,
                     "called without matching shutdown()");
        if (mNamespaces)
            return MB_TRUE;
        mNamespaces = new txList();
        if (!mNamespaces)
            return MB_FALSE;
        /*
         * Hardwiring some Namespace IDs.
         * no Namespace is 0
         * xmlns prefix is 1, mapped to http://www.w3.org/2000/xmlns/
         * xml prefix is 2, mapped to http://www.w3.org/XML/1998/namespace
         */
        String* XMLNSUri = new String("http://www.w3.org/2000/xmlns/");
        if (!XMLNSUri) {
            delete mNamespaces;
            mNamespaces = 0;
            return MB_FALSE;
        }
        mNamespaces->add(XMLNSUri);
        String* XMLUri = new String("http://www.w3.org/XML/1998/namespace");
        if (!XMLUri) {
            delete mNamespaces;
            mNamespaces = 0;
            return MB_FALSE;
        }
        mNamespaces->add(XMLUri);
        String* XSLTUri = new String("http://www.w3.org/1999/XSL/Transform");
        if (!XSLTUri) {
            delete mNamespaces;
            mNamespaces = 0;
            return MB_FALSE;
        }
        mNamespaces->add(XSLTUri);
        return MB_TRUE;
    }

    static void shutdown()
    {
        NS_ASSERTION(mNamespaces, "called without matching init()");
        if (!mNamespaces)
            return;
        txListIterator iter(mNamespaces);
        while (iter.hasNext())
            delete (String*)iter.next();
        delete mNamespaces;
        mNamespaces = NULL;
    }

private:
    static txList* mNamespaces;
};

#define TX_IMPL_DOM_STATICS \
    txList* txNamespaceManager::mNamespaces = 0

#endif
