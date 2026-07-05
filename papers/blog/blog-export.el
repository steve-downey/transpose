;;; blog-export.el --- Shared Org->HTML export config for the transpose blog  -*- lexical-binding: t; -*-

;;; Commentary:
;;
;; Loaded by the `papers/blog' Makefile on top of the repo `.emacs.d/init.el'
;; (which supplies org, htmlize, citeproc, and org-transclusion).  Its only job
;; is to make a standalone HTML export succeed for posts that are normally
;; published to sdowney.org through org2blog.
;;
;; `{{{TEASER_END}}}' is org2blog's "read more" fold marker.  The publishing
;; path defines it; a bare `org-export-to-file' does not, and Org aborts on the
;; undefined macro.  We define it here as the empty string (the teaser fold is
;; meaningless in a single self-contained page).  `modification-time' is a
;; built-in Org macro and needs no help.

;;; Code:

(require 'org)

(setq org-export-global-macros
      (append '(("TEASER_END" . ""))
              (and (boundp 'org-export-global-macros) org-export-global-macros)))

;; Citations use the built-in `basic' processor: no #+cite_export / CSL setup is
;; present in the posts, and it renders [cite:@key] + #+print_bibliography: from
;; the local references.bib without pulling in a style file.
(with-eval-after-load 'oc
  (setq org-cite-export-processors '((t basic))))

;; The posts reference source with `orgit-file:PATH' links (an Emacs/magit link
;; type).  Without the orgit package those links are unresolvable, and since the
;; posts set `#+OPTIONS: broken-links:nil' an unresolvable link aborts the whole
;; export.  Register a lightweight export handler so the links resolve to a
;; monospaced anchor at PATH.
;;
;; NOTE: these PATHs are still the trees-repo layout (src/smd/...); they need to
;; be repointed at this repo's include/beman/transpose/... (or a GitHub URL)
;; before publishing.  For now they render as visible references.
(with-eval-after-load 'ox
  (org-link-set-parameters
   "orgit-file"
   :export (lambda (path desc backend _info)
             (let ((label (or desc path)))
               (pcase backend
                 ((or 'html 're-reveal)
                  (format "<a href=\"%s\"><code>%s</code></a>" path label))
                 (_ label))))))

(provide 'blog-export)
;;; blog-export.el ends here
