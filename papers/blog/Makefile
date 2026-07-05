# papers/blog/Makefile --- render the transpose persuasive blog series to HTML
#
# Reuses the repo's real Org toolchain (../../.emacs.d/init.el: org, htmlize,
# citeproc, org-transclusion) plus blog-export.el for the org2blog TEASER_END
# macro.  Self-contained: `make` here builds every *.org post to *.html.
#
#   make          # build all posts to HTML
#   make clean    # remove generated HTML and dep files
#
# First run installs the Emacs packages named in .emacs.d/init.el into
# .emacs.d/elpa-<version> (needs network once).

EMACS     ?= emacs
# Self-contained: this repo's own .emacs.d/init.el installs and configures the
# whole Org toolchain (org, htmlize, citeproc, org-transclusion, ox-gfm).  The
# first build populates .emacs.d/elpa-<version>/ (needs network once); it is
# gitignored.
ROOT      := $(shell git rev-parse --show-toplevel)
EMACSDIR  := $(ROOT)/.emacs.d
EXPORT_EL := $(CURDIR)/blog-export.el

ORGFILES  := $(wildcard *.org)
HTML      := $(ORGFILES:.org=.html)

.PHONY: all clean
all: $(HTML)

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

-include $(ORGFILES:%.org=%.html.deps)

clean:
	-rm -f $(HTML) $(ORGFILES:%.org=%.html.deps)
