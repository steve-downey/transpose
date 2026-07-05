# papers/blog/Makefile --- render the transpose persuasive blog series
#
# Reuses the repo's Org toolchain (../../.emacs.d/init.el: org, htmlize,
# citeproc, org-transclusion, ox-gfm) plus blog-export.el for the org2blog
# TEASER_END macro and the orgit-file link shim.
#
#   make            # build all posts to HTML
#   make blog-md    # convert all posts to GFM markdown (blog style)
#   make test       # smoke test: render all posts to HTML
#   make clean      # remove generated HTML/MD and dep files
#
# First run installs the Emacs packages named in .emacs.d/init.el into
# .emacs.d/elpa-<version> (needs network once); the store is gitignored.

EMACS     ?= emacs
ROOT      := $(shell git rev-parse --show-toplevel)
EMACSDIR  := $(ROOT)/.emacs.d
EXPORT_EL := $(CURDIR)/blog-export.el

ORGFILES  := $(wildcard *.org)
HTML      := $(ORGFILES:.org=.html)
MD        := $(ORGFILES:.org=.md)

.PHONY: all blog-md test clean

all: $(HTML)

blog-md: $(MD)

test: $(HTML)

%.html : %.org $(EXPORT_EL)
	$(EMACS) --init-directory=$(EMACSDIR)/ \
	  --batch --load $(EMACSDIR)/init.el \
	  -f package-initialize \
	  --load $(EXPORT_EL) \
	  --eval "(setq enable-local-variables :all)" \
	  --visit $< \
	  --eval "(org-transclusion-mode t)" \
	  --eval "(org-export-to-file 'html \"$@\")"
	@echo "$@ : \\" > $@.deps
	@echo "  $<" >> $@.deps
	@sed -n "s/^.*\[\[file:\(\S*\)::.*$$/\1/p" < $< | sort -u | xargs -r printf "  %s \\\\\n" >> $@.deps

%.md : %.org $(EXPORT_EL)
	$(EMACS) --init-directory=$(EMACSDIR)/ \
	  --batch --load $(EMACSDIR)/init.el \
	  -f package-initialize \
	  --load $(EXPORT_EL) \
	  --eval "(setq enable-local-variables :all)" \
	  --visit $< \
	  --eval "(org-transclusion-mode t)" \
	  --eval "(require 'ox-gfm)" \
	  --eval "(org-export-to-file 'gfm \"$(abspath $@)\")"
	@echo "$@ : \\" > $@.deps
	@echo "  $<" >> $@.deps
	@sed -n "s/^.*\[\[file:\(\S*\)::.*$$/\1/p" < $< | sort -u | xargs -r printf "  %s \\\\\n" >> $@.deps

-include $(ORGFILES:%.org=%.html.deps)
-include $(ORGFILES:%.org=%.md.deps)

clean:
	-rm -f $(HTML) $(MD) $(ORGFILES:%.org=%.html.deps) $(ORGFILES:%.org=%.md.deps)
