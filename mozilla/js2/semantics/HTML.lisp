;;; The contents of this file are subject to the Mozilla Public
;;; License Version 1.1 (the "License"); you may not use this file
;;; except in compliance with the License. You may obtain a copy of
;;; the License at http://www.mozilla.org/MPL/
;;; 
;;; Software distributed under the License is distributed on an "AS
;;; IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
;;; implied. See the License for the specific language governing
;;; rights and limitations under the License.
;;; 
;;; The Original Code is the Language Design and Prototyping Environment.
;;; 
;;; The Initial Developer of the Original Code is Netscape Communications
;;; Corporation.  Portions created by Netscape Communications Corporation are
;;; Copyright (C) 1999 Netscape Communications Corporation.  All
;;; Rights Reserved.
;;; 
;;; Contributor(s):   Waldemar Horwat <waldemar@acm.org>

;;;
;;; HTML output generator
;;;
;;; Waldemar Horwat (waldemar@acm.org)
;;;


;;; ------------------------------------------------------------------------------------------------------
;;; ELEMENTS

(defstruct (html-element (:constructor make-html-element (name self-closing indent
                                                               newlines-before newlines-begin newlines-end newlines-after))
                         (:predicate html-element?))
  (name nil :type symbol :read-only t)               ;Name of the tag
  (self-closing nil :type bool :read-only t)         ;True if the closing tag should be omitted
  (indent nil :type integer :read-only t)            ;Number of spaces by which to indent this tag's contents in HTML source
  (newlines-before nil :type integer :read-only t)   ;Number of HTML source newlines preceding the opening tag
  (newlines-begin nil :type integer :read-only t)    ;Number of HTML source newlines immediately following the opening tag
  (newlines-end nil :type integer :read-only t)      ;Number of HTML source newlines immediately preceding the closing tag
  (newlines-after nil :type integer :read-only t))   ;Number of HTML source newlines following the closing tag


; Define symbol to refer to the given html-element.
(defun define-html (symbol newlines-before newlines-begin newlines-end newlines-after &key self-closing (indent 0))
  (setf (get symbol 'html-element) (make-html-element symbol self-closing indent
                                                      newlines-before newlines-begin newlines-end newlines-after)))


;;; ------------------------------------------------------------------------------------------------------
;;; ELEMENT DEFINITIONS

(define-html 'a 0 0 0 0)
(define-html 'b 0 0 0 0)
(define-html 'blockquote 1 0 0 1 :indent 2)
(define-html 'body 1 1 1 1)
(define-html 'br 0 0 0 1 :self-closing t)
(define-html 'code 0 0 0 0)
(define-html 'dd 1 0 0 1 :indent 2)
(define-html 'del 0 0 0 0)
(define-html 'div 1 0 0 1 :indent 2)
(define-html 'dl 1 0 0 2 :indent 2)
(define-html 'dt 1 0 0 1 :indent 2)
(define-html 'em 0 0 0 0)
(define-html 'h1 2 0 0 2 :indent 2)
(define-html 'h2 2 0 0 2 :indent 2)
(define-html 'h3 2 0 0 2 :indent 2)
(define-html 'h4 1 0 0 2 :indent 2)
(define-html 'h5 1 0 0 2 :indent 2)
(define-html 'h6 1 0 0 2 :indent 2)
(define-html 'head 1 1 1 2)
(define-html 'hr 1 0 0 1 :self-closing t)
(define-html 'html 0 1 1 1)
(define-html 'i 0 0 0 0)
(define-html 'li 1 0 0 1 :indent 2)
(define-html 'link 1 0 0 1 :self-closing t)
(define-html 'ol 1 1 1 2 :indent 2)
(define-html 'p 1 0 0 2)
(define-html 'script 0 0 0 0)
(define-html 'span 0 0 0 0)
(define-html 'strong 0 0 0 0)
(define-html 'sub 0 0 0 0)
(define-html 'sup 0 0 0 0)
(define-html 'table 1 1 1 2)
(define-html 'td 1 0 0 1 :indent 2)
(define-html 'th 1 0 0 1 :indent 2)
(define-html 'title 1 0 0 1)
(define-html 'tr 1 0 0 1 :indent 2)
(define-html 'u 0 0 0 0)
(define-html 'ul 1 1 1 2 :indent 2)
(define-html 'var 0 0 0 0)


;;; ------------------------------------------------------------------------------------------------------
;;; ATTRIBUTES

;;; The following element attributes require their values to always be in quotes.
(dolist (attribute '(alt href name))
  (setf (get attribute 'quoted-attribute) t))


;;; ------------------------------------------------------------------------------------------------------
;;; ENTITIES

(defvar *html-entities-list*
  '((#\& . "amp")
    (#\" . "quot")
    (#\< . "lt")
    (#\> . "gt")
    (nbsp . "nbsp")))

(defvar *html-entities-hash* (make-hash-table))

(dolist (entity-binding *html-entities-list*)
  (setf (gethash (first entity-binding) *html-entities-hash*) (rest entity-binding)))


; Return a freshly consed list of <html-source> that represent the characters in the string except that
; '&', '<', and '>' are replaced by their entities and spaces are replaced by the entity
; given by the space parameter (which should be either 'space or 'nbsp).
(defun escape-html-characters (string space)
  (let ((html-sources nil))
    (labels
      ((escape-remainder (start)
         (let ((i (position-if #'(lambda (char) (member char '(#\& #\< #\> #\space))) string :start start)))
           (if i
             (let ((char (char string i)))
               (unless (= i start)
                 (push (subseq string start i) html-sources))
               (push (if (eql char #\space) space char) html-sources)
               (escape-remainder (1+ i)))
             (push (if (zerop start) string (subseq string start)) html-sources)))))
      (unless (zerop (length string))
        (escape-remainder 0))
      (nreverse html-sources))))


; Escape all content strings in the html-source, while interpreting :nowrap, :wrap, and :none pseudo-tags.
; Return a freshly consed list of html-sources.
(defun escape-html-source (html-source space)
  (cond
   ((stringp html-source)
    (escape-html-characters html-source space))
   ((or (characterp html-source) (symbolp html-source) (integerp html-source))
    (list html-source))
   ((consp html-source)
    (let ((tag (first html-source))
          (contents (rest html-source)))
      (case tag
        (:none (mapcan #'(lambda (html-source) (escape-html-source html-source space)) contents))
        (:nowrap (mapcan #'(lambda (html-source) (escape-html-source html-source 'nbsp)) contents))
        (:wrap (mapcan #'(lambda (html-source) (escape-html-source html-source 'space)) contents))
        (t (list (cons tag
                       (mapcan #'(lambda (html-source) (escape-html-source html-source space)) contents)))))))
   (t (error "Bad html-source: ~S" html-source))))


; Escape all content strings in the html-source, while interpreting :nowrap, :wrap, and :none pseudo-tags.
(defun escape-html (html-source)
  (let ((results (escape-html-source html-source 'space)))
    (assert-true (= (length results) 1))
    (first results)))


;;; ------------------------------------------------------------------------------------------------------
;;; HTML WRITER

;; <html-source> has one of the following formats:
;;   <string>                                                  ;String to be printed literally
;;   <symbol>                                                  ;Named entity
;;   <integer>                                                 ;Numbered entity
;;   space                                                     ;Space or newline
;;   (<tag> <html-source> ... <html-source>)                   ;Tag and its contents
;;   ((:nest <tag> ... <tag>) <html-source> ... <html-source>) ;Equivalent to (<tag> (... (<tag> <html-source> ... <html-source>)))
;;
;; <tag> has one of the following formats:
;;   <symbol>                                                  ;Tag with no attributes
;;   (<symbol> <attribute> ... <attribute>)                    ;Tag with attributes
;;   :nowrap                                                   ;Pseudo-tag indicating that spaces in contents should be non-breaking
;;   :wrap                                                     ;Pseudo-tag indicating that spaces in contents should be breaking
;;   :none                                                     ;Pseudo-tag indicating no tag -- the contents should be inlined
;;
;; <attribute> has one of the following formats:
;;   (<symbol> <string>)                                       ;Attribute name and value
;;   (<symbol>)                                                ;Attribute name with omitted value


(defparameter *html-right-margin* 100)
(defparameter *allow-line-breaks-in-tags* nil) ;Allow line breaks in tags between attributes?

(defvar *current-html-pos*)          ;Number of characters written to the current line of the stream; nil if *current-html-newlines* is nonzero
(defvar *current-html-pending*)      ;String following a space or newline pending to be printed on the current line or nil if none
(defvar *current-html-indent*)       ;Indent to use for emit-html-newlines-and-indent calls
(defvar *current-html-newlines*)     ;Number of consecutive newlines just written to the stream; zero if last character wasn't a newline


; Flush *current-html-pending* onto the stream.
(defun flush-current-html-pending (stream)
  (when *current-html-pending*
    (unless (zerop (length *current-html-pending*))
      (write-char #\space stream)
      (write-string *current-html-pending* stream)
      (incf *current-html-pos* (1+ (length *current-html-pending*))))
    (setq *current-html-pending* nil)))


; Emit n-newlines onto the stream and indent the next line by *current-html-indent* spaces.
(defun emit-html-newlines-and-indent (stream n-newlines)
  (decf n-newlines *current-html-newlines*)
  (when (plusp n-newlines)
    (flush-current-html-pending stream)
    (dotimes (i n-newlines)
      (write-char #\newline stream))
    (incf *current-html-newlines* n-newlines)
    (setq *current-html-pos* nil)))


; Write the string to the stream, observing *current-html-pending* and *current-html-pos*.
(defun write-html-string (stream html-string)
  (unless (zerop (length html-string))
    (unless *current-html-pos*
      (setq *current-html-newlines* 0)
      (write-string (make-string *current-html-indent* :initial-element #\space) stream)
      (setq *current-html-pos* *current-html-indent*))
    (if *current-html-pending*
      (progn
        (setq *current-html-pending* (if (zerop (length *current-html-pending*))
                                       html-string
                                       (concatenate 'string *current-html-pending* html-string)))
        (when (>= (+ *current-html-pos* (length *current-html-pending*)) *html-right-margin*)
          (write-char #\newline stream)
          (write-string *current-html-pending* stream)
          (setq *current-html-pos* (length *current-html-pending*))
          (setq *current-html-pending* nil)))
      (progn
        (write-string html-string stream)
        (incf *current-html-pos* (length html-string))))))


; Return true if the value string contains a character that would require an attribute to be quoted.
; For convenience, this returns true if value contains a period, even though strictly speaking periods do
; not force quoting.
(defun attribute-value-needs-quotes (value)
  (dotimes (i (length value) nil)
    (let ((ch (char value i)))
      (unless (or (char<= #\0 ch #\9) (char<= #\A ch #\Z) (char<= #\a ch #\z) (char= ch #\-))
        (return t)))))


; Emit the html tag with the given tag-symbol (name), attributes, and contents.
(defun write-html-tag (stream tag-symbol attributes contents)
  (let ((element (assert-non-null (get tag-symbol 'html-element))))
    (emit-html-newlines-and-indent stream (html-element-newlines-before element))
    (write-html-string stream (format nil "<~A" (html-element-name element)))
    (let ((*current-html-indent* (+ *current-html-indent* (html-element-indent element))))
      (dolist (attribute attributes)
        (let ((name (first attribute))
              (value (second attribute)))
          (write-html-source stream (if *allow-line-breaks-in-tags* 'space #\space))
          (write-html-string stream (string-downcase (symbol-name name)))
          (when value
            (write-html-string
             stream
             (format nil
                     (if (or (attribute-value-needs-quotes value)
                             (get name 'quoted-attribute))
                       "=\"~A\""
                       "=~A")
                     value)))))
      (write-html-string stream ">")
      (emit-html-newlines-and-indent stream (html-element-newlines-begin element))
      (dolist (html-source contents)
        (write-html-source stream html-source)))
    (unless (html-element-self-closing element)
      (emit-html-newlines-and-indent stream (html-element-newlines-end element))
      (write-html-string stream (format nil "</~A>" (html-element-name element))))
    (emit-html-newlines-and-indent stream (html-element-newlines-after element))))


; Write html-source to the character stream.
(defun write-html-source (stream html-source)
  (cond
   ((stringp html-source)
    (write-html-string stream html-source))
   ((eq html-source 'space)
    (when (zerop *current-html-newlines*)
      (flush-current-html-pending stream)
      (setq *current-html-pending* "")))
   ((or (characterp html-source) (symbolp html-source))
    (let ((entity-name (gethash html-source *html-entities-hash*)))
      (cond
       (entity-name
        (write-html-string stream (format nil "&~A;" entity-name)))
       ((characterp html-source)
        (write-html-string stream (string html-source)))
       (t (error "Bad html-source ~S" html-source)))))
   ((integerp html-source)
    (assert-true (and (>= html-source 0) (< html-source 65536)))
    (write-html-string stream (format nil "&#~D;" html-source)))
   ((consp html-source)
    (let ((tag (first html-source))
          (contents (rest html-source)))
      (if (consp tag)
        (write-html-tag stream (first tag) (rest tag) contents)
        (write-html-tag stream tag nil contents))))
   (t (error "Bad html-source: ~S" html-source))))


; Write the top-level html-source to the character stream.
(defun write-html (html-source &optional (stream t))
  (with-standard-io-syntax
    (let ((*print-readably* nil)
          (*print-escape* nil)
          (*print-case* :upcase)
          (*current-html-pos* nil)
          (*current-html-pending* nil)
          (*current-html-indent* 0)
          (*current-html-newlines* 9999))
      (write-string "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\" \"http://www.w3.org/TR/REC-html40/loose.dtd\">" stream)
      (write-char #\newline stream)
      (write-html-source stream (escape-html html-source)))))


; Write html to the text file with the given name (relative to the
; local directory).
(defun write-html-to-local-file (filename html)
  (with-open-file (stream (filename-to-semantic-engine-pathname filename)
                          :direction :output
                          :if-exists :supersede
                          #+mcl :mac-file-creator #+mcl "MOSS")
    (write-html html stream)))


; Expand the :nest constructs inside html-source.
(defun unnest-html-source (html-source)
  (labels
    ((unnest-tags (tags contents)
       (assert-true tags)
       (cons (first tags)
             (if (endp (rest tags))
               contents
               (list (unnest-tags (rest tags) contents))))))
    (if (consp html-source)
      (let ((tag (first html-source))
            (contents (rest html-source)))
        (if (and (consp tag) (eq (first tag) ':nest))
          (unnest-html-source (unnest-tags (rest tag) contents))
          (cons tag (mapcar #'unnest-html-source contents))))
      html-source)))


;;; ------------------------------------------------------------------------------------------------------
;;; HTML MAPPINGS

(defparameter *html-definitions*
  '(((:new-line t) (br))
    
    ;Misc.
    (:spc nbsp)
    (:tab2 nbsp nbsp)
    (:tab3 nbsp nbsp nbsp)
    (:nbhy "-")             ;Non-breaking hyphen
    
    ;Symbols (-10 suffix means 10-point, etc.)
    ((:bullet 1) (:script "document.write(U_bull)"))                    ;#x2022
    ((:minus 1) "-")
    ((:not-equal 1) (:script "document.write(U_ne)"))                   ;#x2260
    ((:less-or-equal 1) (:script "document.write(U_le)"))               ;#x2264
    ((:greater-or-equal 1) (:script "document.write(U_ge)"))            ;#x2265
    ((:infinity 1) (:script "document.write(U_infin)"))                 ;#x221E
    ((:left-single-quote 1) #x2018)
    ((:right-single-quote 1) #x2019)
    ((:left-double-quote 1) #x201C)
    ((:right-double-quote 1) #x201D)
    ((:left-angle-quote 1) #x00AB)
    ((:right-angle-quote 1) #x00BB)
    ((:bottom-10 1) (:script "document.write(U_perp)"))                 ;#x22A5
    ((:vector-assign-10 1) (:script "document.write(U_larr)"))          ;#x2190
    ((:up-arrow-10 1) (:script "document.write(U_uarr)"))               ;#x2191
    ((:function-arrow-10 2) (:script "document.write(U_rarr)"))         ;#x2192
    ((:cartesian-product-10 2) (:script "document.write(U_times)"))     ;#x00D7
    ((:identical-10 2) (:script "document.write(U_equiv)"))             ;#x2261
    ((:circle-plus-10 2) (:script "document.write(U_oplus)"))           ;#x2295
    ((:empty-10 2) (:script "document.write(U_empty)"))                 ;#x2205
    ((:intersection-10 1) (:script "document.write(U_cap)"))            ;#x2229
    ((:union-10 1) (:script "document.write(U_cup)"))                   ;#x222A
    ((:member-10 2) (:script "document.write(U_isin)"))                 ;#x2208
    ((:not-member-10 2) (:script "document.write(U_notin)"))            ;#x2209
    ((:derives-10 2) (:script "document.write(U_rArr)"))                ;#x21D2
    ((:left-triangle-bracket-10 1) (:script "document.write(U_lang)"))  ;#x2329
    ((:right-triangle-bracket-10 1) (:script "document.write(U_rang)")) ;#x232A
    
    ((:alpha 1) (:symbol "a"))
    ((:beta 1) (:symbol "b"))
    ((:chi 1) (:symbol "c"))
    ((:delta 1) (:symbol "d"))
    ((:epsilon 1) (:symbol "e"))
    ((:phi 1) (:symbol "f"))
    ((:gamma 1) (:symbol "g"))
    ((:eta 1) (:symbol "h"))
    ((:iota 1) (:symbol "i"))
    ((:kappa 1) (:symbol "k"))
    ((:lambda 1) (:symbol "l"))
    ((:mu 1) (:symbol "m"))
    ((:nu 1) (:symbol "n"))
    ((:omicron 1) (:symbol "o"))
    ((:pi 1) (:symbol "p"))
    ((:theta 1) (:symbol "q"))
    ((:rho 1) (:symbol "r"))
    ((:sigma 1) (:symbol "s"))
    ((:tau 1) (:symbol "t"))
    ((:upsilon 1) (:symbol "u"))
    ((:omega 1) (:symbol "w"))
    ((:xi 1) (:symbol "x"))
    ((:psi 1) (:symbol "y"))
    ((:zeta 1) (:symbol "z"))
    
    ;Block Styles
    (:body-text p)
    (:section-heading h2)
    (:subsection-heading h3)
    (:grammar-header h4)
    (:grammar-rule (:nest :nowrap (div (class "grammar-rule"))))
    (:grammar-lhs (:nest :nowrap (div (class "grammar-lhs"))))
    (:grammar-lhs-last :grammar-lhs)
    (:grammar-rhs (:nest :nowrap (div (class "grammar-rhs"))))
    (:grammar-rhs-last :grammar-rhs)
    (:grammar-argument (:nest :nowrap (div (class "grammar-argument"))))
    (:semantics (:nest :nowrap (p (class "semantics"))))
    (:semantics-next (:nest :nowrap (p (class "semantics-next"))))
    
    ;Inline Styles
    (:script (script (type "text/javascript")))
    (:symbol (span (class "symbol")))
    (:character-literal code)
    (:character-literal-control (span (class "control")))
    (:terminal (span (class "terminal")))
    (:terminal-keyword (code (class "terminal-keyword")))
    (:nonterminal (var (class "nonterminal")))
    (:nonterminal-attribute (span (class "nonterminal-attribute")))
    (:nonterminal-argument (span (class "nonterminal-argument")))
    (:semantic-keyword (span (class "semantic-keyword")))
    (:type-expression (span (class "type-expression")))
    (:type-name (span (class "type-name")))
    (:field-name (span (class "field-name")))
    (:global-variable (span (class "global-variable")))
    (:local-variable (span (class "local-variable")))
    (:action-name (span (class "action-name")))
    (:text :wrap)
    
    ;Specials
    (:invisible del)
    ((:but-not 6) (b "except"))
    ((:begin-negative-lookahead 13) "[lookahead" :not-member-10 "{")
    ((:end-negative-lookahead 2) "}]")
    ((:no-line-break 15) "[no" nbsp "line" nbsp "break]")
    (:subscript sub)
    (:superscript sup)
    (:plain-subscript :subscript)
    ((:action-begin 1) "[")
    ((:action-end 1) "]")
    ((:vector-begin 1) (b "["))
    ((:vector-end 1) (b "]"))
    ((:empty-vector 2) (b "[]"))
    ((:vector-append 2) :circle-plus-10)
    ((:tuple-begin 1) (b :left-triangle-bracket-10))
    ((:tuple-end 1) (b :right-triangle-bracket-10))
    ((:true 4) (:global-variable "true"))
    ((:false 5) (:global-variable "false"))
    ))


;;; ------------------------------------------------------------------------------------------------------
;;; HTML STREAMS

(defstruct (html-stream (:include markup-stream)
                        (:constructor allocate-html-stream (env head tail level logical-position anchors))
                        (:copier nil)
                        (:predicate html-stream?))
  (anchors nil :type list :read-only t))  ;A mutable cons cell for accumulating anchors at the beginning of a paragraph
;                                         ;or nil if not inside a paragraph.


(defmethod print-object ((html-stream html-stream) stream)
  (print-unreadable-object (html-stream stream :identity t)
    (write-string "html-stream" stream)))


; Make a new, empty, open html-stream with the given definitions for its markup-env.
(defun make-html-stream (markup-env level logical-position anchors)
  (let ((head (list nil)))
    (allocate-html-stream markup-env head head level logical-position anchors)))


; Make a new, empty, open, top-level html-stream with the given definitions
; for its markup-env.  If links is true, allow links.
(defun make-top-level-html-stream (html-definitions links)
  (let ((head (list nil))
        (markup-env (make-markup-env links)))
    (markup-env-define-alist markup-env html-definitions)
    (allocate-html-stream markup-env head head *markup-stream-top-level* nil nil)))


; Return the approximate width of the html item; return t if it is a line break.
; Also allow html tags as long as they do not contain line breaks.
(defmethod markup-group-width ((html-stream html-stream) item)
  (if (consp item)
    (reduce #'+ (rest item) :key #'(lambda (subitem) (markup-group-width html-stream subitem)))
    (markup-width html-stream item)))


; Create a top-level html-stream and call emitter to emit its contents.
; emitter takes one argument -- an html-stream to which it should emit paragraphs.
; Return the top-level html-stream.  If links is true, allow links.
(defun depict-html-top-level (title links emitter)
  (let ((html-stream (make-top-level-html-stream *html-definitions* links)))
    (markup-stream-append1 html-stream 'html)
    (depict-block-style (html-stream 'head)
      (depict-block-style (html-stream 'title)
        (markup-stream-append1 html-stream title))
      (markup-stream-append1 html-stream '((link (rel "stylesheet") (href "../styles.css"))))
      (markup-stream-append1 html-stream '((script (type "text/javascript") (language "JavaScript1.2") (src "../unicodeCompatibility.js")))))
    (depict-block-style (html-stream 'body)
      (funcall emitter html-stream))
    (let ((links (markup-env-links (html-stream-env html-stream))))
      (warn-missing-links links))
    html-stream))


; Create a top-level html-stream and call emitter to emit its contents.
; emitter takes one argument -- an html-stream to which it should emit paragraphs.
; Write the resulting html to the text file with the given name (relative to the
; local directory).
; If links is true, allow links.  If external-link-base is also provided, emit links for
; predefined items and assume that they are located on the page specified by the
; external-link-base string.
(defun depict-html-to-local-file (filename title links emitter &key external-link-base)
  (let* ((*external-link-base* external-link-base)
         (top-html-stream (depict-html-top-level title links emitter)))
    (write-html-to-local-file filename (markup-stream-output top-html-stream)))
  filename)


; Return the markup accumulated in the markup-stream after expanding all of its macros.
; The markup-stream is closed after this function is called.
(defmethod markup-stream-output ((html-stream html-stream))
  (unnest-html-source
   (markup-env-expand (markup-stream-env html-stream) (markup-stream-unexpanded-output html-stream) '(:none :nowrap :wrap :nest))))



(defmethod depict-block-style-f ((html-stream html-stream) block-style emitter)
  (assert-true (<= (markup-stream-level html-stream) *markup-stream-paragraph-level*))
  (assert-true (and block-style (symbolp block-style)))
  (let ((inner-html-stream (make-html-stream (markup-stream-env html-stream) *markup-stream-paragraph-level* nil nil)))
    (markup-stream-append1 inner-html-stream block-style)
    (prog1
      (funcall emitter inner-html-stream)
      (markup-stream-append1 html-stream (markup-stream-unexpanded-output inner-html-stream)))))


(defmethod depict-paragraph-f ((html-stream html-stream) paragraph-style emitter)
  (assert-true (= (markup-stream-level html-stream) *markup-stream-paragraph-level*))
  (assert-true (and paragraph-style (symbolp paragraph-style)))
  (let* ((anchors (list 'anchors))
         (inner-html-stream (make-html-stream (markup-stream-env html-stream)
                                              *markup-stream-content-level*
                                              (make-logical-position)
                                              anchors)))
    (prog1
      (funcall emitter inner-html-stream)
      (markup-stream-append1 html-stream (cons paragraph-style
                                               (nreconc (cdr anchors)
                                                        (markup-stream-unexpanded-output inner-html-stream)))))))


(defmethod depict-char-style-f ((html-stream html-stream) char-style emitter)
  (assert-true (>= (markup-stream-level html-stream) *markup-stream-content-level*))
  (assert-true (and char-style (symbolp char-style)))
  (let ((inner-html-stream (make-html-stream (markup-stream-env html-stream)
                                             *markup-stream-content-level*
                                             (markup-stream-logical-position html-stream)
                                             (html-stream-anchors html-stream))))
    (markup-stream-append1 inner-html-stream char-style)
    (prog1
      (funcall emitter inner-html-stream)
      (markup-stream-append1 html-stream (markup-stream-unexpanded-output inner-html-stream)))))


(defmethod depict-anchor ((html-stream html-stream) link-prefix link-name duplicate)
  (assert-true (= (markup-stream-level html-stream) *markup-stream-content-level*))
  (let* ((links (markup-env-links (html-stream-env html-stream)))
         (name (record-link-definition links link-prefix link-name duplicate)))
    (when name
      (push (list (list 'a (list 'name name))) (cdr (html-stream-anchors html-stream))))))


(defmethod depict-link-reference-f ((html-stream html-stream) link-prefix link-name external emitter)
  (assert-true (= (markup-stream-level html-stream) *markup-stream-content-level*))
  (let* ((links (markup-env-links (html-stream-env html-stream)))
         (href (record-link-reference links link-prefix link-name external)))
    (if href
      (let ((inner-html-stream (make-html-stream (markup-stream-env html-stream)
                                                 *markup-stream-content-level*
                                                 (markup-stream-logical-position html-stream)
                                                 (html-stream-anchors html-stream))))
        (markup-stream-append1 inner-html-stream (list 'a (list 'href href)))
        (prog1
          (funcall emitter inner-html-stream)
          (markup-stream-append1 html-stream (markup-stream-unexpanded-output inner-html-stream))))
      (funcall emitter html-stream))))


#|
(write-html
 '(html
   (head
    (:nowrap (title "This is my title!<>")))
   ((body (atr1 "abc") (beta) (qq))
    "My page this  is  " (br) (p))))
|#
