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
;;; ECMAScript semantic calculus markup emitters
;;;
;;; Waldemar Horwat (waldemar@acm.org)
;;;


(defvar *hide-$-nonterminals* t) ; Should rules and actions expanding nonterminals starting with $ be invisible?

(defvar *styled-text-world*)


(defun hidden-nonterminal? (general-nonterminal)
  (and *hide-$-nonterminals*
       (eql (first-symbol-char (general-grammar-symbol-symbol general-nonterminal)) #\$)))


; Return true if this action call should be replaced by a plain reference to the action's nonterminal.
(defun default-action? (action-name)
  (equal (symbol-name action-name) "$DEFAULT-ACTION"))


;;; ------------------------------------------------------------------------------------------------------
;;; SEMANTIC DEPICTION UTILITIES

(defparameter *semantic-keywords*
  '(not and or xor mod new
    type tag record
    function
    begin end nothing
    if then elsif else
    while do
    invariant
    return
    throw try catch
    case of))

; Emit markup for one of the semantic keywords, as specified by keyword-symbol.
; space can be either nil, :before, or :after to indicate space placement.
(defun depict-semantic-keyword (markup-stream keyword-symbol space)
  (assert-true (and (member keyword-symbol *semantic-keywords*)
                    (member space '(nil :before :after))))
  (when (eq space :before)
    (depict-space markup-stream))
  (depict-char-style (markup-stream :semantic-keyword)
    (depict markup-stream (string-downcase (symbol-name keyword-symbol))))
  (when (eq space :after)
    (depict-space markup-stream)))


; If test is true, depict an opening parenthesis, evaluate body, and depict a closing
; parentheses.  Otherwise, just evaluate body.
; Return the result value of body.
(defmacro depict-optional-parentheses ((markup-stream test) &body body)
  (let ((temp (gensym "PAREN")))
    `(let ((,temp ,test))
       (when ,temp
         (depict ,markup-stream "("))
       (prog1
         (progn ,@body)
         (when ,temp
           (depict ,markup-stream ")"))))))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICT-ENV

; A depict-env holds state that helps in depicting a grammar or lexer.
(defstruct (depict-env (:constructor make-depict-env (visible-semantics)))
  (visible-semantics t :type bool)                               ;Nil if semantics are not to be depicted
  (grammar-info nil :type (or null grammar-info))                ;The current grammar-info or nil if none
  (seen-nonterminals nil :type (or null hash-table))             ;Hash table (nonterminal -> t) of nonterminals already depicted
  (seen-grammar-arguments nil :type (or null hash-table))        ;Hash table (grammar-argument -> t) of grammar-arguments already depicted
  (mode nil :type (member nil :syntax :semantics))               ;Current heading (:syntax or :semantics) or nil if none
  (pending-actions-reverse nil :type list))                      ;Reverse-order list of closures of actions pending for a %print-actions


(defun checked-depict-env-grammar-info (depict-env)
  (or (depict-env-grammar-info depict-env)
      (error "Grammar needed")))


; Set the mode to the given mode without emitting any headings.
; Return true if the contents should be visible, nil if not.
(defun quiet-depict-mode (depict-env mode)
  (unless (member mode '(nil :syntax :semantics))
    (error "Bad mode: ~S" mode))
  (setf (depict-env-mode depict-env) mode)
  (or (depict-env-visible-semantics depict-env)
      (not (eq mode :semantics))))


; Set the mode to the given mode, emitting a heading if necessary.
; Return true if the contents should be visible, nil if not.
(defun depict-mode (markup-stream depict-env mode)
  (unless (eq mode (depict-env-mode depict-env))
    (when (depict-env-visible-semantics depict-env)
      (ecase mode
        (:syntax (depict-paragraph (markup-stream :grammar-header)
                   (depict markup-stream "Syntax")))
        (:semantics (depict-paragraph (markup-stream :grammar-header)
                      (depict markup-stream "Semantics")))
        ((nil)))))
  (quiet-depict-mode depict-env mode))


; Emit markup paragraphs for a command.
(defun depict-command (markup-stream world depict-env command)
  (handler-bind ((error #'(lambda (condition)
                            (declare (ignore condition))
                            (format *error-output* "~&While depicting: ~:W~%" command))))
    (let ((depictor (and (consp command)
                         (identifier? (first command))
                         (get (world-intern world (first command)) :depict-command))))
      (if depictor
        (apply depictor markup-stream world depict-env (rest command))
        (error "Bad command: ~S" command)))))


; Emit markup paragraphs for a list of commands.
(defun depict-commands (markup-stream world depict-env commands)
  (dolist (command commands)
    (depict-command markup-stream world depict-env command)))


; Emit markup paragraphs for the world's commands.
(defun depict-world-commands (markup-stream world &key (visible-semantics t))
  (let ((depict-env (make-depict-env visible-semantics)))
    (depict-commands markup-stream world depict-env (world-commands-source world))
    (depict-clear-grammar markup-stream world depict-env)))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING TAGS


(defparameter *tag-name-special-cases*
  '((:+zero "PlusZero" "+zero")
    (:-zero "MinusZero" (:minus "zero"))
    (:+infinity "PlusInfinity" ("+" :infinity))
    (:-infinity "MinusInfinity" (:minus :infinity))
    (:nan "NaN" "NaN")))


; Return two values:
;   A string to use as the tag's link name;
;   A depict item or list of items forming the tag's name.
(defun tag-link-name-and-name (tag)
  (let ((special-case (assoc (tag-keyword tag) *tag-name-special-cases*)))
    (if special-case
      (values (second special-case) (third special-case))
      (let ((name (symbol-lower-mixed-case-name (tag-name tag))))
        (values name name)))))


; Emit markup for a tag.
; link should be one of:
;   :reference   if this is a reference or external reference of this tag;
;   :definition  if this is a definition of this tag;
;   nil          if this use of the tag should not be cross-referenced.
(defun depict-tag-name (markup-stream tag link)
  (when (eq link :reference)
    (setq link (tag-link tag)))
  (multiple-value-bind (link-name name) (tag-link-name-and-name tag)
    (depict-link (markup-stream link "R-" link-name nil)
      (depict-char-style (markup-stream :tag-name)
        (depict-item-or-list markup-stream name)))))


; Emit markup for a tag's label, which must be a symbol.
; link should be one of:
;   :reference   if this is a reference or external reference to this label;
;   nil          if this use of the label should not be cross-referenced.
(defun depict-label-name (markup-stream tag label link)
  (unless (tag-find-field tag label)
    (error "Tag ~A doesn't have label ~A" tag label))
  (when (eq link :reference)
    (setq link (tag-link tag)))
  (depict-link (markup-stream link "R-" (tag-link-name-and-name tag) nil)
    (depict-char-style (markup-stream :field-name)
      (depict markup-stream (symbol-lower-mixed-case-name label)))))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING TYPES

;;; The level argument indicates what kinds of component types may be represented without being placed
;;; in parentheses.

(defparameter *type-level* (make-partial-order))
(def-partial-order-element *type-level* %%primary%%)                              ;id, tuple, (type)
(def-partial-order-element *type-level* %%suffix%% %%primary%%)                   ;type[], type{}
(def-partial-order-element *type-level* %%function%% %%suffix%%)                  ;type x type -> type
(def-partial-order-element *type-level* %%type%% %%function%%)                    ;type U type


; Emit markup for the name of a type, which must be a symbol.
; link should be one of:
;   :reference   if this is a reference of this type name;
;   :external    if this is an external reference of this type name;
;   :definition  if this is a definition of this type name;
;   nil          if this use of the type name should not be cross-referenced.
(defun depict-type-name (markup-stream type-name link)
  (let ((name (symbol-upper-mixed-case-name type-name)))
    (depict-link (markup-stream link "T-" name nil)
      (depict-char-style (markup-stream :type-name)
        (depict markup-stream name)))))


; If level < threshold, depict an opening parenthesis, evaluate body, and depict a closing
; parentheses.  Otherwise, just evaluate body.
; Return the result value of body.
(defmacro depict-type-parentheses ((markup-stream level threshold) &body body)
  `(depict-optional-parentheses (,markup-stream (partial-order-< ,level ,threshold))
     ,@body))


; Emit markup for the given type expression.  level is non-nil if this is a recursive
; call to depict-type-expr for which the markup-stream's style is :type-expression.
; In this case level indicates the binding level imposed by the enclosing type expression.
(defun depict-type-expr (markup-stream world type-expr &optional level)
  (cond
   ((identifier? type-expr)
    (let ((type-name (world-intern world type-expr)))
      (depict-type-name markup-stream type-expr (if (symbol-type-user-defined type-name) :reference :external))))
   ((consp type-expr)
    (let ((depictor (get (world-intern world (first type-expr)) :depict-type-constructor)))
      (if level
        (apply depictor markup-stream world level (rest type-expr))
        (depict-char-style (markup-stream :type-expression)
          (apply depictor markup-stream world %%type%% (rest type-expr))))))
   (t (error "Bad type expression: ~S" type-expr))))


; (-> (<arg-type1> ... <arg-typen>) <result-type>)
;   "<arg-type1> x ... x <arg-typen> -> <result-type>"
(defun depict--> (markup-stream world level arg-type-exprs result-type-expr)
  (depict-type-parentheses (markup-stream level %%function%%)
    (depict-list markup-stream
                 #'(lambda (markup-stream arg-type-expr)
                     (depict-type-expr markup-stream world arg-type-expr %%suffix%%))
                 arg-type-exprs
                 :separator '(" " :cartesian-product-10 " ")
                 :empty "()")
    (depict markup-stream " " :function-arrow-10 " ")
    (if (eq result-type-expr 'void)
      (depict markup-stream "()")
      (depict-type-expr markup-stream world result-type-expr %%suffix%%))))


; (vector <element-type>)
;   "<element-type>[]"
(defun depict-vector (markup-stream world level element-type-expr)
  (depict-type-parentheses (markup-stream level %%suffix%%)
    (depict-type-expr markup-stream world element-type-expr %%suffix%%)
    (depict markup-stream "[]")))


; (set <element-type>)
;   "<element-type>{}"
(defun depict-set (markup-stream world level element-type-expr)
  (depict-type-parentheses (markup-stream level %%suffix%%)
    (depict-type-expr markup-stream world element-type-expr %%suffix%%)
    (depict markup-stream "{}")))


; (tag <tag> ... <tag>)
;   "{<tag> *, ..., <tag> *}"
(defun depict-tag-type (markup-stream world level &rest tag-names)
  (declare (ignore level))
  (depict-list
   markup-stream
   #'(lambda (markup-stream tag-name)
       (let* ((tag (scan-tag world tag-name))
              (mutable (tag-mutable tag)))
         (depict-tag-name markup-stream tag :reference)
         (unless (tag-keyword tag)
           (depict markup-stream
                   (if mutable :record-begin :tuple-begin)
                   "..."
                   (if mutable :record-end :tuple-end)))))
   tag-names
   :indent 1
   :prefix "{"
   :suffix "}"
   :separator ","
   :break 1))


; (union <type1> ... <typen>)
;   "<type1> U ... U <typen>"
;   "{}" if no types are given
(defun depict-union (markup-stream world level &rest type-exprs)
  (cond
   ((endp type-exprs) (depict markup-stream "{}"))
   ((endp (cdr type-exprs)) (depict-type-expr markup-stream world (first type-exprs) level))
   (t (depict-type-parentheses (markup-stream level %%type%%)
        (depict-list markup-stream
                     #'(lambda (markup-stream type-expr)
                         (depict-type-expr markup-stream world type-expr %%function%%))
                     type-exprs
                     :indent 0
                     :separator '(" " :union-10)
                     :break 1)))))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING EXPRESSIONS


;;; The level argument indicates what kinds of subexpressions may be represented without being placed
;;; in parentheses (or on a separate line for the case of function and if/then/else).


; If primitive-level is not a superset of threshold, depict an opening parenthesis,
; evaluate body, and depict a closing parentheses.  Otherwise, just evaluate body.
; Return the result value of body.
(defmacro depict-expr-parentheses ((markup-stream primitive-level threshold) &body body)
  `(depict-optional-parentheses (,markup-stream (partial-order-< ,primitive-level ,threshold))
     ,@body))


; Emit markup for the name of a global variable, which must be a symbol.
; link should be one of:
;   :reference   if this is a reference of this global variable;
;   :external    if this is an external reference of this global variable;
;   :definition  if this is a definition of this global variable;
;   nil          if this use of the global variable should not be cross-referenced.
(defun depict-global-variable (markup-stream global-name link)
  (let ((name (symbol-lower-mixed-case-name global-name)))
    (depict-link (markup-stream link "V-" name nil)
      (depict-char-style (markup-stream :global-variable)
        (depict markup-stream name)))))


; Emit markup for the name of a local variable, which must be a symbol.
(defun depict-local-variable (markup-stream name)
  (depict-char-style (markup-stream :local-variable)
    (depict markup-stream (symbol-lower-mixed-case-name name))))


; Emit markup for the name of an action, which must be a symbol.
(defun depict-action-name (markup-stream action-name)
  (depict-char-style (markup-stream :action-name)
    (depict markup-stream (symbol-upper-mixed-case-name action-name))))


; Emit markup for the value constant.
(defun depict-constant (markup-stream constant)
  (cond
   ((integerp constant)
    (depict-integer markup-stream constant))
   ((floatp constant)
    (depict markup-stream
            (if (zerop constant)
              (if (minusp (float64-sign constant)) "-0.0" "+0.0")
              (progn
                (when (minusp constant)
                  (depict markup-stream :minus)
                  (setq constant (- constant)))
                (format nil (if (= constant (floor constant 1)) "~,1F" "~F") constant)))))
   ((characterp constant)
    (depict markup-stream :left-single-quote)
    (depict-char-style (markup-stream :character-literal)
      (depict-character markup-stream constant nil))
    (depict markup-stream :right-single-quote))
   ((stringp constant)
    (depict-string markup-stream constant))
   (t (error "Bad constant ~S" constant))))


; Emit markup for the primitive when it is not called in a function call.
(defun depict-primitive (markup-stream primitive)
  (unless (eq (primitive-appearance primitive) :global)
    (error "Can't depict primitive ~S outside a call" primitive))
  (let ((markup (primitive-markup1 primitive))
        (external-name (primitive-markup2 primitive)))
    (if external-name
      (depict-link (markup-stream :external "V-" external-name nil)
        (depict-item-or-group-list markup-stream markup))
      (depict-item-or-group-list markup-stream markup))))


; Emit markup for the parameters to a function call.
(defun depict-call-parameters (markup-stream world annotated-parameters)
  (depict-list markup-stream
               #'(lambda (markup-stream parameter)
                   (depict-expression markup-stream world parameter %expr%))
               annotated-parameters
               :indent 4
               :prefix "("
               :prefix-break 0
               :suffix ")"
               :separator ","
               :break 1
               :empty nil))


; Emit markup for the function or primitive call.  level indicates the binding level imposed
; by the enclosing expression.
(defun depict-call (markup-stream world level annotated-function-expr &rest annotated-arg-exprs)
  (if (eq (first annotated-function-expr) 'expr-annotation:primitive)
    (let ((primitive (symbol-primitive (second annotated-function-expr))))
      (depict-expr-parentheses (markup-stream level (primitive-level primitive))
        (ecase (primitive-appearance primitive)
          (:global
           (depict-primitive markup-stream primitive)
           (depict-call-parameters markup-stream world annotated-arg-exprs))
          (:infix
           (assert-true (= (length annotated-arg-exprs) 2))
           (depict-logical-block (markup-stream 0)
             (depict-expression markup-stream world (first annotated-arg-exprs) (primitive-level1 primitive))
             (let ((spaces (primitive-markup2 primitive)))
               (when spaces
                 (depict-space markup-stream))
               (depict-item-or-group-list markup-stream (primitive-markup1 primitive))
               (depict-break markup-stream (if spaces 1 0)))
             (depict-expression markup-stream world (second annotated-arg-exprs) (primitive-level2 primitive))))
          (:unary
           (assert-true (= (length annotated-arg-exprs) 1))
           (depict-item-or-group-list markup-stream (primitive-markup1 primitive))
           (depict-expression markup-stream world (first annotated-arg-exprs) (primitive-level1 primitive))
           (depict-item-or-group-list markup-stream (primitive-markup2 primitive)))
          (:phantom
           (assert-true (= (length annotated-arg-exprs) 1))
           (depict-expression markup-stream world (first annotated-arg-exprs) level)))))
    (depict-expr-parentheses (markup-stream level %suffix%)
      (depict-expression markup-stream world annotated-function-expr %suffix%)
      (depict-call-parameters markup-stream world annotated-arg-exprs))))


; Emit markup for the reference to the action on the given general grammar symbol.
(defun depict-action-reference (markup-stream action-name general-grammar-symbol &optional index)
  (let ((action-default (default-action? action-name)))
    (unless action-default
      (depict-action-name markup-stream action-name)
      (depict markup-stream :action-begin))
    (depict-general-grammar-symbol markup-stream general-grammar-symbol :reference index)
    (unless action-default
      (depict markup-stream :action-end))))


; Emit markup for the given annotated value expression. level indicates the binding level imposed
; by the enclosing expression.
(defun depict-expression (markup-stream world annotated-expr level)
  (let ((annotation (first annotated-expr))
        (args (rest annotated-expr)))
    (ecase annotation
      (expr-annotation:constant (depict-constant markup-stream (first args)))
      (expr-annotation:primitive (depict-primitive markup-stream (symbol-primitive (first args))))
      (expr-annotation:tag (depict-tag-name markup-stream (first args) :reference))
      (expr-annotation:local (depict-local-variable markup-stream (first args)))
      (expr-annotation:global (depict-global-variable markup-stream (first args) :reference))
      (expr-annotation:call (apply #'depict-call markup-stream world level args))
      (expr-annotation:action (apply #'depict-action-reference markup-stream args))
      (expr-annotation:special-form
       (apply (get (first args) :depict-special-form) markup-stream world level (rest args))))))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING SPECIAL FORMS


; (bottom)
(defun depict-bottom (markup-stream world level)
  (declare (ignore world level))
  (depict markup-stream :bottom-10))


; (todo)
(defun depict-todo (markup-stream world level)
  (declare (ignore world level))
  (depict markup-stream "????"))


; (hex <integer> [<length>])
(defun depict-hex (markup-stream world level n length)
  (if (minusp n)
    (progn
      (depict markup-stream "-")
      (depict-hex markup-stream world level (- n) length))
    (depict markup-stream (format nil "0x~V,'0X" length n))))


; (expt <base> <exponent>)
(defun depict-expt (markup-stream world level base-annotated-expr exponent-annotated-expr)
  (depict-expr-parentheses (markup-stream level %prefix%)
    (depict-expression markup-stream world base-annotated-expr %primary%)
    (depict-char-style (markup-stream :superscript)
      (depict-expression markup-stream world exponent-annotated-expr %term%))))


; (= <expr1> <expr2> [<type>])
; (/= <expr1> <expr2> [<type>])
; (< <expr1> <expr2> [<type>])
; (> <expr1> <expr2> [<type>])
; (<= <expr1> <expr2> [<type>])
; (>= <expr1> <expr2> [<type>])
(defun depict-comparison (markup-stream world level order annotated-expr1 annotated-expr2)
  (depict-expr-parentheses (markup-stream level %relational%)
    (depict-logical-block (markup-stream 0)
      (depict-expression markup-stream world annotated-expr1 %term%)
      (depict-space markup-stream)
      (depict markup-stream order)
      (depict-break markup-stream 1)
      (depict-expression markup-stream world annotated-expr2 %term%))))


; (cascade <type> <expr1> <order1> <expr2> <order2> ... <ordern-1> <exprn>)
(defun depict-cascade (markup-stream world level annotated-expr1 &rest orders-and-exprs)
  (depict-expr-parentheses (markup-stream level %relational%)
    (depict-logical-block (markup-stream 0)
      (depict-expression markup-stream world annotated-expr1 %term%)
      (do ()
          ((endp orders-and-exprs))
        (depict-space markup-stream)
        (depict markup-stream (pop orders-and-exprs))
        (depict-break markup-stream 1)
        (depict-expression markup-stream world (pop orders-and-exprs) %term%)))))


; (and <expr> ... <expr>)
; (or <expr> ... <expr>)
; (xor <expr> ... <expr>)
(defun depict-and-or-xor (markup-stream world level op annotated-expr &rest annotated-exprs)
  (if annotated-exprs
    (depict-expr-parentheses (markup-stream level %logical%)
      (depict-logical-block (markup-stream 0)
        (depict-expression markup-stream world annotated-expr %not%)
        (dolist (annotated-expr annotated-exprs)
          (depict-semantic-keyword markup-stream op :before)
          (depict-break markup-stream 1)
          (depict-expression markup-stream world annotated-expr %not%))))
    (depict-expression markup-stream world annotated-expr level)))


(defun depict-function-signature (markup-stream world arg-binding-exprs result-type-expr show-type)
  (depict-logical-block (markup-stream 12)
    (depict-break markup-stream 0)
    (depict-list markup-stream
                 #'(lambda (markup-stream arg-binding)
                     (depict-local-variable markup-stream (first arg-binding))
                     (depict markup-stream ": ")
                     (depict-type-expr markup-stream world (second arg-binding)))
                 arg-binding-exprs
                 :indent 2
                 :prefix "("
                 :suffix ")"
                 :separator ","
                 :break 1
                 :empty nil)
    (unless (or (eq result-type-expr 'void) (not show-type))
      (depict markup-stream ":")
      (depict-break markup-stream 1)
      (depict-type-expr markup-stream world result-type-expr))))


(defun depict-function-body (markup-stream world body-annotated-stmts)
  (depict-logical-block (markup-stream 4)
    (let ((first-annotated-stmt (first body-annotated-stmts)))
      (if (and body-annotated-stmts
               (endp (cdr body-annotated-stmts))
               (special-form-annotated-stmt? world 'return first-annotated-stmt)
               (cdr first-annotated-stmt))
        (progn
          (depict-break markup-stream 1)
          (depict markup-stream :identical-10 " ")
          (depict-expression markup-stream world (second first-annotated-stmt) %expr%))
        (progn
          (depict-break markup-stream t)
          (depict-semantic-keyword markup-stream 'begin nil)
          (depict-logical-block (markup-stream 4)
            (depict-statements markup-stream world t body-annotated-stmts))
          (depict-break markup-stream t)
          (depict-semantic-keyword markup-stream 'end nil))))))


; (lambda ((<var1> <type1> [:unused]) ... (<varn> <typen> [:unused])) <result-type> . <statements>)
(defun depict-lambda (markup-stream world level arg-binding-exprs result-type-expr &rest body-annotated-stmts)
  (depict-expr-parentheses (markup-stream level %expr%)
    (depict-logical-block (markup-stream 0)
      (depict-semantic-keyword markup-stream 'function nil)
      (depict-function-signature markup-stream world arg-binding-exprs result-type-expr t)
      (depict-function-body markup-stream world body-annotated-stmts))))


; (if <condition-expr> <true-expr> <false-expr>)
(defun depict-if-expr (markup-stream world level condition-annotated-expr true-annotated-expr false-annotated-expr)
  (depict-expr-parentheses (markup-stream level %expr%)
    (depict-logical-block (markup-stream 0)
      (depict-semantic-keyword markup-stream 'if :after)
      (depict-logical-block (markup-stream 4)
        (depict-expression markup-stream world condition-annotated-expr %logical%))
      (depict-break markup-stream 1)
      (depict-semantic-keyword markup-stream 'then :after)
      (depict-logical-block (markup-stream 7)
        (depict-expression markup-stream world true-annotated-expr %expr%))
      (depict-break markup-stream 1)
      (depict-semantic-keyword markup-stream 'else :after)
      (depict-logical-block (markup-stream (if (special-form-annotated-expr? world 'if false-annotated-expr) nil 6))
        (depict-expression markup-stream world false-annotated-expr %expr%)))))


;;; Vectors

; (vector <element-expr> <element-expr> ... <element-expr>)
(defun depict-vector-expr (markup-stream world level &rest element-annotated-exprs)
  (declare (ignore level))
  (if element-annotated-exprs
    (depict-list markup-stream
                 #'(lambda (markup-stream element-annotated-expr)
                     (depict-expression markup-stream world element-annotated-expr %expr%))
                 element-annotated-exprs
                 :indent 1
                 :prefix :vector-begin
                 :suffix :vector-end
                 :separator ","
                 :break 1)
    (depict markup-stream :empty-vector)))


#|
(defun depict-subscript-type-expr (markup-stream world type-expr)
  (depict-char-style (markup-stream 'sub)
    (depict-type-expr markup-stream world type-expr)))
|#


#|
(defun depict-special-function (markup-stream world name-str &rest arg-annotated-exprs)
  (depict-link (markup-stream :external "V-" name-str nil)
    (depict-char-style (markup-stream :global-variable)
      (depict markup-stream name-str)))
  (depict-call-parameters markup-stream world arg-annotated-exprs))
|#


; (empty <vector-expr>)
(defun depict-empty (markup-stream world level vector-annotated-expr)
  (depict-expr-parentheses (markup-stream level %relational%)
    (depict-logical-block (markup-stream 0)
      (depict-length markup-stream world %term% vector-annotated-expr)
      (depict markup-stream " = ")
      (depict-constant markup-stream 0))))


; (length <vector-expr>)
(defun depict-length (markup-stream world level vector-annotated-expr)
  (declare (ignore level))
  (depict markup-stream "|")
  (depict-expression markup-stream world vector-annotated-expr %expr%)
  (depict markup-stream "|"))


; (nth <vector-expr> <n-expr>)
(defun depict-nth (markup-stream world level vector-annotated-expr n-annotated-expr)
  (depict-expr-parentheses (markup-stream level %suffix%)
    (depict-expression markup-stream world vector-annotated-expr %suffix%)
    (depict markup-stream "[")
    (depict-expression markup-stream world n-annotated-expr %expr%)
    (depict markup-stream "]")))


; (subseq <vector-expr> <low-expr> [<high-expr>])
(defun depict-subseq (markup-stream world level vector-annotated-expr low-annotated-expr high-annotated-expr)
  (depict-expr-parentheses (markup-stream level %suffix%)
    (depict-expression markup-stream world vector-annotated-expr %suffix%)
    (depict-logical-block (markup-stream 4)
      (depict markup-stream "[")
      (depict-expression markup-stream world low-annotated-expr %term%)
      (depict markup-stream " ...")
      (when high-annotated-expr
        (depict-break markup-stream 1)
        (depict-expression markup-stream world high-annotated-expr %term%))
      (depict markup-stream "]"))))


; (append <vector-expr> <vector-expr>)
(defun depict-append (markup-stream world level vector1-annotated-expr vector2-annotated-expr)
  (depict-expr-parentheses (markup-stream level %term%)
    (depict-logical-block (markup-stream 0)
      (depict-expression markup-stream world vector1-annotated-expr %term%)
      (depict markup-stream " " :vector-append)
      (depict-break markup-stream 1)
      (depict-expression markup-stream world vector2-annotated-expr %term%))))


; (set-nth <vector-expr> <n-expr> <value-expr>)
(defun depict-set-nth (markup-stream world level vector-annotated-expr n-annotated-expr value-annotated-expr)
  (depict-expr-parentheses (markup-stream level %suffix%)
    (depict-expression markup-stream world vector-annotated-expr %suffix%)
    (depict-logical-block (markup-stream 4)
      (depict markup-stream "[")
      (depict-expression markup-stream world n-annotated-expr %term%)
      (depict markup-stream " \\")
      (depict-break markup-stream 1)
      (depict-expression markup-stream world value-annotated-expr %term%)
      (depict markup-stream "]"))))


; (map <vector-expr> <var> <value-expr> [<condition-expr>])
(defun depict-map (markup-stream world level vector-annotated-expr var value-annotated-expr &optional condition-annotated-expr)
  (declare (ignore level))
  (depict-logical-block (markup-stream 2)
    (depict markup-stream :vector-begin)
    (depict-expression markup-stream world value-annotated-expr %expr%)
    (depict markup-stream " " :vector-construct)
    (depict-break markup-stream 1)
    (depict markup-stream :for-all-10)
    (depict-local-variable markup-stream var)
    (depict markup-stream " " :member-10 " ")
    (depict-expression markup-stream world vector-annotated-expr %term%)
    (when condition-annotated-expr
      (depict-semantic-keyword markup-stream 'and :before)
      (depict-break markup-stream 1)
      (depict-expression markup-stream world condition-annotated-expr %not%))
    (depict markup-stream :vector-end)))


;;; Sets

; (set-of-ranges <element-type> <low-expr> <high-expr> ... <low-expr> <high-expr>)
(defun depict-set-of-ranges (markup-stream world level element-type-expr &rest element-annotated-exprs)
  (declare (ignore level element-type-expr))
  (labels
    ((combine-exprs (element-annotated-exprs)
       (if (endp element-annotated-exprs)
         nil
         (acons (first element-annotated-exprs) (second element-annotated-exprs)
                (combine-exprs (cddr element-annotated-exprs))))))
    (depict-list markup-stream
                 #'(lambda (markup-stream element-annotated-expr-range)
                     (let ((element-annotated-expr1 (car element-annotated-expr-range))
                           (element-annotated-expr2 (cdr element-annotated-expr-range)))
                       (depict-expression markup-stream world element-annotated-expr1 %term%)
                       (when element-annotated-expr2
                         (depict markup-stream " ...")
                         (depict-break markup-stream 1)
                         (depict-expression markup-stream world element-annotated-expr2 %term%))))
                 (combine-exprs element-annotated-exprs)
                 :indent 1
                 :prefix "{"
                 :suffix "}"
                 :separator ","
                 :break 1
                 :empty nil)))


;;; Tags

(defparameter *depict-tag-labels* nil)

; (tag <tag> <field-expr1> ... <field-exprn>)
(defun depict-tag-expr (markup-stream world level tag &rest annotated-exprs)
  (let ((mutable (tag-mutable tag)))
    (flet
      ((depict-tag-and-args (markup-stream)
         (let ((fields (tag-fields tag)))
           (assert-true (= (length fields) (length annotated-exprs)))
           (depict-tag-name markup-stream tag :reference)
           (if (tag-keyword tag)
             (assert-true (null annotated-exprs))
             (depict-list markup-stream
                          #'(lambda (markup-stream parameter)
                              (let ((field (pop fields)))
                                (if (and mutable *depict-tag-labels*)
                                  (depict-logical-block (markup-stream 4)
                                    (depict-label-name markup-stream tag (field-label field) :reference)
                                    (depict markup-stream " " :label-assign-10)
                                    (depict-break markup-stream 1)
                                    (depict-expression markup-stream world parameter %expr%))
                                  (depict-expression markup-stream world parameter %expr%))))
                          annotated-exprs
                          :indent 4
                          :prefix (if mutable :record-begin :tuple-begin)
                          :prefix-break 0
                          :suffix (if mutable :record-end :tuple-end)
                          :separator ","
                          :break 1
                          :empty nil)))))
      
      (if mutable
        (depict-expr-parentheses (markup-stream level %prefix%)
          (depict-logical-block (markup-stream 4)
            (depict-semantic-keyword markup-stream 'new :after)
            (depict-tag-and-args markup-stream)))
        (depict-tag-and-args markup-stream)))))


; (& <label> <record-expr>)
(defun depict-& (markup-stream world level tag label annotated-expr)
  (depict-expr-parentheses (markup-stream level %suffix%)
    (depict-expression markup-stream world annotated-expr %suffix%)
    (depict markup-stream ".")
    (depict-label-name markup-stream tag label :reference)))


;;; Unions

(defun depict-in-or-not-in (markup-stream world level type type-expr value-annotated-expr op single-op)
  (depict-expr-parentheses (markup-stream level %relational%)
    (depict-expression markup-stream world value-annotated-expr %suffix%)
    (depict-space markup-stream)
    (if (and (eq (type-kind type) :tag) (tag-keyword (type-tag type)))
      (progn
        (depict markup-stream single-op)
        (depict-space markup-stream)
        (depict-tag-name markup-stream (type-tag type) :reference))
      (progn
        (depict markup-stream op)
        (depict-space markup-stream)
        (depict-type-expr markup-stream world type-expr)))))

; (in <type> <expr>)
(defun depict-in (markup-stream world level type type-expr value-annotated-expr)
  (depict-in-or-not-in markup-stream world level type type-expr value-annotated-expr :member-10 "="))

; (not-in <type> <expr>)
(defun depict-not-in (markup-stream world level type type-expr value-annotated-expr)
  (depict-in-or-not-in markup-stream world level type type-expr value-annotated-expr :not-member-10 :not-equal))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING STATEMENTS


; Emit markup for the annotated statement.
(defun depict-statement (markup-stream world annotated-stmt)
  (apply (get (first annotated-stmt) :depict-statement) markup-stream world (rest annotated-stmt)))


; Emit markup for the block of annotated statements, including the preceding line breaks using
; the given prefix.
(defun depict-statements (markup-stream world prefix-break annotated-stmts)
  (if annotated-stmts
    (depict-list markup-stream
                 #'(lambda (markup-stream annotated-stmt)
                     (depict-statement markup-stream world annotated-stmt))
                 annotated-stmts
                 :indent 0
                 :prefix-break prefix-break
                 :separator ";"
                 :break t)
    (progn
      (depict-break markup-stream prefix-break)
      (depict-semantic-keyword markup-stream 'nothing nil))))


; (exec <expr>)
(defun depict-exec (markup-stream world annotated-expr)
  (depict-expression markup-stream world annotated-expr %logical%))


; (const <name> <type> <value>)
; (var <name> <type> <value>)
(defun depict-var (markup-stream world name type-expr value-annotated-expr)
  (depict-local-variable markup-stream name)
  (depict markup-stream ": ")
  (depict-type-expr markup-stream world type-expr)
  (depict markup-stream " " :assign-10)
  (depict-logical-block (markup-stream 6)
    (depict-break markup-stream 1)
    (depict-expression markup-stream world value-annotated-expr %expr%)))


; (function (<name> (<var1> <type1> [:unused]) ... (<varn> <typen> [:unused])) <result-type> . <statements>)
(defun depict-function (markup-stream world name-and-arg-binding-exprs result-type-expr &rest body-annotated-stmts)
  (depict-logical-block (markup-stream 0)
    (depict-semantic-keyword markup-stream 'function :after)
    (depict-local-variable markup-stream (first name-and-arg-binding-exprs))
    (depict-function-signature markup-stream world (rest name-and-arg-binding-exprs) result-type-expr t)
    (depict-function-body markup-stream world body-annotated-stmts)))


; (<- <name> <value>)
(defun depict-<- (markup-stream world name value-annotated-expr)
  (depict-local-variable markup-stream name)
  (depict markup-stream " " :assign-10)
  (depict-logical-block (markup-stream 6)
    (depict-break markup-stream 1)
    (depict-expression markup-stream world value-annotated-expr %expr%)))


; (&= <record-expr> <value-expr>)
(defun depict-&= (markup-stream world tag label record-annotated-expr value-annotated-expr)
  (depict-& markup-stream world %unary% tag label record-annotated-expr)
  (depict markup-stream " " :assign-10)
  (depict-logical-block (markup-stream 6)
    (depict-break markup-stream 1)
    (depict-expression markup-stream world value-annotated-expr %expr%)))


; (return [<value-expr>])
(defun depict-return (markup-stream world value-annotated-expr)
  (depict-logical-block (markup-stream 4)
    (depict-semantic-keyword markup-stream 'return nil)
    (when value-annotated-expr
      (depict-space markup-stream)
      (depict-expression markup-stream world value-annotated-expr %logical%))))


; (cond (<condition-expr> . <statements>) ... (<condition-expr> . <statements>) [(nil . <statements>)])
(defun depict-cond (markup-stream world &rest annotated-cases)
  (assert-true (and annotated-cases (caar annotated-cases)))
  (depict-logical-block (markup-stream 0)
    (do ((annotated-cases annotated-cases (rest annotated-cases))
         (else nil t))
        ((endp annotated-cases))
      (let ((annotated-case (first annotated-cases)))
        (depict-logical-block (markup-stream 4)
          (let ((condition-annotated-expr (first annotated-case)))
            (if condition-annotated-expr
              (progn
                (depict-semantic-keyword markup-stream (if else 'elsif 'if) :after)
                (depict-logical-block (markup-stream 4)
                  (depict-expression markup-stream world condition-annotated-expr %logical%))
                (depict-semantic-keyword markup-stream 'then :before))
              (depict-semantic-keyword markup-stream 'else nil)))
          (depict-statements markup-stream world 1 (rest annotated-case)))
        (depict-break markup-stream 1)))
    (depict-semantic-keyword markup-stream 'end :after)
    (depict-semantic-keyword markup-stream 'if nil)))


; (while <condition-expr> . <statements>)
(defun depict-while (markup-stream world condition-annotated-expr &rest loop-annotated-stmts)
  (depict-logical-block (markup-stream 0)
    (depict-logical-block (markup-stream 4)
      (depict-semantic-keyword markup-stream 'while :after)
      (depict-logical-block (markup-stream 4)
        (depict-expression markup-stream world condition-annotated-expr %logical%))
      (depict-semantic-keyword markup-stream 'do :before)
      (depict-statements markup-stream world 1 loop-annotated-stmts))
    (depict-break markup-stream 1)
    (depict-semantic-keyword markup-stream 'end :after)
    (depict-semantic-keyword markup-stream 'while nil)))


; (assert <condition-expr>)
(defun depict-assert (markup-stream world condition-annotated-expr)
  (depict-logical-block (markup-stream 4)
    (depict-semantic-keyword markup-stream 'invariant :after)
    (depict-expression markup-stream world condition-annotated-expr %logical%)))


; (throw <value-expr>)
(defun depict-throw (markup-stream world value-annotated-expr)
  (depict-logical-block (markup-stream 4)
    (depict-semantic-keyword markup-stream 'throw :after)
    (depict-expression markup-stream world value-annotated-expr %logical%)))


; (catch <body-statements> (<var> [:unused]) . <handler-statements>)
(defun depict-catch (markup-stream world body-annotated-stmts arg-binding-expr &rest handler-annotated-stmts)
  (depict-logical-block (markup-stream 0)
    (depict-logical-block (markup-stream 4)
      (depict-semantic-keyword markup-stream 'try nil)
      (depict-statements markup-stream world 1 body-annotated-stmts))
    (depict-break markup-stream)
    (depict-logical-block (markup-stream 4)
      (depict-semantic-keyword markup-stream 'catch :after)
      (depict-logical-block (markup-stream 4)
        (depict-local-variable markup-stream (first arg-binding-expr))
        (depict markup-stream ": ")
        (depict-type-expr markup-stream world *semantic-exception-type-name*))
      (depict-semantic-keyword markup-stream 'do :before)
      (depict-statements markup-stream world 1 handler-annotated-stmts))
    (depict-break markup-stream)
    (depict-semantic-keyword markup-stream 'end :after)
    (depict-semantic-keyword markup-stream 'try nil)))


; (case <value-expr> (key <type> . <statements>) ... (keyword <type> . <statements>))
; where each key is one of:
;    :select    No special action
;    :narrow    Narrow the type of <value-expr>, which must be a variable, to this case's <type>
;    :otherwise Catch-all else case; <type> should be either nil or the remaining catch-all type
(defun depict-case (markup-stream world value-annotated-expr &rest annotated-cases)
  (depict-logical-block (markup-stream 0)
    (depict-semantic-keyword markup-stream 'case :after)
    (depict-logical-block (markup-stream 8)
      (depict-expression markup-stream world value-annotated-expr %logical%))
    (depict-semantic-keyword markup-stream 'of :before)
    (depict-list
     markup-stream
     #'(lambda (markup-stream annotated-case)
         (depict-logical-block (markup-stream 4)
           (ecase (first annotated-case)
             ((:select :narrow) (depict-type-expr markup-stream world (second annotated-case)))
             ((:otherwise) (depict-semantic-keyword markup-stream 'else nil)))
           (depict-semantic-keyword markup-stream 'do :before)
           (depict-statements markup-stream world 1 (cddr annotated-case))))
     annotated-cases
     :indent 4
     :prefix-break t
     :separator ";"
     :break t
     :empty nil)
    (depict-break markup-stream)
    (depict-semantic-keyword markup-stream 'end :after)
    (depict-semantic-keyword markup-stream 'case nil)))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING COMMANDS


(defmacro depict-semantics ((markup-stream depict-env &optional (paragraph-style :semantics)) &body body)
  `(when (depict-mode ,markup-stream ,depict-env :semantics)
     (depict-paragraph (,markup-stream ,paragraph-style)
       ,@body)))


; (%highlight <highlight> <command> ... <command>)
; Depict the commands highlighted with the <highlight> block style.
(defun depict-%highlight (markup-stream world depict-env highlight &rest commands)
  (when commands
    (depict-block-style (markup-stream highlight t)
      (depict-commands markup-stream world depict-env commands))))


; (%section "section-name")
; (%section <mode> "section-name")
; <mode> is one of:
;   :syntax     This is a comment about the syntax
;   :semantics  This is a comment about the semantics (not displayed when semantics are not displayed)
;   nil         This is a general comment
(defun depict-%section (markup-stream world depict-env mode &optional section-name)
  (declare (ignore world))
  (depict-section-or-subsection markup-stream depict-env mode section-name :section-heading))


; (%subsection "subsection-name")
; (%subsection <mode> "subsection-name")
; <mode> is one of:
;   :syntax     This is a comment about the syntax
;   :semantics  This is a comment about the semantics (not displayed when semantics are not displayed)
;   nil         This is a general comment
(defun depict-%subsection (markup-stream world depict-env mode &optional section-name)
  (declare (ignore world))
  (depict-section-or-subsection markup-stream depict-env mode section-name :subsection-heading))


; Common routine for depict-%section and depict-%subsection.
(defun depict-section-or-subsection (markup-stream depict-env mode section-name paragraph-style)
  (when (stringp mode)
    (when section-name
      (error "Bad %section or %subsection"))
    (setq section-name mode)
    (setq mode nil))
  (assert-type section-name string)
  (when (quiet-depict-mode depict-env mode)
    (depict-paragraph (markup-stream paragraph-style)
      (depict markup-stream section-name))))


; (%text <mode> . <styled-text>)
; <mode> is one of:
;   :syntax     This is a comment about the syntax
;   :semantics  This is a comment about the semantics (not displayed when semantics are not displayed)
;   :comment    This is a comment about the following piece of semantics (not displayed when semantics are not displayed)
;   nil         This is a general comment
(defun depict-%text (markup-stream world depict-env mode &rest text)
  (when (depict-mode markup-stream depict-env (if (eq mode :comment) :semantics mode))
    (depict-paragraph (markup-stream (if (eq mode :comment) :semantic-comment :body-text))
      (let ((grammar-info (depict-env-grammar-info depict-env))
            (*styled-text-world* world))
        (if grammar-info
          (let ((*styled-text-grammar-parametrization* (grammar-info-grammar grammar-info)))
            (depict-styled-text markup-stream text))
          (depict-styled-text markup-stream text))))))


; (grammar-argument <argument> <attribute> <attribute> ... <attribute>)
(defun depict-grammar-argument (markup-stream world depict-env argument &rest attributes)
  (declare (ignore world))
  (let ((seen-grammar-arguments (depict-env-seen-grammar-arguments depict-env))
        (abbreviated-argument (symbol-abbreviation argument)))
    (unless (gethash abbreviated-argument seen-grammar-arguments)
      (when (depict-mode markup-stream depict-env :syntax)
        (depict-paragraph (markup-stream :grammar-argument)
          (depict-nonterminal-argument markup-stream argument)
          (depict markup-stream " " :member-10 " ")
          (depict-list markup-stream
                       #'(lambda (markup-stream attribute)
                           (depict-nonterminal-attribute markup-stream attribute))
                       attributes
                       :prefix "{"
                       :suffix "}"
                       :separator ", ")))
      (setf (gethash abbreviated-argument seen-grammar-arguments) t))))


; (%rule <general-nonterminal-source>)
(defun depict-%rule (markup-stream world depict-env general-nonterminal-source)
  (declare (ignore world))
  (let* ((grammar-info (checked-depict-env-grammar-info depict-env))
         (grammar (grammar-info-grammar grammar-info))
         (general-nonterminal (grammar-parametrization-intern grammar general-nonterminal-source))
         (seen-nonterminals (depict-env-seen-nonterminals depict-env)))
    (when (grammar-info-charclass grammar-info general-nonterminal)
      (error "Shouldn't use %rule on a lexer charclass nonterminal ~S" general-nonterminal))
    (labels
      ((seen-nonterminal? (nonterminal)
         (gethash nonterminal seen-nonterminals)))
      (unless (or (hidden-nonterminal? general-nonterminal)
                  (every #'seen-nonterminal? (general-grammar-symbol-instances grammar general-nonterminal)))
        (let ((visible (depict-mode markup-stream depict-env :syntax)))
          (dolist (general-rule (grammar-general-rules grammar general-nonterminal))
            (let ((rule-lhs-nonterminals (general-grammar-symbol-instances grammar (general-rule-lhs general-rule))))
              (unless (every #'seen-nonterminal? rule-lhs-nonterminals)
                (when (some #'seen-nonterminal? rule-lhs-nonterminals)
                  (warn "General rule for ~S listed before specific ones; use %rule to disambiguate" general-nonterminal))
                (when visible
                  (depict-general-rule markup-stream general-rule (grammar-highlights grammar)))
                (dolist (nonterminal rule-lhs-nonterminals)
                  (setf (gethash nonterminal seen-nonterminals) t))))))))))
;******** May still have a problem when a specific rule precedes a general one.


; (%charclass <nonterminal>)
(defun depict-%charclass (markup-stream world depict-env nonterminal-source)
  (let* ((grammar-info (checked-depict-env-grammar-info depict-env))
         (grammar (grammar-info-grammar grammar-info))
         (nonterminal (grammar-parametrization-intern grammar nonterminal-source))
         (charclass (grammar-info-charclass grammar-info nonterminal)))
    (unless charclass
      (error "%charclass with a non-charclass ~S" nonterminal))
    (if (gethash nonterminal (depict-env-seen-nonterminals depict-env))
      (warn "Duplicate charclass ~S" nonterminal)
      (progn
        (when (depict-mode markup-stream depict-env :syntax)
          (depict-charclass markup-stream charclass)
          (dolist (action-cons (charclass-actions charclass))
            (depict-charclass-action markup-stream world depict-env (car action-cons) (cdr action-cons) nonterminal)))
        (setf (gethash nonterminal (depict-env-seen-nonterminals depict-env)) t)))))


; (%print-actions)
(defun depict-%print-actions (markup-stream world depict-env)
  (declare (ignore world))
  (dolist (pending-action (nreverse (depict-env-pending-actions-reverse depict-env)))
    (funcall pending-action markup-stream depict-env))
  (setf (depict-env-pending-actions-reverse depict-env) nil))


; (deftag <name> (<name1> <type1>) ... (<namen> <typen>))
; (defrecord <name> (<name1> <type1>) ... (<namen> <typen>))
(defun depict-deftag (markup-stream world depict-env name &rest fields)
  (depict-semantics (markup-stream depict-env)
    (depict-logical-block (markup-stream 2)
      (let* ((tag (scan-tag world name))
             (mutable (tag-mutable tag)))
        (depict-semantic-keyword markup-stream (if mutable 'record 'tag) :after)
        (depict-tag-name markup-stream tag :definition)
        (when (or mutable fields)
          (depict-list
           markup-stream
           #'(lambda (markup-stream field)
               (depict-label-name markup-stream tag (first field) nil)
               (depict markup-stream ": ")
               (depict-type-expr markup-stream world (second field) %%type%%))
           fields
           :indent 6
           :prefix (if mutable :record-begin :tuple-begin)
           :prefix-break 0
           :suffix (if mutable :record-end :tuple-end)
           :separator ","
           :break 1
           :empty nil)))
      (depict markup-stream ";"))))


; (deftype <name> <type>)
(defun depict-deftype (markup-stream world depict-env name type-expr)
  (depict-semantics (markup-stream depict-env)
    (depict-logical-block (markup-stream 2)
      (depict-semantic-keyword markup-stream 'type :after)
      (depict-type-name markup-stream name :definition)
      (depict-break markup-stream 0)
      (depict-logical-block (markup-stream 4)
        (depict markup-stream " = ")
        (depict-type-expr markup-stream world type-expr))
      (depict markup-stream ";"))))


(defun depict-type-and-value (markup-stream world type-expr value-annotated-expr show-type)
  (when show-type
    (depict-logical-block (markup-stream 4)
      (depict markup-stream ":")
      (depict-break markup-stream 1)
      (depict-type-expr markup-stream world type-expr)))
  (if (eq (car value-annotated-expr) 'expr-annotation:begin)
    (depict-function-body markup-stream world (cdr value-annotated-expr))
    (progn
      (depict-break markup-stream 0)
      (depict-logical-block (markup-stream 4)
        (depict markup-stream " = ")
        (depict-expression markup-stream world value-annotated-expr %expr%))))
  (depict markup-stream ";"))

; (define <name> <type> <value>)
(defun depict-define (markup-stream world depict-env name type-expr value-expr)
  (depict-semantics (markup-stream depict-env)
    (depict-logical-block (markup-stream 0)
      (depict-global-variable markup-stream name :definition)
      (let ((value-annotated-expr (nth-value 2 (scan-value world *null-type-env* value-expr))))
        (depict-type-and-value markup-stream world type-expr value-annotated-expr t)))))


(defun depict-signature-and-body (markup-stream world type-expr value-annotated-expr show-type)
  (declare (ignore type-expr))
  (assert-true (special-form-annotated-expr? world 'lambda value-annotated-expr))
  (depict-function-signature markup-stream world (third value-annotated-expr) (fourth value-annotated-expr) show-type)
  (depict-function-body markup-stream world (cddddr value-annotated-expr))
  (depict markup-stream ";"))

; (defun <name> (-> (<type1> ... <typen>) <result-type>) (lambda ((<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>))
(defun depict-defun (markup-stream world depict-env name type-expr value-expr)
  (assert-true (eq (first type-expr) '->))
  (let ((value-annotated-expr (nth-value 2 (scan-value world *null-type-env* value-expr))))
    (depict-semantics (markup-stream depict-env)
      (depict-logical-block (markup-stream 0)
        (depict-semantic-keyword markup-stream 'function :after)
        (depict-global-variable markup-stream name :definition)
        (depict-signature-and-body markup-stream world type-expr value-annotated-expr t)))))


; (set-grammar <name>)
(defun depict-set-grammar (markup-stream world depict-env name)
  (depict-clear-grammar markup-stream world depict-env)
  (let ((grammar-info (world-grammar-info world name)))
    (unless grammar-info
      (error "Unknown grammar ~A" name))
    (setf (depict-env-grammar-info depict-env) grammar-info)
    (setf (depict-env-seen-nonterminals depict-env) (make-hash-table :test #'eq))
    (setf (depict-env-seen-grammar-arguments depict-env) (make-hash-table :test #'eq))))


; (clear-grammar)
(defun depict-clear-grammar (markup-stream world depict-env)
  (depict-%print-actions markup-stream world depict-env)
  (depict-mode markup-stream depict-env nil)
  (let ((grammar-info (depict-env-grammar-info depict-env)))
    (when grammar-info
      (let ((seen-nonterminals (depict-env-seen-nonterminals depict-env))
            (missed-nonterminals nil))
        (dolist (nonterminal (grammar-nonterminals-list (grammar-info-grammar grammar-info)))
          (unless (or (gethash nonterminal seen-nonterminals)
                      (eq nonterminal *start-nonterminal*)
                      (hidden-nonterminal? nonterminal))
            (push nonterminal missed-nonterminals)))
        (when missed-nonterminals
          (warn "Nonterminals not printed: ~S" missed-nonterminals)))
      (setf (depict-env-grammar-info depict-env) nil)
      (setf (depict-env-seen-nonterminals depict-env) nil)
      (setf (depict-env-seen-grammar-arguments depict-env) nil))))


(defmacro depict-delayed-action ((markup-stream depict-env) &body depictor)
  (let ((saved-block-style (gensym "SAVED-BLOCK-STYLE")))
    `(let ((,saved-block-style (save-block-style ,markup-stream)))
       (push #'(lambda (,markup-stream ,depict-env)
                 (with-saved-block-style (,markup-stream ,saved-block-style t) ,@depictor))
             (depict-env-pending-actions-reverse ,depict-env)))))


(defun depict-declare-action-contents (markup-stream world action-name general-grammar-symbol type-expr)
  (depict-action-name markup-stream action-name)
  (depict markup-stream :action-begin)
  (depict-general-grammar-symbol markup-stream general-grammar-symbol :reference)
  (depict markup-stream :action-end)
  (depict-break markup-stream 0)
  (depict-logical-block (markup-stream 2)
    (depict markup-stream ": ")
    (depict-type-expr markup-stream world type-expr)))


; (declare-action <action-name> <general-grammar-symbol> <type> <n-productions>)
(defun depict-declare-action (markup-stream world depict-env action-name general-grammar-symbol-source type-expr n-productions)
  (let* ((grammar-info (checked-depict-env-grammar-info depict-env))
         (general-grammar-symbol (grammar-parametrization-intern (grammar-info-grammar grammar-info) general-grammar-symbol-source)))
    (unless (or (and (general-nonterminal? general-grammar-symbol) (hidden-nonterminal? general-grammar-symbol))
                (grammar-info-charclass-or-partition grammar-info general-grammar-symbol)
                (= n-productions 1))
      (depict-delayed-action (markup-stream depict-env)
        (depict-semantics (markup-stream depict-env)
          (depict-logical-block (markup-stream 4)
            (depict-declare-action-contents markup-stream world action-name general-grammar-symbol type-expr)))))))


; Declare and define the lexer-action on the charclass given by nonterminal.
(defun depict-charclass-action (markup-stream world depict-env action-name lexer-action nonterminal)
  (unless (default-action? action-name)
    (depict-delayed-action (markup-stream depict-env)
      (depict-semantics (markup-stream depict-env)
        (depict-logical-block (markup-stream 4)
          (depict-declare-action-contents markup-stream world action-name
                                          nonterminal (lexer-action-type-expr lexer-action))
          (depict-break markup-stream 1)
          (depict-logical-block (markup-stream 3)
            (depict markup-stream "= ")
            (depict-global-variable markup-stream (lexer-action-function-name lexer-action) :external)
            (depict markup-stream "(")
            (depict-general-nonterminal markup-stream nonterminal :reference)
            (depict markup-stream ")"))
          (depict markup-stream ";"))))))


; (action <action-name> <production-name> <type> <n-productions> <value>)
(defun depict-action (markup-stream world depict-env action-name production-name type-expr n-productions value-expr)
  (depict-general-action markup-stream world depict-env action-name production-name type-expr n-productions value-expr nil))

; (actfun <action-name> <production-name> (-> (<type1> ... <typen>) <result-type>) <n-productions>
;    (lambda ((<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>))
(defun depict-actfun (markup-stream world depict-env action-name production-name type-expr n-productions value-expr)
  (depict-general-action markup-stream world depict-env action-name production-name type-expr n-productions value-expr t))

(defun depict-general-action (markup-stream world depict-env action-name production-name type-expr n-productions value-expr destructured)
  (let* ((grammar-info (checked-depict-env-grammar-info depict-env))
         (grammar (grammar-info-grammar grammar-info))
         (general-production (grammar-general-production grammar production-name))
         (lhs (general-production-lhs general-production))
         (show-type (= n-productions 1)))
    (unless (or (grammar-info-charclass grammar-info lhs)
                (hidden-nonterminal? lhs))
      (depict-delayed-action (markup-stream depict-env)
        (depict-semantics (markup-stream depict-env (if show-type :semantics :semantics-next))
          (depict-logical-block (markup-stream 0)
            (let* ((initial-env (general-production-action-env grammar general-production))
                   (type (scan-type world type-expr))
                   (value-annotated-expr (nth-value 1 (scan-typed-value-or-begin world initial-env value-expr type)))
                   (action-grammar-symbols (annotated-expr-grammar-symbols value-annotated-expr)))
              (depict-action-name markup-stream action-name)
              (depict markup-stream :action-begin)
              (depict-general-production markup-stream general-production :reference action-grammar-symbols)
              (depict markup-stream :action-end)
              (if destructured
                (depict-signature-and-body markup-stream world type-expr value-annotated-expr show-type)
                (depict-type-and-value markup-stream world type-expr value-annotated-expr show-type)))))))))


; (terminal-action <action-name> <terminal> <lisp-function>)
(defun depict-terminal-action (markup-stream world depict-env action-name terminal function)
  (declare (ignore markup-stream world depict-env action-name terminal function)))


;;; ------------------------------------------------------------------------------------------------------
;;; DEPICTING STYLED TEXT

; Styled text can include the formats below as long as *styled-text-world* is bound around the call
; to depict-styled-text.

; (:type <type-expression>)
(defun depict-styled-text-type (markup-stream type-expression)
  (depict-type-expr markup-stream *styled-text-world* type-expression))

(setf (styled-text-depictor :type) #'depict-styled-text-type)


; (:tag <name>)
(defun depict-styled-text-tag (markup-stream tag-name)
  (depict-tag-name markup-stream (scan-tag *styled-text-world* tag-name) :reference))

(setf (styled-text-depictor :tag) #'depict-styled-text-tag)


; (:label <tag-name> <label>)
(defun depict-styled-text-label (markup-stream tag-name label)
  (depict-label-name markup-stream (scan-tag *styled-text-world* tag-name) label :reference))

(setf (styled-text-depictor :label) #'depict-styled-text-label)


; (:global <name>)
(defun depict-styled-text-global-variable (markup-stream name)
  (let ((interned-name (world-find-symbol *styled-text-world* name)))
    (if (and interned-name (symbol-primitive interned-name))
      (depict-primitive markup-stream (symbol-primitive interned-name))
      (progn
        (unless (symbol-type interned-name)
          (warn "~A is depicted as a global variable but isn't one" name))
        (depict-global-variable markup-stream name :reference)))))

(setf (styled-text-depictor :global) #'depict-styled-text-global-variable)


; (:local <name>)
(setf (styled-text-depictor :local) #'depict-local-variable)


; (:constant <value>)
; <value> can be either an integer, a float, a character, or a string.
(setf (styled-text-depictor :constant) #'depict-constant)


; (:action <name>)
(setf (styled-text-depictor :action) #'depict-action-name)

